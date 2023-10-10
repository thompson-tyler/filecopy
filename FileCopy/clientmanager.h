#ifndef MANAGER_H
#define MANAGER_H

#include <unordered_map>

#include "c150dgmsocket.h"
#include "c150nastyfile.h"
#include "files.h"
#include "messenger.h"

// This is the class that manages the state machine for the client
class ClientManager {
   public:
    // loads all files from dir
    ClientManager(C150NETWORK::C150NastyFile *nfp,
                  std::vector<std::string> *filenames);

    // loop through filemap and send all the files
    bool sendFiles(Messenger *m);

    // loop through filemap and E2E verify all the files
    bool endToEndCheck(Messenger *m);

   private:
    enum FileTransferStatus {
        LOCALONLY,
        EXISTSREMOTE,
        COMPLETED,
    };

    struct FileTracker {
        uint8_t *filedata;
        int filelen;
        std::string filename;
        FileTransferStatus status;
        int SOScount;
        FileTracker(std::string filename);
        ~FileTracker();
    };

    // if it's in here it IS a local file
    unordered_map<int, FileTracker> m_filemap;

    C150NETWORK::C150NastyFile *m_nfp;
};

#endif
