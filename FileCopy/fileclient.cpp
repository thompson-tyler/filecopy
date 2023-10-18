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
    // TODO: test and handle directories missing the '/'
    // TODO: uncomment
    // GRADEME(argc, argv);
    setup_logging("clientlog.txt", argc, argv);

    // Set random seed
    srand(time(NULL));

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
    string srcdir_str = argv[4];

    // Add trailing '/' to srcdir if necessary
    if (srcdir_str.back() != '/') {
        srcdir_str += '/';
    }
    auto srcdir = (char *)srcdir_str.c_str();

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
        transfer(&fs, &messenger);
    } catch (C150Exception &e) {
        cerr << "fileclient: Caught C150Exception: " << e.formattedExplanation()
             << endl;
    };

    delete sock;
    delete nfp;
}
