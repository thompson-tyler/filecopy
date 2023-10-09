
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

ClientManager::ClientManager(C150NastyFile *nfp, vector<string> *files) {
    assert(nfp);
    m_nfp = nfp;
    // TODO: load all files from directory into m_filemap
}

bool ClientManager::sendFiles(Messenger *m) {
    // TODO
    assert(m);
    (void)m;
    return true;
}

bool ClientManager::endToEndCheck(Messenger *m) {
    // TODO
    assert(m);
    (void)m;
    return true;
}

ClientManager::FileTracker::FileTracker(string filename) {
    filedata = nullptr;
    filelen = -1;
    filename = filename;
    SOScount = 0;
    status = LOCALONLY;
}

ClientManager::FileTracker::~FileTracker() { free(filedata); }
