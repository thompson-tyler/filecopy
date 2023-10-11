#include "clientmanager.h"

#include <cassert>
#include <cstddef>

#include "c150debug.h"

ClientManager::ClientManager(C150NastyFile *nfp, string dir,
                             vector<string> *filenames) {
    assert(nfp && filenames);
    m_nfp = nfp;
    m_dir = dir;

    int f_id = 0;
    for (string fname : *filenames) {
        c150debug->printf(C150APPLICATION,
                          "Adding tranfer of id:%d filename:%s\n", f_id,
                          fname.c_str());

        m_filemap[f_id] = FileTracker();
        m_filemap[f_id].filename = fname;
        f_id++;
    }

    c150debug->printf(C150APPLICATION, "Finished set up client manager\n");
}

bool ClientManager::sendFiles(Messenger *m) {
    assert(m);

    c150debug->printf(C150APPLICATION, "Trying to send files\n");

    for (auto &kv_pair : m_filemap) {
        int f_id = kv_pair.first;
        FileTracker &ft = kv_pair.second;

        // Skip files that have been transfered
        if (ft.status != LOCALONLY) continue;

        c150debug->printf(C150APPLICATION, "Trying to transfer %s\n",
                          ft.filename.c_str());

        // Read file into memory if not already
        if (!ft.filedata) {
            ft.filelen = fileToBuffer(m_nfp, makeFileName(m_dir, ft.filename),
                                      &ft.filedata, ft.checksum);
            c150debug->printf(C150APPLICATION,
                              "Read %s to buffer of length %d\n",
                              ft.filename.c_str(), ft.filelen);
        }

        string data_string = string((char *)ft.filedata, ft.filelen);

        c150debug->printf(C150APPLICATION,
                          "Converted %s buffer to C++ string\n",
                          ft.filename.c_str());

        // Try at most MAX_SOS_COUNT times to send each file
        for (int i = 0; i < MAX_SOS_COUNT; i++) {
            c150debug->printf(C150APPLICATION, "Trying to send file %s\n",
                              ft.filename.c_str());

            // Send file using messenger
            bool success = m->sendBlob(data_string, f_id, ft.filename);

            // Update status based on whether transfer succeeded
            if (success) {
                ft.status = EXISTSREMOTE;
                ft.deleteFileData();
                break;
            }
        }

        assert(false && "DONE SEND ONE FILE!!!");
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
            fileToBuffer(m_nfp, makeFileName(m_dir, ft.filename), &ft.filedata,
                         ft.checksum);

        // Request the check and update status
        Message msg =
            Message().ofCheckIsNecessary(f_id, ft.filename, ft.checksum);
        if (m->send_one(msg)) {
            ft.status = COMPLETED;
            ft.deleteFileData();
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

    c150debug->printf(C150APPLICATION, "Starting transfer\n");

    while (!(sendFiles(m)))  // && endToEndCheck(m)))
        c150debug->printf(C150APPLICATION, "RE-Starting transfer\n");
}

ClientManager::FileTracker::FileTracker() {
    filedata = nullptr;
    filelen = -1;
    status = LOCALONLY;
    memset(checksum, 0, SHA_DIGEST_LENGTH);
}

ClientManager::FileTracker::~FileTracker() { deleteFileData(); }

void ClientManager::FileTracker::deleteFileData() {
    if (filedata) {
        free(filedata);
        filedata = nullptr;
    }
}
