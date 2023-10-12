#include <dirent.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150utility.h"
#include "clientmanager.h"
#include "diskio.h"
#include "settings.h"
#include "utils.h"

using namespace C150NETWORK;
using namespace std;

int main(int argc, char **argv) {
    // GRADEME(argc, argv);
    setUpDebugLogging("clientlog.txt", argc, argv);

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

    checkDirectory(argv[4]);

    // Set up socket
    C150DgmSocket *sock = new C150NastyDgmSocket(network_nastiness);
    sock->setServerName(server_name);

    // Set up file handler
    C150NastyFile *nfp = new C150NastyFile(file_nastiness);

    cerr << "Set up socket and file handler" << endl;

    // Get list of files to send
    vector<string> filenames;
    DIR *src = opendir(srcdir);
    struct dirent *sourceFile;  // Directory entry for source file

    if (src == NULL) {
        fprintf(stderr, "Error opening source directory %s\n", argv[2]);
        exit(8);
    }

    while ((sourceFile = readdir(src)) != NULL) {
        // skip the . and .. names
        if ((strcmp(sourceFile->d_name, ".") == 0) ||
            (strcmp(sourceFile->d_name, "..") == 0))
            continue;  // never copy . or ..

        // do the copy -- this will check for and
        // skip subdirectories
        filenames.push_back(string(sourceFile->d_name));
    }
    closedir(src);

    ClientManager manager(nfp, string(srcdir), &filenames);
    Messenger messenger(sock);

    try {
        bool check_success = false;
        while (!check_success) {
            // Send files
            manager.transfer(&messenger);
            cerr << "File transfer complete, starting e2e" << endl;

            check_success = manager.endToEndCheck(&messenger);
            cerr << "End to end check " << (check_success ? "passed" : "failed")
                 << endl;
        }
    } catch (C150Exception &e) {
        cerr << "fileclient: Caught C150Exception: " << e.formattedExplanation()
             << endl;
    };
}
