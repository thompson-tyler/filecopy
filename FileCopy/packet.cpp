#include "packet.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

using namespace std;

// /* client side */
// Packet Packet::ofCheckIsNecessary(int id, std::string filename,
//                                   unsigned char checksum[SHA_DIGEST_LENGTH])
//                                   {
//     hdr.fid = id;
//     hdr.type = CHECK_IS_NECESSARY;
//     hdr.len = sizeof(hdr) + sizeof(value.check);
//
//     assert(filename.length() < MAX_FILENAME_LENGTH);
//     memset(&value.check, 0, sizeof(value.check));
//
//     memcpy(value.check.filename, filename.c_str(), filename.length());
//     memcpy(value.check.checksum, checksum, SHA_DIGEST_LENGTH);
//
//     return *this;
// }
//
// Packet Packet::ofKeepIt(int id) {
//     hdr.len = sizeof(hdr);
//     hdr.fid = id;
//     hdr.type = KEEP_IT;
//     return *this;
// }
//
// Packet Packet::ofDeleteIt(int id) {
//     hdr.len = sizeof(hdr);
//     hdr.fid = id;
//     hdr.type = DELETE_IT;
//     return *this;
// }
//
// Packet Packet::ofPrepareForBlob(int id, std::string filename, uint32_t
// nparts) {
//     hdr.fid = id;
//     hdr.type = PREPARE_FOR_BLOB;
//     hdr.len = sizeof(hdr) + sizeof(value.prep);
//
//     assert(filename.length() < MAX_FILENAME_LENGTH);
//     memset(&value.prep, 0, sizeof(value.prep));
//
//     memcpy(value.prep.filename, filename.c_str(), filename.length());
//     value.prep.nparts = nparts;
//     return *this;
// }
//
// Packet Packet::ofBlobSection(int id, uint32_t partno, uint32_t size,
//                              const uint8_t *data) {
//     hdr.fid = id;
//     hdr.type = BLOB_SECTION;
//     hdr.len = sizeof(hdr) + sizeof(value.section.partno) + size;
//
//     assert(hdr.len <= MAX_PACKET_SIZE);
//     memset(&value.section, 0, sizeof(value.section));
//
//     value.section.partno = partno;
//     memcpy(value.section.data, data, size);
//     return *this;
// }
//
// /* server side */
// Packet Packet::intoAck() {
//     hdr.type = ACK;
//     return *this;
// }
//
// Packet Packet::intoSOS() {
//     hdr.type = SOS;
//     return *this;
// }

void packet_ack(packet_t *p) {
    p->hdr.type = ACK;
    p->hdr.len = sizeof(p->hdr);
}

void packet_sos(packet_t *p) {
    p->hdr.type = SOS;
    p->hdr.len = sizeof(p->hdr);
}

void packet_checkisnecessary(packet_t *p, fid_t id, const char *filename,
                             const checksum_t checksum) {
    p->hdr.type = CHECK_IS_NECESSARY;
    p->hdr.len = sizeof(p->hdr) + sizeof(p->value.check);
    p->hdr.id = id;
    strncpy(p->value.check.filename, filename, MAX_FILENAME_LENGTH);
    memcpy(p->value.check.checksum, checksum, SHA_DIGEST_LENGTH);
}

void packet_keepit(packet_t *p, fid_t id) {
    p->hdr.type = KEEP_IT;
    p->hdr.len = sizeof(p->hdr);
    p->hdr.id = id;
}

void packet_deleteit(packet_t *p, fid_t id) {
    p->hdr.type = DELETE_IT;
    p->hdr.len = sizeof(p->hdr);
    p->hdr.id = id;
}

void packet_prepare(packet_t *p, fid_t id, const char *filename,
                    uint32_t nparts) {
    p->hdr.type = PREPARE_FOR_BLOB;
    p->hdr.len = sizeof(p->hdr) + sizeof(p->value.prep);
    p->hdr.id = id;
    p->value.prep.nparts = nparts;
    strncpy(p->value.prep.filename, filename, MAX_FILENAME_LENGTH);
}

void packet_section(packet_t *p, fid_t id, uint32_t partno, uint32_t offset,
                    uint32_t size, const uint8_t *data) {
    p->hdr.type = BLOB_SECTION;
    p->hdr.len = sizeof(p->hdr) + sizeof(p->value.section) -
                 (sizeof(p->value.section.data) - size);
    p->hdr.id = id;
    p->value.section.partno = partno;
    p->value.section.start = offset;
    memcpy(p->value.section.data, data, size);
}

int packet_t::datalen() {
    assert(hdr.type == BLOB_SECTION);
    // p->hdr.len = sizeof(p->hdr) + sizeof(p->value.section) -
    //              sizeof(p->value.section.data) + size;
    return hdr.len - sizeof(hdr) - sizeof(value.section) +
           sizeof(value.section.data);
}

string packet_t::tostring() {
    stringstream ss;
    ss << "---------------\n";
    ss << "Packet\n";
    ss << "Seqno; " << hdr.seqno << " ID: " << hdr.id << " Len: " << hdr.len
       << endl;
    switch (hdr.type) {
        case SOS:
            ss << "Type: "
               << "SOS\n";
            break;
        case ACK:
            ss << "Type: "
               << "ACK\n";
            break;
        case CHECK_IS_NECESSARY:
            ss << "Type: "
               << "Check is necessary\n";
            ss << "Filename: ";
            for (int i = 0; i < MAX_FILENAME_LENGTH && value.prep.filename[i];
                 i++)
                ss << value.prep.filename[i];
            ss << endl;
            ss << "Checksum: ";
            for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
                ss << hex << value.check.checksum[i];
            ss << endl;
            break;
        case KEEP_IT:
            ss << "Type: "
               << "Keep it\n";
            break;
        case DELETE_IT:
            ss << "Type: "
               << "Delete it\n";
            break;
        case PREPARE_FOR_BLOB:
            ss << "Type: "
               << "Prepare for blob\n";
            ss << "Filename: ";
            for (int i = 0; i < MAX_FILENAME_LENGTH && value.prep.filename[i];
                 i++)
                ss << value.prep.filename[i];
            ss << endl;
            ss << "Number of parts: " << value.prep.nparts << endl;
            break;
        case BLOB_SECTION:
            ss << "Type: "
               << "Blob section\n";
            ss << "Section number: " << value.section.partno << endl;
            ss << "Offset: " << value.section.start << endl;
            break;
    }
    ss << "---------------\n";
    return ss.str();
}
