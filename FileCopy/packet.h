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
    CHECK_IS_NECESSARY = 0b00000001, 
    KEEP_IT            = 0b00000010,
    DELETE_IT          = 0b00000100,
    PREPARE_FOR_BLOB   = 0b00001000,
    BLOB_SECTION       = 0b00010000,
};
// clang-format on

struct header_t {
    // minimum size of whole packet
    messagetype_e type;
    int len;
    seq_t seqno = -1;
    fid_t id;
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

union payload_u {
    check_is_neccesary_t check;
    prepare_for_blob_t prep;
    blob_section_t section;
    int seqmax;
};

struct packet_t {
    header_t hdr;
    payload_u value;

    int datalen();          /* blob section data length */
    std::string tostring(); /* for debugging */
};

/* constructors */
void packet_checkisnecessary(packet_t *p, fid_t id, const char *filename,
                             const checksum_t checksum);
void packet_keepit(packet_t *p, fid_t id);
void packet_deleteit(packet_t *p, fid_t id);
void packet_prepare(packet_t *p, fid_t id, const char *filename, int length,
                    uint32_t nparts);
void packet_section(packet_t *p, fid_t id, uint32_t partno, uint32_t offset,
                    uint32_t size, const uint8_t *data);

#endif
