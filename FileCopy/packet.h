#ifndef PACKET_H
#define PACKET_H

#include <openssl/sha.h>

#include "c150dgmsocket.h"
#include "settings.h"

typedef int seq_t;
typedef int fid_t;
typedef unsigned char checksum_t[SHA_DIGEST_LENGTH];

// All message types, doesn't discriminate on origin of client vs server
// clang-format off
enum messagetype_e {
    // SOS & ACK must == 0
    SOS                = 0b10000000, 
    ACK                = 0b01000000,
    // these don't have to be bitflags, but why not? 
    CHECK_IS_NECESSARY = 0b00000001, 
    KEEP_IT            = 0b00000010,
    DELETE_IT          = 0b00000100,
    PREPARE_FOR_BLOB   = 0b00001000,
    BLOB_SECTION       = 0b00010000,
};
// clang-format on

struct header_t {
    messagetype_e type;
    int len;           // length of whole! packet
    seq_t seqno = -1;  // sequence number is set by messenger_t
    fid_t id;          // id is determined by files_t
};

const int MAX_PACKET_SIZE = C150NETWORK::MAXDGMSIZE;
const int MAX_PAYLOAD_SIZE = C150NETWORK::MAXDGMSIZE - sizeof(header_t);

struct check_is_neccesary_t {
    unsigned char checksum[SHA_DIGEST_LENGTH];
    char filename[FILENAME_LENGTH];
};

struct prepare_for_blob_t {
    char filename[FILENAME_LENGTH];
    int filelength;
    int nparts;
};

struct blob_section_t {
    int partno;
    int offset;
    uint8_t data[MAX_PAYLOAD_SIZE - sizeof(partno) - sizeof(offset)];
};

// clang-format off
union payload_u {                // valid only when type is:
    check_is_neccesary_t check;  // CHECK_IS_NECESSARY
    prepare_for_blob_t prep;     // PREPARE_FOR_BLOB
    blob_section_t section;      // BLOB_SECTION
    int seqmax;                  // ACK | SOS
};
// clang-format on

struct packet_t {
    header_t hdr;
    payload_u value;
    int datalen();           // ONLY for BLOB_SECTION!!! data section nbytes
    std::string tostring();  // for debugging
};

/* constructors */

// Asserts all input data is valid ptrs and char ptrs are null terminated.

void packet_checkisnecessary(packet_t *p, fid_t id, const char *filename,
                             const checksum_t checksum);
void packet_keepit(packet_t *p, fid_t id);
void packet_deleteit(packet_t *p, fid_t id);
void packet_prepare(packet_t *p, fid_t id, const char *filename, int length,
                    uint32_t nparts);
void packet_section(packet_t *p, fid_t id, uint32_t partno, uint32_t offset,
                    uint32_t size, const uint8_t *data);

#endif
