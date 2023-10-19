#include "files.h"

#include <dirent.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "c150grading.h"
#include "c150nastyfile.h"
#include "c150network.h"
#include "packet.h"
#include "settings.h"
#include "utils.h"

using namespace C150NETWORK;
using namespace std;

#define SHA_LEN SHA_DIGEST_LENGTH
#define mktmpname(fname)                                                    \
    strcat(strncpy((char *)alloca(FULLNAME + 5), (fname), FILENAME_LENGTH), \
           ".tmp")

#define verify(cond) \
    if (!(cond)) {   \
        return -1;   \
    }
#define verify_or_free(cond, buf) \
    if (!(cond)) {                \
        free(buf);                \
        return -1;                \
    }

#define debug(...) c150debug->printf(C150APPLICATION, __VA_ARGS__);

int fmemread_secure(C150NastyFile *nfp, const int nastiness,
                    uint8_t **buffer_pp, unsigned char checksum_out[]);
int fmemread_naive(NASTYFILE *nfp, uint8_t **buffer_pp);

char *mkfullname(char *dst, const char *dirname, const char *fname) {
    assert(dst && dirname && fname);
    strncpy(dst, dirname, DIRNAME_LENGTH - 1);
    strncat(dst, fname, FILENAME_LENGTH - 1);
    return dst;
}

void files_register_fromdir(files_t *fs, char *dirname, C150NastyFile *nfp,
                            int nastiness) {
    assert(fs && dirname && nfp && 0 <= nastiness && nastiness <= 5);
    assert(strnlen(dirname, DIRNAME_LENGTH) < DIRNAME_LENGTH);

    fs->nfp = nfp;
    fs->nastiness = nastiness;
    strncpy(fs->dirname, dirname, DIRNAME_LENGTH - 1);

    DIR *src = opendir(dirname);
    assert(src);
    struct dirent *file;  // Directory entry for source file
    int id = 0;
    while ((file = readdir(src)) != NULL) {
        if ((strcmp(file->d_name, ".") == 0) ||
            (strcmp(file->d_name, "..") == 0))
            continue;  // never copy . or ..
        int namelen = strnlen(file->d_name, FILENAME_LENGTH);
        assert(namelen < FILENAME_LENGTH);
        id = (int)fnv1a_hash(file->d_name, namelen);
        files_register(fs, id++, file->d_name, false);
    }
    closedir(src);
}

bool files_register(files_t *fs, int id, const char *filename,
                    bool allow_touch) {
    char fullname[FULLNAME];
    mkfullname(fullname, fs->dirname, filename);
    if (!allow_touch && !is_file(fullname)) return false;
    strncpy(fs->files[id].filename, filename, FILENAME_LENGTH);
    errp("REGISTERED FILE: %s as %s with id %u\n", fullname,
         fs->files[id].filename, id);
    return true;
}

/*
 * Given a file id, puts the following into packets
 *
 * All data, sectioned into maximum possible sections
 * Number of sections
 * Filelength
 * Filename
 * File SHA1 hash value
 * File id
 */
int files_topackets(files_t *fs, int id, packet_t *prep_out,
                    packet_t **sections_out, packet_t *check_out) {
    assert(fs && prep_out && sections_out && check_out);
    const char *filename = fs->files[id].filename;
    assert(filename);

    uint8_t *buffer = nullptr;
    checksum_t checksum;

    // read the file
    char fullname[FULLNAME];
    mkfullname(fullname, fs->dirname, filename);
    assert(is_file(fullname) && fs->nfp->fopen(fullname, "rb"));

    int len = fmemread_secure(fs->nfp, fs->nastiness, &buffer, checksum);
    if (len < 0) return -1;
    fs->nfp->fclose();

    const int max_data = sizeof((*sections_out)->value.section.data);
    int nparts = (len / max_data) + (len % max_data ? 1 : 0);

    // make prepare_for_blob message packet
    packet_prepare(prep_out, id, filename, len, nparts);

    // make all blob_section packets
    packet_t *sections = (packet_t *)malloc(sizeof(packet_t) * nparts);
    assert(sections);
    for (int i = 0; i < nparts; i++) {
        int offset = i * max_data;
        int size = min(max_data, len - offset);
        packet_section(&sections[i], id, i, offset, size, buffer + offset);
    }

    // make check is necessary packet
    packet_checkisnecessary(check_out, id, filename, checksum);

    free(buffer);
    *sections_out = sections;
    return nparts;
}

