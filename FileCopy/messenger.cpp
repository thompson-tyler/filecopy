#include "messenger.h"

#include <cassert>
#include <unordered_map>
#include <vector>

#include "c150debug.h"
#include "c150network.h"
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

bool Messenger::send_one(Packet &message) {
    vector<Packet> msgs(1, message);
    return send(msgs);
}

// returns true if successful
bool Messenger::send(vector<Packet> &messages) {
    Packet *packets = messages.data();
    int npackets = messages.size();
    seq_t minseq = m_seqno;

    // Build map of messages to send and SOS's files
    m_seqmap.clear();

    // seq number of the "youngest" message in this group

    c150debug->printf(C150APPLICATION, "Try to send, beginning with seqno %u\n",
                      m_seqno);

    for (int i = 0; i < npackets; i++) {
        packets[i].hdr.seqno = m_seqno;
        m_seqmap[m_seqno] = &packets[i];
        m_seqno++;
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
            c150debug->printf(C150APPLICATION,
                              "Completed send of %d messages\n",
                              messages.size());
            return true;
        }

        // Send all un-ACK'd messages
        int throttle = 0;
        for (auto &kv_pair : m_seqmap) {
            Packet *p = kv_pair.second;
            m_sock->write((const char *)p, p->hdr.len);
            // c150debug->printf(C150APPLICATION, "Sent packet!\n%s\n",
            //                   p->toString().c_str());
            if (++throttle == MAX_SEND_GROUP)
                break;  // don't send too many at a time
        }

        c150debug->printf(C150APPLICATION, "Sent %d un-ACK'd messages\n",
                          m_seqmap.size());

        // Read loop - wait for ACKs and remove from resend queue
        while (true) {
            Packet p;
            ssize_t len = m_sock->read((char *)&p, MAX_PACKET_SIZE);
            if (m_sock->timedout()) break;
            if (len != p.hdr.len) {
                c150debug->printf(
                    C150APPLICATION,
                    "Received a packet with length %lu but expected "
                    "length was %d\n",
                    len, p.hdr.len);
                continue;
            }

            // Inspect packet
            if (p.hdr.seqno < minseq) continue;

            // c150debug->printf(C150APPLICATION, "Received valid
            // packet!\n%s\n",
            //                   p.toString().c_str());

            if (p.hdr.type == ACK)
                m_seqmap.erase(p.hdr.seqno);
            else if (p.hdr.type == SOS)  // Something went wrong
                return false;
        }

        c150debug->printf(
            C150APPLICATION,
            "Send round complete, resending %d un-ACK'd messages\n",
            m_seqmap.size());
    }

    throw C150NetworkException("Messenger failure, network error");
}

bool Messenger::sendBlob(string blob, int blobid, string blobName) {
    vector<Packet> sectionMessages = partitionBlob(blob, blobid);
    Packet prepMessage =
        Packet().ofPrepareForBlob(blobid, blobName, sectionMessages.size());
    return (send_one(prepMessage) && send(sectionMessages));
}

vector<Packet> Messenger::partitionBlob(string blob, int blobid) {
    vector<Packet> messages;
    size_t pos = 0;
    uint32_t partno = 0;

    Packet m;
    while (pos < blob.size()) {
        string data_string = blob.substr(pos, sizeof(m.value.section.data));
        c150debug->printf(C150APPLICATION,
                          "partitioning id %i partno %i of length %u\n", blobid,
                          partno, data_string.length());
        pos += data_string.length();
        m.ofBlobSection(blobid, partno++, data_string.length(),
                        (uint8_t *)data_string.c_str());
        messages.push_back(m);
    }

    c150debug->printf(C150APPLICATION, "partitioned into %u messages\n",
                      messages.size());

    return messages;
}
