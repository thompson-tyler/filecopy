#include "packet.h"

#include <cstring>
#include <sstream>

using namespace std;

Packet::Packet() {
    hdr.type = SOS;
    hdr.seqno = -1;
    hdr.len = 0;
    memset(data, 0, sizeof(data));
}

Packet::Packet(uint8_t *fromBuffer) {
    Packet *p = (Packet *)fromBuffer;
    hdr = p->hdr;
    memcpy(data, p->data, p->hdr.len);
}

void Packet::toBuffer(uint8_t *buffer) { memcpy(buffer, this, sizeof(*this)); }

uint32_t Packet::totalSize() { return sizeof(hdr) + hdr.len; }

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