// buffer_in != nullptr
// guaranteed safe write!
bool files_writetmp(files_t *fs, int id, int nbytes, const void *buffer_in,
                    const checksum_t checksum_in) {
    assert(buffer_in && fs);
    assert(nbytes > 0);

    // filename setup
    char tmpname[FULLNAME];
    auto filename = fs->files[id].filename;
    mkfullname(tmpname, fs->dirname, mktmpname(filename));

    int attempts = 0;
    while (attempts++ < DISK_RETRIES(fs->nastiness)) {
        // Try writing to disk
        assert(fs->nfp->fopen(tmpname, "w+b"));
        fs->nfp->fwrite(buffer_in, 1, nbytes);
        fs->nfp->fclose();

        if (verify_hash_tmp(fs, id, checksum_in)) return true;
        *GRADING << "File: " << filename
                 << ", failed to write to disk, attempt " << attempts << endl;
    }
    *GRADING << "File: " << filename
             << ", failed to write to disk, retrying transfer" << endl;
    errp("Total Disk Failure sending SOS\n");
    return false;
}

// renames <dirname>/<filename>.tmp to <dirname>/<filename>
void files_savepermanent(files_t *fs, int id) {
    char filename[FULLNAME];
    char tmpname[FULLNAME];
    mkfullname(filename, fs->dirname, fs->files[id].filename);
    mkfullname(tmpname, fs->dirname, mktmpname(fs->files[id].filename));
    rename(tmpname, filename);
}

// removes <dirname>/<filename>.tmp
void files_remove(files_t *fs, int id) {
    char tmpname[FULLNAME];
    mkfullname(tmpname, fs->dirname, mktmpname(fs->files[id].filename));
    remove(tmpname);
}

// Brute force secure read
// Needs to have <nastiness> matching reads
int fmemread_secure(C150NastyFile *nfp, const int nastiness,
                    uint8_t **buffer_pp, unsigned char checksum_out[]) {
    assert(buffer_pp && *buffer_pp == nullptr && checksum_out);

    // Setup total file buffer
    nfp->fseek(0, SEEK_END);
    int filesize = nfp->ftell();
    *buffer_pp = (uint8_t *)malloc(filesize);
    assert(*buffer_pp);

    // Set up checksum array
    auto checksums = new unsigned char[DISK_RETRIES(nastiness)][SHA_LEN];

    // Read the file in chunks of size READ_CHUNK_SIZE +- (a random number),
    // verifying each chunk as we go
    for (int offset = 0, chunk_size; offset < filesize; offset += chunk_size) {
        // Compute random chunk size
        chunk_size = READ_CHUNK_SIZE + (rand() % (CHUNK_SIZE_RANDOMNESS * 2) -
                                        CHUNK_SIZE_RANDOMNESS);
        // Try reading chunk until a sufficient number of reads match
        bool verified = false;
        uint8_t *chunk_buf = *buffer_pp + offset;
        for (int i = 0; i < DISK_RETRIES(nastiness); i++) {
            nfp->fseek(offset, SEEK_SET);
            int chunk_len = nfp->fread(chunk_buf, 1, chunk_size);
            assert(chunk_len);
            SHA1(chunk_buf, chunk_len, checksums[i]);

            int matches = 0;
            for (int j = 0; j < i; j++)  // for each past checksum
                matches += memcmp(checksums[j], checksums[i], SHA_LEN) == 0;

            if (matches >= MATCHING_READS(nastiness)) {
                verified = true;
                break;
            }
        }
        if (!verified) {
            *GRADING << "Could not reliably read file, retrying transfer"
                     << endl;
            errp(
                "Disk failure -- unable to reliably read section in %d "
                "attempts\n",
                DISK_RETRIES(nastiness));
            free(*buffer_pp);
            *buffer_pp = nullptr;
            delete[] checksums;
            return -1;
        }
    }
    delete[] checksums;

    // Compute the checksum of the whole file
    SHA1(*buffer_pp, filesize, checksum_out);

    return filesize;
}

bool verify_hash_tmp(files_t *fs, int id, const unsigned char *checksum_in) {
    assert(fs && checksum_in);

    // Construct filename of tmp file
    char filename[FULLNAME];
    mkfullname(filename, fs->dirname, mktmpname(fs->files[id].filename));

    // Use secure read to generate checksum of tmp file's contents
    uint8_t *buffer = nullptr;
    checksum_t checksum;
    assert(fs->nfp->fopen(filename, "rb"));
    if (fmemread_secure(fs->nfp, fs->nastiness, &buffer, checksum) < 0)
        return false;
    fs->nfp->fclose();
    free(buffer);

    return memcmp(checksum, checksum_in, SHA_LEN) == 0;
}
