#ifndef PACKET_H
#define PACKET_H

#include <openssl/sha.h>

#include "c150dgmsocket.h"
#include "files.h"
#include "settings.h"

typedef int seq_t;
typedef int fid_t;

// All message types, doesn't discriminate on origin of client vs server
// clang-format off
enum messagetype_e {
    // These don't have to be bit flags, but why not?
    // Original idea was that ACK and SOS preserve 
    // their incoming messages.
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

typedef union payload_t payload_u;

struct Header {
    // minimum size of whole packet
    int len;
    MessageType type;
    seq_t seqno = -1;
    fid_t id;
};

const int MAX_PACKET_SIZE = C150NETWORK::MAXDGMSIZE;
const int MAX_PAYLOAD_SIZE = C150NETWORK::MAXDGMSIZE - sizeof(Header);

struct check_is_neccesary_t {
    unsigned char checksum[SHA_DIGEST_LENGTH];
    char filename[MAX_FILENAME_LENGTH];
};

struct prepare_for_blob_t {
    char filename[MAX_FILENAME_LENGTH];
    int nparts;
};

struct blob_section_t {
    int partno;
    int start;
    uint8_t data[MAX_PAYLOAD_SIZE - sizeof(partno) - sizeof(start)];
};

union payload_t {
    check_is_neccesary_t check;
    prepare_for_blob_t prep;
    blob_section_t section;
};

struct packet_t {
    Header hdr;
    payload_t value;

    /* blob section data length */
    int datalen();

    // for debugging
    std::string tostring();
};

/* constructors */
void packet_ack(packet_t *p);
void packet_sos(packet_t *p);
void packet_checkisnecessary(packet_t *p, fid_t id, const char *filename,
                             const checksum_t checksum);
void packet_keepit(packet_t *p, fid_t id);
void packet_deleteit(packet_t *p, fid_t id);
void packet_prepare(packet_t *p, fid_t id, char *filename, uint32_t nparts);
void packet_section(packet_t *p, fid_t id, uint32_t partno, uint32_t offset,
                    uint32_t size, const uint8_t *data);

#endif
