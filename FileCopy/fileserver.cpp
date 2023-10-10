
#include <cstdio>

#include "c150debug.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "responder.h"
#include "utils.h"

using namespace C150NETWORK;

int main(int argc, char **argv) {
    // TODO: uncomment
    // GRADEME(argc, argv);
    setUpDebugLogging("serverlog.txt", argc, argv);

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

    // Set up socket
    C150DgmSocket *sock = new C150NastyDgmSocket(network_nastiness);

    c150debug->printf(C150APPLICATION,
                      "Set up server socket with nastiness %d\n",
                      network_nastiness);

    // Set up file socket
    C150NastyFile *nfp = new C150NastyFile(file_nastiness);

    c150debug->printf(C150APPLICATION, "Set up file handler nastiness %d\n",
                      file_nastiness);

    try {
        listen(sock, nfp, string(targetdir));
    } catch (C150NetworkException &e) {
        // Write to debug log
        c150debug->printf(C150ALWAYSLOG, "Caught C150NetworkException: %s\n",
                          e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        cerr << argv[0]
             << ": caught C150NetworkException: " << e.formattedExplanation()
             << endl;
    }
}
