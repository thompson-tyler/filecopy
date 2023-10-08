#ifndef PACKET_H
#define PACKET_H

#include "c150dgmsocket.h"
#include "message.h"

typedef int seq_t;

struct Header {
    MessageType type = SOS;
    seq_t seqno = -1;
    // size of data contents
    u_int32_t len = 0;
};

struct Packet {
    Header hdr;
    uint8_t data[C150NETWORK::MAXDGMSIZE - sizeof(hdr)];

    //
    // interface
    //
    Packet(uint8_t *fromBuffer);     // deserialize
    void toBuffer(uint8_t *buffer);  // serialize
    std::string toString();          // for debugging
};

const int MAX_PACKET_SIZE = C150NETWORK::MAXDGMSIZE - sizeof(Header);

#endif
