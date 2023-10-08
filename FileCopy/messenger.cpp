#include "messenger.h"

#include "packet.h"

Messenger::Messenger(C150NETWORK::C150DgmSocket *sock) {
    m_sock = new C150NETWORK::C150DgmSocket();
    m_sock->turnOnTimeouts(MESSENGER_TIMEOUT);
    m_seqno = 0;
}

Messenger::~Messenger() { delete m_sock; }

bool Messenger::send(std::vector<Message> messages) {
    // Build map of messages to send
    m_seqmap.clear();
    for (auto &m : messages) {
        Packet p = m.toPacket();
        p.hdr.seqno = m_seqno++;
        m_seqmap[p.hdr.seqno] = p;
    }

    // Try sending batches until all messages are ACK'd
    while (m_seqmap.size()) {
        // Try to send all un-ACK'd messages
        auto buffer = (char *)malloc(MAX_PACKET_SIZE);
        for (auto &kv_pair : m_seqmap) {
            Packet p = kv_pair.second;
            p.toBuffer((uint8_t *)buffer);
            m_sock->write(buffer, p.totalSize());
        }

        // Wait for ACKs
        while (true) {
            ssize_t len = m_sock->read(buffer, MAX_PACKET_SIZE);
            if (m_sock->timedout() || len == 0) break;

            // This could be unsafe if packet is malformed, TODO: safety check
            // somehow? we may also just decide to fully trust the server
            Packet p = Packet((uint8_t *)buffer);
            if (p.hdr.type == ACK) {
                m_seqmap.erase(p.hdr.seqno);
            } else if (p.hdr.type == SOS) {
                // Something went wrong, abort
                return false;
            }
        }
    }

    return true;
}

bool Messenger::sendBlob(std::string blob, std::string blobName) {
    std::vector<Message> sectionMessages = partitionBlob(blob, blobName);
    std::vector<Message> prepMessage;
    prepMessage.push_back(
        Message().ofPrepareForBlob(blobName, sectionMessages.size()));
    return (send(prepMessage) && send(sectionMessages));
}

std::vector<Message> Messenger::partitionBlob(std::string blob,
                                              std::string blobName) {
    std::vector<Message> messages;
    size_t pos = 0;
    uint32_t partno = 0;
    while (pos < blob.size()) {
        string data_string = blob.substr(pos, MAX_DATA_SIZE);
        auto data = (uint8_t *)data_string.c_str();
        Message m = Message().ofBlobSection(blobName, partno++,
                                            data_string.length(), data);
        messages.push_back(m);
    }
    return messages;
}
