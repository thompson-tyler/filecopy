#include "clientmanager.h"

#include <cassert>
#include <cstddef>

#include "c150debug.h"

ClientManager::ClientManager(C150NastyFile *nfp, string dir,
                             vector<string> *filenames) {
    assert(nfp && filenames);
    m_nfp = nfp;
    m_dir = dir;

    c150debug->printf(C150APPLICATION, "Starting setup of client manager\n");

    for (unsigned int i = 0; i < filenames->size(); i++) {
        string fname = filenames->at(i);
        c150debug->printf(
            C150APPLICATION,
            "Adding transfer to client manager: id=%d, filename=%s\n", i,
            fname.c_str());
        m_filemap[i] = FileTracker();
        m_filemap[i].filename = fname;
    }

    c150debug->printf(C150APPLICATION, "Finished set up client manager\n");
}

ClientManager::~ClientManager() {
    for (auto &kv_pair : m_filemap) {
        FileTracker &ft = kv_pair.second;
        ft.deleteFileData();
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
            ft.filelen = fileToBuffer(m_nfp, makeFileName(m_dir, ft.filename),
                                      &ft.filedata, ft.checksum);

        // Convert file data to string for sending
        string data_string = string((char *)ft.filedata, ft.filelen);

        c150debug->printf(C150APPLICATION, "Trying to send file %s\n",
                          ft.filename.c_str());

        // Send file using messenger
        bool success = m->sendBlob(data_string, f_id, ft.filename);

        // Update status based on whether transfer succeeded
        if (success) {
            c150debug->printf(C150APPLICATION, "File transfer successful: %s\n",
                              ft.filename.c_str());
            ft.status = EXISTSREMOTE;
            ft.deleteFileData();
        } else {
            c150debug->printf(C150APPLICATION, "File transfer failed: %s\n",
                              ft.filename.c_str());
        }
    }

    // Return false if some files failed to send
    for (auto &kv_pair : m_filemap) {
        FileTracker &ft = kv_pair.second;
        if (ft.status == LOCALONLY) return false;
    }
    return true;
}

void ClientManager::transfer(Messenger *m) {
    assert(m);

    c150debug->printf(C150APPLICATION, "Starting file transfer\n");

    while (!sendFiles(m))
        c150debug->printf(C150APPLICATION,
                          "Some file transfers failed, retrying\n");

    c150debug->printf(C150APPLICATION, "File transfer succeeded\n");
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

        // Request the file check, then update status
        Packet check_msg =
            Packet().ofCheckIsNecessary(f_id, ft.filename, ft.checksum);
        Packet response;
        if (m->send_one(check_msg)) {
            ft.status = COMPLETED;
            response = Packet().ofKeepIt(f_id);
        } else {
            ft.status = LOCALONLY;
            response = Packet().ofDeleteIt(f_id);
        }
        m->send_one(response);

        ft.deleteFileData();
    }

    // Return false if some files failed the check
    for (auto &kv_pair : m_filemap) {
        FileTracker &ft = kv_pair.second;
        if (ft.status != COMPLETED) return false;
    }
    return true;
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
