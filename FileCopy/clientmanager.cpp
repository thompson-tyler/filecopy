
// class ClientManager {
//    public:
//     // loads all files from dir
//     ClientManager(C150NETWORK::C150NastyFile *nfp, std::string dir);
//
//     // loop through directory and send all the files
//     bool sendFiles(Messenger &m);
//
//    private:
//     bool openFiles();
//
//     // if it's in here it IS a file
//     unordered_map<std::string, uint8_t *> m_filebufs;
//
//     std::string m_dir;
//     C150NETWORK::C150NastyFile *m_nfp;
// };
//
#include "clientmanager.h"

#include <cassert>
using namespace C150NETWORK;
using namespace std;

ClientManager::ClientManager(C150NastyFile *nfp, string dir) {
    assert(nfp);
    m_nfp = nfp;
    m_dir = dir;
    // TODO: load all files from directory into m_filemap
}

bool sendFiles(Messenger *m) {
    // TODO
    assert(m);
    (void)m;
    return true;
}

ClientManager::FileTracker::FileTracker() {
    buffer = nullptr;
    filelen = -1;
    id = -1;
    SOScount = 0;
}

ClientManager::FileTracker::~FileTracker() { free(buffer); }
