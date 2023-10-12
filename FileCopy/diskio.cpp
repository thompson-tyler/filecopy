#include "diskio.h"

#include "c150debug.h"
#include "settings.h"

#ifndef MAX_DISK_RETRIES
#define MAX_DISK_RETRIES 6
#endif

#include <dirent.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define SHA_LEN SHA_DIGEST_LENGTH

using namespace std;
using namespace C150NETWORK;

/*
 * HELPER FUNCTIONS
 */

// what we'd do if there was no nastiness
bool bufferToFileNaive(NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                       uint32_t bufferlen);

// these have an end to end check
int fileToBufferSecure(NASTYFILE *nfp, string srcfile, uint8_t **buffer_pp,
                       unsigned char checksum[SHA_LEN]);
// these have an end to end check
bool bufferToFileSecure(NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                        uint32_t bufferlen);

/*
 * Overloading wrappers
 */

// returns -1 in disk error
// guarantees that file read off disk is correct by trying repeatedly and
// matching hashcodes
int fileToBuffer(NASTYFILE *nfp, string srcfile, uint8_t **buffer_pp,
                 unsigned char checksum[SHA_LEN]) {
    if (!isFile(srcfile)) {
        fprintf(stderr, "%s is not a file", srcfile.c_str());
        return -1;
    }
    return fileToBufferSecure(nfp, srcfile, buffer_pp, checksum);
}

// returns -1 in disk error
// guarantees that data writted to disk is correct by trying repeatedly and
// matching hashcodes
bool bufferToFile(NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                  uint32_t bufferlen) {
    if (!isFile(srcfile)) {
        fprintf(stderr, "%s is not a file", srcfile.c_str());
        return -1;
    }
    return bufferToFileSecure(nfp, srcfile, buffer, bufferlen);
}

