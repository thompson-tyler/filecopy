#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150utility.h"
#include "clientmanager.h"
#include "settings.h"

using namespace C150NETWORK;

int main(int argc, char **argv) {
    GRADEME(argc, argv);

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

    // Set up socket
    C150DgmSocket *sock = new C150NastyDgmSocket(network_nastiness);
    sock->setServerName(server_name);

    // Set up file socket
    C150NastyFile *nfp = new C150NastyFile(file_nastiness);

    // Get list of files to send
    string dir_string = string(srcdir);
    vector<string> filenames;
    for (auto &fname : std::filesystem::directory_iterator(dir_string))
        filenames.push_back(fname.path());

    ClientManager manager(nfp, &filenames);
    Messenger messenger(sock);

    // Send files
    manager.transfer(&messenger);
}
