#include "packet.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

using namespace std;

/* client side */
Packet Packet::ofCheckIsNecessary(int id, std::string filename,
                                  unsigned char checksum[SHA_DIGEST_LENGTH]) {
    hdr.fid = id;
    hdr.type = CHECK_IS_NECESSARY;
    hdr.len = sizeof(hdr) + sizeof(value.check);

    assert(filename.length() < MAX_FILENAME_LENGTH);
    memset(value.check.filename, 0, MAX_FILENAME_LENGTH);
    memcpy(value.check.filename, filename.c_str(), filename.length());
    memcpy(value.check.checksum, checksum, SHA_DIGEST_LENGTH);
}

Packet Packet::ofKeepIt(int id) {
    hdr.len = sizeof(hdr);
    hdr.fid = id;
    hdr.type = KEEP_IT;
}

Packet Packet::ofDeleteIt(int id) {
    hdr.len = sizeof(hdr);
    hdr.fid = id;
    hdr.type = DELETE_IT;
}

Packet Packet::ofPrepareForBlob(int id, std::string filename, uint32_t nparts) {
    hdr.fid = id;
    hdr.type = PREPARE_FOR_BLOB;

    assert(filename.length() < MAX_FILENAME_LENGTH);
    memset(value.prep.filename, 0, MAX_FILENAME_LENGTH);
    memcpy(value.prep.filename, filename.c_str(), filename.length());
    value.prep.nparts = nparts;
}

Packet Packet::ofBlobSection(int id, uint32_t partno, uint32_t size,
                             const uint8_t *data) {
    hdr.fid = id;
    hdr.type = BLOB_SECTION;
}

/* servPacket::er side */
Packet Packet::intoAck() {}
Packet Packet::intoSOS() {}

string Packet::toString() {}
