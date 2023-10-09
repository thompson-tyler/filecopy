#ifndef MANAGER_H
#define MANAGER_H

#include <unordered_map>

#include "c150dgmsocket.h"
#include "c150nastyfile.h"
#include "messenger.h"

// This is the class that manages the state machine for the client
class ClientManager {
   public:
    // loads all files from dir
    ClientManager(C150NETWORK::C150NastyFile *nfp, std::string dir);

    // loop through directory and send all the files
    bool sendFiles(Messenger *m);

   private:
    struct FileTracker {
        uint8_t *buffer;
        int filelen;
        int id;
        int SOScount;
        FileTracker();
        ~FileTracker();
    };

    // if it's in here it IS a file
    unordered_map<std::string, FileTracker> m_filemap;

    std::string m_dir;
    C150NETWORK::C150NastyFile *m_nfp;
};

#endif
