#ifndef PACKET_H
#define PACKET_H

#include <openssl/sha.h>

#include "c150dgmsocket.h"
#include "messenger.h"

typedef uint32_t seqNum_t;

struct Packet {
    //
    // data members
    //
    struct {
        MessageType type;
        seqNum_t seqno;
        // update if size changes later
        u_int32_t size = C150NETWORK::MAXDGMSIZE;
        char checksum[SHA_DIGEST_LENGTH];
    } header;
    char data[C150NETWORK::MAXDGMSIZE - sizeof(header)];

    //
    // interface
    //

    Packet();                  // empty packet
    Packet(char *fromBuffer);  // deserialize
    ~Packet();
    void toBuffer(char *buffer);  // serialize

    // If checksum is valid returns true,
    // otherwise returns false and destroys the contents of the packet
    bool verifyChecksum();
    int insertChecksum();

    std::string toString();  // for debugging
};

#endif
