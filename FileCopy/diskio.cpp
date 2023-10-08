#include "diskio.h"

#include "settings.h"

#ifndef MAX_DISK_RETRIES
#define MAX_DISK_RETRIES 6
#endif

#include <dirent.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define SHA_LEN SHA_DIGEST_LENGTH

using namespace std;
using namespace C150NETWORK;

// what we'd do if there was no nastiness
uint32_t fileToBufferNaive(NASTYFILE *nfp, string srcfile, uint8_t **buffer_pp);
bool bufferToFileNaive(NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                       uint32_t bufferlen);

// these have an end to end check
uint32_t fileToBufferSecure(NASTYFILE *nfp, string srcfile, uint8_t **buffer_pp,
                            unsigned char checksum[SHA_LEN]);
// these have an end to end check
bool bufferToFileSecure(NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                        uint32_t bufferlen);

// returns -1 in disk error
// guarantees that file read off disk is correct by trying repeatedly and
// matching hashcodes
uint32_t fileToBuffer(NASTYFILE *nfp, string dir, string filename,
                      uint8_t **buffer_pp, unsigned char checksum[SHA_LEN]) {
    string srcfile = makeFileName(dir, filename);
    if (!isFile(srcfile)) {
        fprintf(stderr, "%s is not a file", srcfile.c_str());
        return -1;
    }
    return fileToBufferSecure(nfp, srcfile, buffer_pp, checksum);
}

bool bufferToFile(NASTYFILE *nfp, string dir, string filename, uint8_t *buffer,
                  uint32_t bufferlen) {
    string srcfile = makeFileName(dir, filename);
    if (!isFile(srcfile)) {
        fprintf(stderr, "%s is not a file", srcfile.c_str());
        return -1;
    }

    return bufferToFileSecure(nfp, srcfile, buffer, bufferlen);
}

// Brute force secure read
// needs to have 2 flawless of n reads
uint32_t fileToBufferSecure(NASTYFILE *nfp, string srcfile, uint8_t **buffer_pp,
                            unsigned char checksumOut[SHA_LEN]) {
    unsigned char checksums[MAX_DISK_RETRIES][SHA_LEN];
    for (int i = 0; i < MAX_DISK_RETRIES; i++) {  // for each try
        uint8_t *buffer = nullptr;
        uint8_t buflen = fileToBufferNaive(nfp, srcfile, &buffer);
        SHA1(buffer, buflen, checksums[i]);
        for (int j = 0; j < i; j++) {  // for each past checksum
            if (memcmp(checksums[j], checksums[i], SHA_LEN) == 0) {
                *buffer_pp = buffer;
                memcpy(checksumOut, checksums[i], SHA_LEN);
                return buflen;
            }
        }
        free(buffer);
    }

    fprintf(stderr,
            "Disk failure -- unable to reliably open file %s in %i attempts\n",
            srcfile.c_str(), MAX_DISK_RETRIES);
    return -1;
}

uint32_t fileToBufferNaive(NASTYFILE *nfp, string srcfile,
                           uint8_t **buffer_pp) {
    if (*buffer_pp != nullptr) {
        fprintf(stderr, "CANNOT pass fileToBuffer reference to non-null ptr\n");
        exit(EXIT_FAILURE);
    }

    //
    // Read whole input file
    //
    struct stat statbuf;
    if (lstat(srcfile.c_str(), &statbuf) != 0) {
        fprintf(stderr, "copyFile: Error stating supplied source file %s\n",
                srcfile.c_str());
        exit(20);
    }

    uint8_t *buffer = (uint8_t *)malloc(statbuf.st_size);

    void *fopenretval = nfp->fopen(srcfile.c_str(), "rb");
    if (fopenretval == NULL) {
        cerr << "Error opening input file " << srcfile << endl;
        exit(12);
    }

    uint32_t len = nfp->fread(buffer, 1, statbuf.st_size);
    if (len != statbuf.st_size) {
        cerr << "Error reading input file " << srcfile << endl;
        exit(16);
    }

    free(*buffer_pp);
    *buffer_pp = buffer;
    if (nfp->fclose() != 0) {
        cerr << "Error closing input file " << srcfile << endl;
        exit(16);
    }

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
        if (memcmp(bufferChecksum, diskChecksum, SHA_LEN) == 0) {
            return true;
        }
    }

    fprintf(stderr,
            "Disk failure -- unable to reliably open file %s in %i attempts\n",
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