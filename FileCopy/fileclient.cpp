#include <dirent.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150utility.h"
#include "files.h"
#include "messenger.h"
#include "settings.h"
#include "utils.h"

using namespace C150NETWORK;
using namespace std;

int main(int argc, char **argv) {
    // GRADEME(argc, argv);
    setup_logging("clientlog.txt", argc, argv);

    if (argc != 5) {
        fprintf(
            stderr,
            "Usage: %s <server> <networknastiness> <filenastiness> <srcdir>\n",
            argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse arguments
    char *server_name = argv[1];
    int network_nastiness = atoi(argv[2]);
    int file_nastiness = atoi(argv[3]);
    char *srcdir = argv[4];

    check_directory(argv[4]);

    // Set up socket
    C150DgmSocket *sock = new C150NastyDgmSocket(network_nastiness);
    sock->setServerName(server_name);

    // Set up file handler
    C150NastyFile *nfp = new C150NastyFile(file_nastiness);

    cerr << "Set up socket and file handler" << endl;

    messenger_t messenger = {sock, network_nastiness, 0};
    messenger.sock->turnOnTimeouts(CLIENT_TIMEOUT);
    assert(messenger.sock->timeoutIsSet());

    files_t fs;
    files_register_fromdir(&fs, srcdir, nfp, file_nastiness);

    try {
        // Send files
        errp("starting transfer\n");
        transfer(&fs, &messenger);
    } catch (C150Exception &e) {
        cerr << "fileclient: Caught C150Exception: " << e.formattedExplanation()
             << endl;
    };

    delete sock;
    delete nfp;
}
