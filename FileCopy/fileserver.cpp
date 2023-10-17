
#include <assert.h>

#include <cstdio>

#include "c150debug.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150network.h"
#include "cache.h"
#include "settings.h"
#include "utils.h"

using namespace C150NETWORK;

void listen(C150DgmSocket *sock, files_t *files, cache_t *cache) {
    packet_t p;
    do {
        int len = sock->read((char *)&p, MAX_PACKET_SIZE);
        if (len <= 0 || p.hdr.len != len || p.hdr.seqno < 0) continue;
        bounce(files, cache, &p);
        sock->write((char *)&p, p.hdr.len);
    } while (!sock->timedout());

    throw C150NetworkException("One minute of inactivity, ending process");
}

int main(int argc, char **argv) {
    // TODO: uncomment
    // GRADEME(argc, argv);
    setup_logging("serverlog.txt", argc, argv);

    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s <networknastiness> <filenastiness> <targetdir>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse arguments
    int network_nastiness = atoi(argv[1]);
    int file_nastiness = atoi(argv[2]);
    char *targetdir = argv[3];

    check_directory(argv[3]);

    // Set up socket
    C150DgmSocket *sock = new C150NastyDgmSocket(network_nastiness);
    sock->turnOnTimeouts(1000 * SERVER_SHUTDOWN_SECONDS);

    c150debug->printf(C150APPLICATION,
                      "Set up server socket with nastiness %d\n",
                      network_nastiness);

    // Set up file socket
    C150NastyFile *nfp = new C150NastyFile(file_nastiness);

    // setup the file collection
    files_t fs;
    fs.nfp = nfp;
    fs.nastiness = file_nastiness;
    assert(strnlen(targetdir, DIRNAME_LENGTH) < DIRNAME_LENGTH);
    strncpy(fs.dirname, targetdir, DIRNAME_LENGTH);

    cache_t cache;

    c150debug->printf(C150APPLICATION, "Set up file handler nastiness %d\n",
                      file_nastiness);

    try {
        listen(sock, &fs, &cache);
    } catch (C150NetworkException &e) {
        // Write to debug log
        c150debug->printf(C150ALWAYSLOG, "Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        cerr << argv[0]
             << ": caught C150NetworkException: " << e.formattedExplanation()
             << endl;
    }

    cache_free(&cache);
    delete sock;
    delete nfp;
}
