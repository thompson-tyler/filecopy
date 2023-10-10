
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
#include <cstddef>

ClientManager::ClientManager(C150NastyFile *nfp, vector<string> *filenames) {
    assert(nfp && filenames);
    m_nfp = nfp;

    int f_id = 0;
    for (auto &fname : *filenames) {
        FileTracker ft(fname);
        m_filemap[f_id++] = ft;
    }
}

bool ClientManager::sendFiles(Messenger *m) {
    assert(m);

    for (auto &kv_pair : m_filemap) {
        int f_id = kv_pair.first;
        FileTracker &ft = kv_pair.second;

        // Skip files that have been transfered
        if (ft.status != LOCALONLY) continue;

        // Read file into memory if not already
        if (!ft.filedata)
            fileToBuffer(m_nfp, ft.filename, &ft.filedata, ft.checksum);
        string data_string = string((char *)ft.filedata, ft.filelen);

        // Try at most MAX_SOS_COUNT times to send each file
        for (int i = 0; i < MAX_SOS_COUNT; i++) {
            // Send file using messenger
            bool success = m->sendBlob(data_string, f_id, ft.filename);

            // Update status based on whether transfer succeeded
            if (success) {
                ft.status = EXISTSREMOTE;
                free(ft.filedata);
                ft.filedata = nullptr;
                break;
            }
        }
    }

    // Return false if some files failed to send
    for (auto &kv_pair : m_filemap) {
        FileTracker &ft = kv_pair.second;
        if (ft.status == LOCALONLY) return false;
    }
    return true;
}

bool ClientManager::endToEndCheck(Messenger *m) {
    assert(m);

    for (auto &kv_pair : m_filemap) {
        int f_id = kv_pair.first;
        FileTracker &ft = kv_pair.second;

        // Skip files that have not been transfered, or already checked
        if (ft.status != EXISTSREMOTE) continue;

        // Read file into memory if not already
        if (!ft.filedata)
            fileToBuffer(m_nfp, ft.filename, &ft.filedata, ft.checksum);

        // Request the check and update status
        Message msg =
            Message().ofCheckIsNecessary(f_id, ft.filename, ft.checksum);
        if (m->send(msg)) {
            ft.status = COMPLETED;
            free(ft.filedata);
        }
    }

    // Return false if some files failed the check
    for (auto &kv_pair : m_filemap) {
        FileTracker &ft = kv_pair.second;
        if (ft.status == EXISTSREMOTE) return false;
    }
    return true;
}

void ClientManager::transfer(Messenger *m) {
    assert(m);

    while (true) {
        bool send_status = sendFiles(m);
        bool check_status = endToEndCheck(m);
        if (send_status && check_status) break;
    }
}

ClientManager::FileTracker::FileTracker() {
    filedata = nullptr;
    filelen = -1;
    status = LOCALONLY;
    memset(checksum, 0, SHA_DIGEST_LENGTH);
}

ClientManager::FileTracker::FileTracker(string filename) {
    filedata = nullptr;
    filelen = -1;
    filename = filename;
    status = LOCALONLY;
    memset(checksum, 0, SHA_DIGEST_LENGTH);
}

ClientManager::FileTracker::~FileTracker() { free(filedata); }
