#include "messenger.h"

#include <unordered_map>
#include <vector>

#include "c150debug.h"
#include "c150network.h"
#include "message.h"
#include "packet.h"

using namespace C150NETWORK;
using namespace std;

Messenger::Messenger(C150DgmSocket *sock) {
    m_sock = sock;
    m_sock->turnOnTimeouts(MESSENGER_TIMEOUT);
    m_seqno = 0;

    c150debug->printf(C150APPLICATION, "Set up manager\n");
}

Messenger::~Messenger() { delete m_sock; }

bool Messenger::send(const Message &message) {
    return send(vector<Message>(1, message));
}

// returns true if successful
bool Messenger::send(const vector<Message> &messages) {
    // Build map of messages to send and SOS's files
    m_seqmap.clear();
    auto buffer = (char *)malloc(MAX_PACKET_SIZE);

    // seq number of the "youngest" message in this group
    seq_t minseq = m_seqno;

    c150debug->printf(C150APPLICATION, "Try to send, beginning with seqno %u\n",
                      m_seqno);

    for (auto &m : messages) {
        Packet p = m.toPacket();
        p.hdr.seqno = m_seqno++;
        m_seqmap[p.hdr.seqno] = p;
    }

    c150debug->printf(C150APPLICATION,
                      "Assigned packets and sequences to %d messages\n",
                      messages.size());

    // Try sending batches until all messages are ACK'd or network failure
    // TODO: could also try making this smarter, maybe we detect a stall
    // mathematically
    for (int i = 0; i < MAX_RESEND_ATTEMPTS; i++) {
        // all messages were ACK'd!
        if (m_seqmap.size() == 0) {
            free(buffer);
            c150debug->printf(C150APPLICATION,
                              "Completed send of %d messages\n",
                              messages.size());
            return true;
        }

        // Send all un-ACK'd messages
        for (auto &kv_pair : m_seqmap) {
            Packet p = kv_pair.second;
            p.toBuffer((uint8_t *)buffer);
            m_sock->write(buffer, p.totalSize());
            c150debug->printf(C150APPLICATION, "Sent packet!\n%s\n",
                              p.toString().c_str());
        }

        c150debug->printf(C150APPLICATION, "Sent %d un-ACK'd messages\n",
                          m_seqmap.size());

        // Read loop - wait for ACKs and remove from resend queue
        while (true) {
            ssize_t len = m_sock->read(buffer, MAX_PACKET_SIZE);
            if (m_sock->timedout() || len == 0)
                break;
            else if (len > MAX_PACKET_SIZE || len < (ssize_t)sizeof(Header)) {
                fprintf(stderr, "read an incorrectly sized packet, strange");
                continue;
            }

            Packet p = Packet((uint8_t *)buffer);
            if (p.hdr.seqno < minseq)
                continue;
            else if (p.hdr.type == ACK)
                m_seqmap.erase(p.hdr.seqno);
            else if (p.hdr.type == SOS)  // Something went wrong
                return false;
        }

        c150debug->printf(
            C150APPLICATION,
            "Send round complete, resending %d un-ACK'd messages\n",
            m_seqmap.size());
    }

    free(buffer);
    throw C150NetworkException("Network failure");
}

bool Messenger::sendBlob(string blob, int blobid, string blobName) {
    vector<Message> sectionMessages = partitionBlob(blob, blobid);
    Message prepMessage =
        Message().ofPrepareForBlob(blobid, blobName, sectionMessages.size());
    return (send(prepMessage) && send(sectionMessages));
}

vector<Message> Messenger::partitionBlob(string blob, int blobid) {
    vector<Message> messages;
    size_t pos = 0;
    uint32_t partno = 0;

    c150debug->printf(C150APPLICATION,
                      "partitioning blob message %i of length %u\n", blobid,
                      blob.length());

    while (pos < blob.size()) {
        string data_string = blob.substr(pos, MAX_DATA_SIZE);
        pos += data_string.length();
        auto data = (uint8_t *)data_string.c_str();
        Message m = Message().ofBlobSection(blobid, partno++,
                                            data_string.length(), data);
        messages.push_back(std::move(m));
    }
    c150debug->printf(C150APPLICATION, "partitioned into %u messages\n",
                      messages.size());

    return messages;
}
