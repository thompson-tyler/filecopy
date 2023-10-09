#ifndef MESSENGER_H
#define MESSENGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "c150dgmsocket.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "message.h"
#include "packet.h"
#include "settings.h"

class Messenger {
   public:
    Messenger(C150NETWORK::C150DgmSocket *sock);
    ~Messenger();

    // Sends a message and makes sure it is acknowledged.
    // Returns true if successful, aborts and returns false if SOS (TODO: make
    // sure this is what we want).
    bool send(const vector<Message> &messages);

    // 1. Creates and sends client Message of PREPARE_FOR_BLOB and waits
    // until it's acknowledged.
    // 2. Then splits blob into sections and sends them, making sure all are
    // acknowledged
    //
    // Returns true if successful, aborts and returns false if SOS
    // (TODO: make sure this is what we want).
    bool sendBlob(std::string blob, int blobid, std::string blobName);

   private:
    std::vector<Message> partitionBlob(string blob, int blobid);

    std::string read();

    C150NETWORK::C150DgmSocket *m_sock;
    seq_t m_seqno;

    unordered_map<seq_t, Packet> m_seqmap;
};

#endif
