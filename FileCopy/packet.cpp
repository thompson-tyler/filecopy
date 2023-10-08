#include "packet.h"

#include <cstring>
#include <sstream>

using namespace std;

Packet::Packet(uint8_t *fromBuffer) {
    Packet *p = (Packet *)(fromBuffer);
    hdr = p->hdr;
    memcpy(data, p->data, MAX_PACKET_SIZE);
}

void Packet::toBuffer(uint8_t *buffer) { memcpy(buffer, this, sizeof(*this)); }

string messageToString(MessageType m) {
    switch (m) {
        case ACK:
            return "ACK";
        case CHECK_IS_NECESSARY:
            return "Check is necessary";
        case KEEP_IT:
            return "Save file";
        case DELETE_IT:
            return "Delete file";
        case PREPARE_FOR_BLOB:
            return "Prepare for file";
        case BLOB_SECTION:
            return "Section of file";
        default:
            return "SOS";
    }
}

string Packet::toString() {
    stringstream ss;
    Message m = Message(this);
    ss << "---------\n";
    ss << "Packet:\n";
    ss << "Seqno:" << hdr.seqno << "\n";
    ss << "Size:" << hdr.len << "\n";
    ss << m.toString();
    ss << "---------\n";
    return ss.str();
}
