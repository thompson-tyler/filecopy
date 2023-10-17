#include "utils.h"

#include <sys/stat.h>

#include <cassert>
#include <cstdlib>
#include <fstream>

#include "c150debug.h"
#include "c150nastyfile.h"

using namespace C150NETWORK;
using namespace std;

void check_directory(const char *dirname) {
    assert(dirname);
    struct stat statbuf;
    if (lstat(dirname, &statbuf) != 0) {
        errp("Error stating supplied source directory %s\n", dirname);
        exit(8);
    }
    if (!S_ISDIR(statbuf.st_mode)) {
        errp("File %s exists but is not a directory\n", dirname);
        exit(8);
    }
}

bool is_file(const char *filename) {
    assert(filename);
    struct stat statbuf;
    if (lstat(filename, &statbuf) != 0) {
        errp("is file: Error stating supplied source file %s\n", filename);
        return false;
    }
    if (!S_ISREG(statbuf.st_mode)) {
        errp("is file: %s exists but is not a regular file\n", filename);
        return false;
    }
    return true;
}

void setup_logging(const char *logname, int argc, char *argv[]) {
    ofstream *outstreamp = new ofstream(logname);
    DebugStream *filestreamp = new DebugStream(outstreamp);
    DebugStream::setDefaultLogger(filestreamp);
    c150debug->setPrefix(argv[0]);
    c150debug->enableTimestamp();
    c150debug->enableLogging(C150APPLICATION | C150NETWORKTRAFFIC |
                             C150NETWORKDELIVERY);
}

// non crypto hash
// taken from somewhere inside https://github.com/p4lang/p4c
unsigned long fnv1a_hash(const void *data, int size) {
    const uint64_t FNV_offset_basis = 14695981039346656037ULL;
    const uint64_t FNV_prime = 1099511628211ULL;
    const unsigned char *bytes = (const unsigned char *)data;
    uint64_t hash = FNV_offset_basis;
    for (int i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= FNV_prime;
    }
    return (unsigned long)hash;
}
