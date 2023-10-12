#ifndef MANAGER_H
#define MANAGER_H

#include <openssl/sha.h>
#include <sys/stat.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "c150dgmsocket.h"
#include "c150nastyfile.h"
#include "diskio.h"
#include "messenger.h"
#include "settings.h"

using namespace C150NETWORK;
using namespace std;

// This is the class that manages the state machine for the client
class ClientManager {
   public:
    // loads all files from dir
    ClientManager(C150NastyFile *nfp, string dir, vector<string> *filenames);
    ~ClientManager();

    void transfer(Messenger *m);

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
        size_t filelen;
        string filename;
        FileTransferStatus status;
        unsigned char checksum[SHA_DIGEST_LENGTH];
        FileTracker();
        ~FileTracker();
        void deleteFileData();
    };

    // if it's in here it IS a local file
    unordered_map<int, FileTracker> m_filemap;

    C150NastyFile *m_nfp;
    string m_dir;

    // loop through filemap and send all the files
    // returns false if some files reached the SOS limit
    bool sendFiles(Messenger *m);
};

#endif
