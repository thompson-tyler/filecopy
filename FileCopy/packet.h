#ifndef PACKET_H
#define PACKET_H

#include "c150dgmsocket.h"
#include "message.h"

typedef uint32_t seq_t;

struct Header {
    MessageType type;
    seq_t seqno;
    // size of data contents
    u_int32_t len = 0;
};

struct Packet {
    Header hdr;
    uint8_t data[C150NETWORK::MAXDGMSIZE - sizeof(hdr)];

    //
    // interface
    //
    Packet();                        // zeroed out
    Packet(uint8_t *fromBuffer);     // deserialize
    void toBuffer(uint8_t *buffer);  // serialize
    std::string toString();          // for debugging
};

const int MAX_PACKET_SIZE = C150NETWORK::MAXDGMSIZE - sizeof(Header);

#endif
