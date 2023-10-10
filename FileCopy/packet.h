#ifndef PACKET_H
#define PACKET_H

#include "c150dgmsocket.h"
#include "openssl/sha.h"
#include "settings.h"

typedef int seq_t;
typedef int fid_t;

// All message types, doesn't discriminate on origin of client vs server
// clang-format off
enum MessageType {
    // these don't have to be bit flags, but why not?
    // Original idea was that ACK and SOS preserve 
    // their original messages.
    //
    // For now no funny business, just used as normal enum.
    SOS                = 0b10000000, 
    ACK                = 0b01000000,
    CHECK_IS_NECESSARY = 0b00000001, 
    KEEP_IT            = 0b00000010,
    DELETE_IT          = 0b00000100,
    PREPARE_FOR_BLOB   = 0b00001000,
    BLOB_SECTION       = 0b00010000,
};
// clang-format on

struct Header {
    // minimum size of whole packet
    u_int32_t len;
    MessageType type;
    seq_t seqno;
    fid_t fid;
};

const int MAX_PACKET_SIZE = C150NETWORK::MAXDGMSIZE;
const int MAX_PAYLOAD_SIZE = C150NETWORK::MAXDGMSIZE - sizeof(Header);

struct CheckIsNecessary {
    unsigned char checksum[SHA_DIGEST_LENGTH];
    char filename[MAX_FILENAME_LENGTH];
};

struct PrepareForBlob {
    char filename[MAX_FILENAME_LENGTH];
    uint32_t nparts;
};

struct BlobSection {
    uint32_t partno;
    uint8_t data[MAX_PAYLOAD_SIZE - sizeof(partno)];
};

union Payload {
    CheckIsNecessary check;
    PrepareForBlob prep;
    BlobSection section;
};

struct Packet {
    Header hdr;
    Payload value;

    // message constructors

    /* client side */
    Packet ofCheckIsNecessary(int id, std::string filename,
                              unsigned char checksum[SHA_DIGEST_LENGTH]);
    Packet ofKeepIt(int id);
    Packet ofDeleteIt(int id);
    Packet ofPrepareForBlob(int id, std::string filename, uint32_t nparts);
    Packet ofBlobSection(int id, uint32_t partno, uint32_t size,
                         const uint8_t *data);
    /* server side */
    Packet intoAck();
    Packet intoSOS();

    // for debugging
    std::string toString();
};

typedef Packet Message;

#endif
