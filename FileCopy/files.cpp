#include "files.h"

#include <dirent.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstring>

#include "c150network.h"
#include "packet.h"
#include "settings.h"
#include "utils.h"

using namespace C150NETWORK;

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
    strncat(dst, fname, FILENAME_LENGTH);
    return dst;
}

void files_register_fromdir(files_t *fs, char *dirname, C150NastyFile *nfp,
                            int nastiness) {
    assert(fs && dirname && nfp && 0 <= nastiness && nastiness <= 5);
    assert(strnlen(dirname, DIRNAME_LENGTH) < DIRNAME_LENGTH);
    strncpy(fs->dirname, dirname, DIRNAME_LENGTH - 1);

    fs->nfp = nfp;
    fs->nastiness = nastiness;

    DIR *src = opendir(dirname);
    assert(src);
    struct dirent *file;  // Directory entry for source file
    for (int id = 0; (file = readdir(src)) != NULL; id++) {
        if ((strcmp(file->d_name, ".") == 0) ||
            (strcmp(file->d_name, "..") == 0))
            continue;  // never copy . or ..
        assert(strnlen(file->d_name, FILENAME_LENGTH) < FILENAME_LENGTH);
        files_register(fs, id, file->d_name, false);
    }
    closedir(src);
}

bool files_register(files_t *fs, int id, const char *filename, bool allow_new) {
    char fullname[FULLNAME];
    if (!allow_new && !is_file(mkfullname(fullname, fs->dirname, filename)))
        return false;
    strncpy(fs->files[id].filename, filename, FILENAME_LENGTH);
    errp("REGISTERED FILE: %s as %s with id %d\n", fullname,
         fs->files[id].filename, id);
    return true;
}

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
    assert(len > 0);
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
        int size = min(offset, len - offset);
        packet_section(&sections[i], id, i, offset, size, buffer + offset);
    }

    // make check is necessary packet
    packet_checkisnecessary(check_out, id, filename, checksum);

    free(buffer);
    *sections_out = sections;
    return nparts;
}

// buffer_in != nullptr && 0 <= start <= end <= length
// guaranteed safe!
//
// stores in TMP file
bool files_writetmp(files_t *fs, int id, int nbytes, const void *buffer_in,
                    const checksum_t checksum_in) {
    assert(buffer_in && fs);
    assert(nbytes > 0);

    // file open
    char tmpname[FULLNAME];
    mkfullname(tmpname, fs->dirname, mktmpname(fs->files[id].filename));
    assert(fs->nfp->fopen(tmpname, "w+b"));

    checksum_t checksum;

    int attempts = 0;
    do {
        uint8_t *buffer = nullptr;
        fs->nfp->fseek(0, SEEK_SET);
        fs->nfp->fwrite(buffer_in, 1, nbytes);
        int len = fmemread_secure(fs->nfp, fs->nastiness, &buffer, checksum);
        if (len != nbytes) return false;
        free(buffer);
        if (memcmp(checksum_in, checksum, SHA_LEN) == 0) {
            fs->nfp->fclose();
            return true;
        }
    } while (++attempts < DISK_RETRIES(fs->nastiness));

    errp("Total Disk Failure sending SOS\n");
    return false;
}

// renames TMP file to permanent
void files_savepermanent(files_t *fs, int id) {
    char filename[FULLNAME];
    char tmpname[FULLNAME];
    mkfullname(filename, fs->dirname, fs->files[id].filename);
    mkfullname(tmpname, fs->dirname, mktmpname(fs->files[id].filename));
    rename(tmpname, filename);
}

void files_remove(files_t *fs, int id) { remove(fs->files[id].filename); }

// Brute force secure read
// Needs to have <nastiness> matching reads
int fmemread_secure(C150NastyFile *nfp, const int nastiness,
                    uint8_t **buffer_pp, unsigned char checksum_out[]) {
    verify(buffer_pp && *buffer_pp == nullptr);
    assert(checksum_out);

    auto checksums = new unsigned char[DISK_RETRIES(nastiness)][SHA_LEN];
    for (int i = 0; i < DISK_RETRIES(nastiness); i++) {  // for each try
        uint8_t *buffer = nullptr;
        int buflen = fmemread_naive(nfp, &buffer);
        verify_or_free(buffer && buflen > 0, buffer);

        SHA1(buffer, buflen, checksums[i]);
        int matches = 0;
        for (int j = 0; j < i; j++)  // for each past checksum
            matches += memcmp(checksums[j], checksums[i], SHA_LEN) == 0;

        if (matches >= MATCHING_READS(nastiness)) {
            *buffer_pp = buffer;
            memcpy(checksum_out, checksums[i], SHA_LEN);
            delete[] checksums;
            return buflen;
        }

        free(buffer);  // free the bad buffer
        if (i > 1) debug("failed read of file for %ith time\n", i);
    }

    fprintf(stderr,
            "Disk failure -- unable to reliably open file in %d attempts\n",
            DISK_RETRIES(nastiness));
    delete[] checksums;
    return -1;
}

// Read whole input file (mostly taken from Noah's samples)
int fmemread_naive(NASTYFILE *nfp, uint8_t **buffer_pp) {
    assert(*buffer_pp == nullptr);

    nfp->fseek(0, SEEK_END);
    int nbytes = nfp->ftell();
    *buffer_pp = (uint8_t *)malloc(nbytes);
    assert(*buffer_pp);

    nfp->fseek(0, SEEK_SET);
    int len = nfp->fread(*buffer_pp, 1, nbytes);
    verify(len == nbytes);
    return len;
}
