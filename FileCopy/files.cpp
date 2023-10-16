#include "files.h"

#include <dirent.h>
#include <sys/stat.h>

#include <cstring>

#include "packet.h"
#include "utils.h"

#define SHA_LEN SHA_DIGEST_LENGTH

#define mktmpname(fname)                                              \
    (strcat(strncpy((char *)alloca(MAX_FILENAME_LENGTH + 4), (fname), \
                    MAX_FILENAME_LENGTH),                             \
            ".tmp"))

int fread_secure(C150NETWORK::C150NastyFile *nfp, const char *srcfile,
                 uint8_t **buffer_pp, checksum_t checksum_out[]);

files_t files_register_fromdir(char *dirname, C150NETWORK::C150NastyFile *nfp,
                               int nastiness) {
    files_t fs;
    fs.nfp = nfp;
    fs.nastiness = nastiness;
    // Get list of files to send
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
                    packet_t **sections_out, checksum_t *checksum_out) {
    const char *filename = fs->files[id].filename;

    fs->nfp->fopen(filename, "rb");

    if (!is_file(filename))
        fprintf(stderr, "%s is not a file or could not be read\n", filename);

    uint8_t *buffer = nullptr;

    int len = fread_secure(fs->nfp, filename, &buffer, checksum_out);
    if (len == -1) {
        fprintf(stderr, "critical failure parsing %s\n", filename);
        exit(1);
    }

    fs->nfp->fclose();

    packet_prepare(prep_out, id, filename, nparts);

    int packet_maxdata = sizeof(prep_out->value.section.data);

    return 0;
}

// buffer_in != nullptr && 0 <= start <= end <= length
// guaranteed safe!
//
// stores in TMP file
void files_storetmp(files_t *fs, int id, int offset, int nbytes,
                    const void *buffer_in) {}

bool files_checktmp(files_t *fs, int id, const checksum_t checksum_in) {
    char *buffer = nullptr;
    checksum_t checksum;
    fread_secure(fs->nfp, fs->files[id].filename, &buffer, checksum);
    free(buffer);
    return memcmp(checksum, checksum_in, SHA_LEN) != -1;
}

// renames TMP file to permanent
void files_savepermanent(files_t *fs, int id) {
    char *filename = fs->files[id].filename;
    char *tmpname = mktmpname(filename);
    rename(filename, tmpname);
}

// try delete file and re-initiliaze registry entry
void files_remove(files_t *fs, int id) { remove(fs->files[id].filename); }

// Brute force secure read
// needs to have 2 flawless of MAX_DISK_RETRIES reads
int fread_secure(C150NETWORK::C150NastyFile *nfp, const char *srcfile,
                 uint8_t **buffer_pp, unsigned char checksumOut[]) {
    assert((buffer_pp == nullptr || *buffer_pp == nullptr) && checksumOut);

    unsigned char checksums[MAX_DISK_RETRIES][SHA_LEN] = {0};
    while (int i = 0; i < MAX_DISK_RETRIES; i++) {  // for each try
        uint8_t *buffer = nullptr;
        int buflen = fread_naive(nfp, srcfile, &buffer);
        assert(buffer);
        if (buflen == -1) return -1;

        SHA1(buffer, buflen, checksums[i]);
        int matching = 0;
        for (int j = 0; j < i; j++) {  // for each past checksum
            if (memcmp(checksums[j], checksums[i], SHA_LEN) == 0) {
                matching++;
            }
            // Require HASH_MATCHES matches to be sure
            if (matching >= HASH_MATCHES) {
                if (buffer_pp) *buffer_pp = buffer;
                if (checksumOut) memcpy(checksumOut, checksums[i], SHA_LEN);
                return buflen;
            }
        }

        if (i > 1)
            c150debug->printf(C150APPLICATION,
                              "failed read of %s for %ith time\n",
                              srcfile.c_str(), i);

        free(buffer);  // bad buffer
    }

    fprintf(stderr,
            "Disk failure -- unable to reliably open file %s in %d attempts\n",
            srcfile.c_str(), MAX_DISK_RETRIES);
    return -1;
}

// Read whole input file (mostly taken from Noah's samples)
int fileToBufferNaive(NASTYFILE *nfp, string srcfile, uint8_t **buffer_pp) {
    assert(*buffer_pp == nullptr);

    struct stat statbuf;
    if (lstat(srcfile.c_str(), &statbuf) != 0) {  // maybe file doesn't exist
        fprintf(stderr, "copyFile: Error stating supplied source file %s\n",
                srcfile.c_str());
        return -1;
    }

    uint8_t *buffer = (uint8_t *)malloc(statbuf.st_size);
    assert(buffer);

    // so file does exist -- but something else is wrong, just kill process
    void *fopenretval = nfp->fopen(srcfile.c_str(), "rb");
    if (fopenretval == NULL) {
        cerr << "Error opening input file " << srcfile << endl;
        exit(EXIT_FAILURE);
    }

    uint32_t len = nfp->fread(buffer, 1, statbuf.st_size);
    if (len != statbuf.st_size) {
        cerr << "Error reading input file " << srcfile << endl;
        exit(EXIT_FAILURE);
    }

    if (nfp->fclose() != 0) {
        cerr << "Error closing input file " << srcfile << endl;
        exit(EXIT_FAILURE);
    }

    *buffer_pp = buffer;
    return len;
}