// Brute force secure read
// needs to have 2 flawless of MAX_DISK_RETRIES reads
int fileToBufferSecure(NASTYFILE *nfp, string srcfile, uint8_t **buffer_pp,
                       unsigned char checksumOut[SHA_LEN]) {
    assert((buffer_pp == nullptr || *buffer_pp == nullptr) && checksumOut);

    int high_count = 0;
    uint8_t *high_buffer = nullptr;
    checksum_t high_checksum;
    int high_buflen = 0;
    unordered_map<string, int> checksums;
    for (int i = 0; i < HASH_SAMPLES; i++) {
        uint8_t *buffer = nullptr;
        checksum_t checksum;

        int buflen = fileToBufferNaive(nfp, srcfile, &buffer);
        assert(buffer);
        if (buflen == -1) return -1;
        SHA1(buffer, buflen, checksum);

        string checksumStr((char *)checksum, SHA_LEN);
        if (checksums.count(checksumStr)) {
            checksums[checksumStr]++;
        } else {
            checksums[checksumStr] = 1;
        }

        if (checksums[checksumStr] > high_count) {
            high_count = checksums[checksumStr];
            if (high_buffer) free(high_buffer);
            high_buffer = buffer;
            memcpy(high_checksum, checksum, SHA_LEN);
            high_buflen = buflen;
        } else {
            free(buffer);
        }
    }

    if (buffer_pp) {
        *buffer_pp = high_buffer;
    } else {
        free(high_buffer);
    }
    if (checksumOut) memcpy(checksumOut, high_checksum, SHA_LEN);
    return high_buflen;

    // unsigned char checksums[MAX_DISK_RETRIES][SHA_LEN] = {0};
    // for (int i = 0; i < MAX_DISK_RETRIES; i++) {  // for each try
    //     uint8_t *buffer = nullptr;
    //     int buflen = fileToBufferNaive(nfp, srcfile, &buffer);
    //     assert(buffer);
    //     if (buflen == -1) return -1;

    //     SHA1(buffer, buflen, checksums[i]);
    //     int matching = 0;
    //     for (int j = 0; j < i; j++) {  // for each past checksum
    //         if (memcmp(checksums[j], checksums[i], SHA_LEN) == 0) {
    //             matching++;
    //         }
    //         // Require HASH_MATCHES matches to be sure
    //         if (matching >= HASH_MATCHES) {
    //             if (buffer_pp) *buffer_pp = buffer;
    //             if (checksumOut) memcpy(checksumOut, checksums[i],
    //             SHA_LEN); return buflen;
    //         }
    //     }

    //     if (i > 1)
    //         c150debug->printf(C150APPLICATION,
    //                           "failed read of %s for %ith time\n",
    //                           srcfile.c_str(), i);

    //     free(buffer);  // bad buffer
    // }

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

// Brute force secure write (with a read sanity check)
// in terms of luck, it only needs a single perfect write
bool bufferToFileSecure(NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                        uint32_t bufferlen) {
    unsigned char bufferChecksum[SHA_LEN];
    unsigned char diskChecksum[SHA_LEN];
    SHA1(buffer, bufferlen, bufferChecksum);

    for (int i = 0; i < MAX_DISK_RETRIES; i++) {  // for each try
        // write to buffer to disk
        bufferToFileNaive(nfp, srcfile, buffer, bufferlen);

        // read back from the disk
        uint8_t *diskbuf = nullptr;
        fileToBufferSecure(nfp, srcfile, &diskbuf, diskChecksum);
        free(diskbuf);
        if (memcmp(bufferChecksum, diskChecksum, SHA_LEN) == 0) return true;

        c150debug->printf(C150APPLICATION,
                          "failed write of %d bytes in %s for %ith time\n",
                          bufferlen, srcfile.c_str(), i);
    }

    fprintf(stderr,
            "Disk failure -- unable to reliably open file %s in %d attempts\n",
            srcfile.c_str(), MAX_DISK_RETRIES);
    return -1;
}

bool bufferToFileNaive(NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                       uint32_t bufferlen) {
    void *fopenretval = nfp->fopen(srcfile.c_str(), "wb");
    if (fopenretval == NULL) {
        cerr << "Error opening input file " << srcfile << endl;
        exit(12);
    }

    uint32_t len = nfp->fwrite(buffer, 1, bufferlen);
    if (len != bufferlen) {
        cerr << "Error writing file " << srcfile << endl;
        exit(16);
    }

    if (nfp->fclose()) {
        cerr << "Error closing output file " << srcfile << endl;
        exit(16);
    }

    return true;
}

void touch(NASTYFILE *nfp, string fname) {
    if (nfp->fopen(fname.c_str(), "w") == NULL) {
        cerr << "Error opening output file " << fname << endl;
        exit(8);
    }
    if (nfp->fclose()) {
        cerr << "Error closing output file " << fname << endl;
        exit(16);
    }
}

//
//
// Utilities
//
//

void checkDirectory(char *dirname) {
    struct stat statbuf;
    if (lstat(dirname, &statbuf) != 0) {
        fprintf(stderr, "Error stating supplied source directory %s\n",
                dirname);
        exit(8);
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "File %s exists but is not a directory\n", dirname);
        exit(8);
    }
}

bool isFile(string fname) {
    const char *filename = fname.c_str();
    struct stat statbuf;
    if (lstat(filename, &statbuf) != 0) {
        fprintf(stderr, "isFile: Error stating supplied source file %s\n",
                filename);
        return false;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr, "isFile: %s exists but is not a regular file\n",
                filename);
        return false;
    }
    return true;
}

string makeFileName(string dir, string name) {
    stringstream ss;
    ss << dir;
    // make sure dir name ends in /
    if (dir.substr(dir.length() - 1, 1) != "/") ss << '/';
    ss << name;       // append file name to dir
    return ss.str();  // return dir/name
}

string makeTmpFileName(string dir, string name) {
    return makeFileName(dir, name + ".tmp");
}
