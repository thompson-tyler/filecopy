#include "utils.h"

#include <sys/stat.h>

#include <cassert>
#include <cstdlib>
#include <fstream>

#include "c150debug.h"
#include "c150nastyfile.h"

using namespace C150NETWORK;
using namespace std;

bool check_directory(const char *dirname) {
    assert(dirname);
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
    return false;
}

bool is_file(const char *filename) {
    assert(filename);
    struct stat statbuf;
    if (lstat(filename, &statbuf) != 0) {
        fprintf(stderr, "is file: Error stating supplied source file %s\n",
                filename);
        return false;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr, "is file: %s exists but is not a regular file\n",
                filename);
        return false;
    }

    return true;
}

void setup_logging(const char *logname, int argc, char *argv[]) {
    //
    //           Choose where debug output should go
    //
    // The default is that debug output goes to cerr.
    //
    // Uncomment the following three lines to direct
    // debug output to a file. Comment them
    // to default to the console.
    //
    // Note: the new DebugStream and ofstream MUST live after we return
    // from setUpDebugLogging, so we have to allocate
    // them dynamically.
    //
    //
    // Explanation:
    //
    //     The first line is ordinary C++ to open a file
    //     as an output stream.
    //
    //     The second line wraps that will all the services
    //     of a comp 150-IDS debug stream, and names that filestreamp.
    //
    //     The third line replaces the global variable c150debug
    //     and sets it to point to the new debugstream. Since c150debug
    //     is what all the c150 debug routines use to find the debug stream,
    //     you've now effectively overridden the default.
    //
    ofstream *outstreamp = new ofstream(logname);
    DebugStream *filestreamp = new DebugStream(outstreamp);
    DebugStream::setDefaultLogger(filestreamp);

    //
    //  Put the program name and a timestamp on each line of the debug log.
    //
    c150debug->setPrefix(argv[0]);
    c150debug->enableTimestamp();

    //
    // Ask to receive all classes of debug message
    //
    // See c150debug.h for other classes you can enable. To get more than
    // one class, you can or (|) the flags together and pass the combined
    // mask to c150debug -> enableLogging
    //
    // By the way, the default is to disable all output except for
    // messages written with the C150ALWAYSLOG flag. Those are typically
    // used only for things like fatal errors. So, the default is
    // for the system to run quietly without producing debug output.
    //
    c150debug->enableLogging(C150APPLICATION | C150NETWORKTRAFFIC |
                             C150NETWORKDELIVERY);
}

// non crypto hash
// taken from chatgpt which took from p4c github
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
