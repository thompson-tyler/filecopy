#include "files.h"

#include <dirent.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstring>

#include "openssl/sha.h"
#include "packet.h"
#include "utils.h"

using namespace C150NETWORK;

#define SHA_LEN SHA_DIGEST_LENGTH

#define mktmpname(fname)                                              \
    (strcat(strncpy((char *)alloca(MAX_FILENAME_LENGTH + 5), (fname), \
                    MAX_FILENAME_LENGTH),                             \
            ".tmp"))

int fmemread_secure(C150NastyFile *nfp, const char *srcfile,
                    const int nastiness, uint8_t **buffer_pp, int offset,
                    int nbytes, unsigned char checksum_out[]);

int fmemread_naive(NASTYFILE *nfp, const char *file, uint8_t **buffer_pp,
                   int offset, int nbytes);

int length(const char *filename);

files_t files_register_fromdir(char *dirname, C150NastyFile *nfp,
                               int nastiness) {
    files_t fs;
    fs.nfp = nfp;
    fs.nastiness = nastiness;

    DIR *src = opendir(dirname);
    struct dirent *file;  // Directory entry for source file
    if (src == NULL) {
        fprintf(stderr, "Error opening source directory %s\n", dirname);
        exit(8);
    }

    int id = 0;
    while ((file = readdir(src)) != NULL) {
        if ((strcmp(file->d_name, ".") == 0) ||
            (strcmp(file->d_name, "..") == 0))
            continue;  // never copy . or ..
        assert(file->d_namlen < MAX_FILENAME_LENGTH);
        if (files_register(&fs, id, file->d_name))
            fs.files[id].lenght = file->d_reclen;
        id++;
    }
    closedir(src);

    return fs;
}

bool files_register(files_t *fs, int id, const char *filename) {
    if (!is_file(filename)) return false;
    strncpy(fs->files[id].filename, filename, MAX_FILENAME_LENGTH);
    return true;
}

int files_topackets(files_t *fs, int id, packet_t *prep_out,
                    packet_t **sections_out, packet_t *check_out) {
    const char *filename = fs->files[id].filename;
    assert(filename);
    assert(is_file(filename));

    uint8_t *buffer = nullptr;
    checksum_t checksum;

    // read the file
    assert(fs->nfp->fopen(filename, "rb"));
    int len = fmemread_secure(fs->nfp, filename, fs->nastiness, &buffer, 0, -1,
                              checksum);
    fs->nfp->fclose();
    if (len == -1) {
        fprintf(stderr, "critical failure parsing %s\n", filename);
        exit(1);
    }

    const int max_datalen = sizeof((*sections_out)->value.section.data);
    int nparts = len / max_datalen + (len % max_datalen ? 1 : 0);

    // make prepare_for_blob message packet
    packet_prepare(prep_out, id, filename, nparts);

    // make all blob_section packets
    packet_t *sections = (packet_t *)malloc(sizeof(packet_t) * nparts);
    assert(sections);
    for (int i = 0; i < nparts; i++) {
        int offset = i * max_datalen;
        int size = min(size, len - offset);
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
void files_storetmp(files_t *fs, int id, int offset, int nbytes,
                    const void *buffer_in) {
    assert(buffer_in);

    const char *filename = mktmpname(fs->files[id].filename);
    checksum_t checksum_in;
    checksum_t checksum_out;

    assert(filename);
    assert(fs->nfp->fopen(filename, "rb"));

    SHA1((unsigned char *)buffer_in, nbytes, checksum_in);
    uint8_t *buffer = nullptr;

    int attempts = 0;
    do {
        fs->nfp->fseek(offset, SEEK_SET);
        fs->nfp->fwrite(buffer_in, 1, nbytes);
        int len = fmemread_secure(fs->nfp, filename, fs->nastiness, &buffer,
                                  offset, nbytes, checksum_out);
        if (len != nbytes) continue;

        free(buffer);

        if (memcmp(checksum_in, checksum_out, SHA_LEN) == 0) break;
    } while (++attempts);

    fs->nfp->fclose();
}

bool files_checktmp(files_t *fs, int id, const checksum_t checksum_in) {
    uint8_t *buffer = nullptr;
    checksum_t checksum;
    const char *tmpname = mktmpname(fs->files[id].filename);
    assert(fs->nfp->fopen(tmpname, "rb"));
    int len = fmemread_secure(fs->nfp, tmpname, fs->nastiness, &buffer, 0, -1,
                              checksum);
    fs->nfp->fclose();
    free(buffer);
    return len != -1 && memcmp(checksum, checksum_in, SHA_LEN) == 0;
}

// renames TMP file to permanent
void files_savepermanent(files_t *fs, int id) {
    char *filename = fs->files[id].filename;
    char *tmpname = mktmpname(filename);
    rename(filename, tmpname);
}

void files_remove(files_t *fs, int id) { remove(fs->files[id].filename); }

#define verify(cond) \
    if (!(cond)) {   \
        return -1;   \
    }

#define debug(fmt, ...) c150debug->printf(C150APPLICATION, (fmt), __VA_ARGS__);

// Brute force secure read
// needs to have 2 flawless of MAX_DISK_RETRIES reads
int fmemread_secure(C150NastyFile *nfp, const char *srcfile,
                    const int nastiness, uint8_t **buffer_pp, int offset,
                    int nbytes, unsigned char checksum_out[]) {
    assert(buffer_pp && *buffer_pp == nullptr);
    assert(checksum_out);

    auto checksums = new unsigned char[MAX_DISK_RETRIES + nastiness][SHA_LEN];
    for (int i = 0; i < MAX_DISK_RETRIES + nastiness; i++) {  // for each try
        uint8_t *buffer = nullptr;
        int buflen = fmemread_naive(nfp, srcfile, &buffer, offset, nbytes);
        assert(buffer);
        verify(buflen > 0);

        SHA1(buffer, buflen, checksums[i]);
        int matching = 0;
        for (int j = 0; j < i; j++) {  // for each past checksum
            if (memcmp(checksums[j], checksums[i], SHA_LEN) == 0) {
                matching++;
            }
            // Require nastiness matches to be sure
            if (matching >= nastiness) {
                *buffer_pp = buffer;
                memcpy(checksum_out, checksums[i], SHA_LEN);
                return buflen;
            }
        }

        free(buffer);  // free the bad buffer
        if (i > 1) debug("failed read of %s for %ith time\n", srcfile, i);
    }

    fprintf(stderr,
            "Disk failure -- unable to reliably open file %s in %d attempts\n",
            srcfile, MAX_DISK_RETRIES);
    return -1;
}

// Read whole input file (mostly taken from Noah's samples)
int fmemread_naive(NASTYFILE *nfp, const char *file, uint8_t **buffer_pp,
                   int offset, int nbytes) {
    assert(*buffer_pp == nullptr);
    if (offset == 0 && nbytes == -1) nbytes = length(file);
    verify(nbytes > 0 && offset >= 0);

    uint8_t *buffer = (uint8_t *)malloc(nbytes);
    assert(buffer);

    nfp->fseek(offset, SEEK_SET);
    uint32_t len = nfp->fread(buffer, 1, nbytes);

    verify(len != nbytes);

    *buffer_pp = buffer;
    return len;
}

int length(const char *filename) {
    struct stat statbuf;
    assert(lstat(filename, &statbuf) == 0);
    return statbuf.st_size;
}
