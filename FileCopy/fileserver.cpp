
#include <cstdio>

#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "responder.h"

using namespace C150NETWORK;

int main(int argc, char **argv) {
    GRADEME(argc, argv);

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

    // Set up file socket
    C150NastyFile *nfp = new C150NastyFile(file_nastiness);

    listen(sock, nfp, string(targetdir));
}
