#ifndef PACKET_H
#define PACKET_H

#include "c150dgmsocket.h"
#include "message.h"

typedef int seq_t;

struct Header {
    MessageType type;
    seq_t seqno;
    // size of data contents!! Doesn't include header
    u_int32_t len;
};

struct Packet {
    Header hdr;
    uint8_t data[C150NETWORK::MAXDGMSIZE - sizeof(hdr)];

    //
    // interface
    //
    Packet();
    Packet(uint8_t *fromBuffer);     // deserialize
    void toBuffer(uint8_t *buffer);  // serialize
    uint32_t totalSize();            // <= sizeof(Packet)
    std::string toString();          // for debugging
};

const int MAX_PACKET_SIZE = C150NETWORK::MAXDGMSIZE - sizeof(Header);

#endif
