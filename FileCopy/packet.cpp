#include "packet.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

using namespace std;

void packet_checkisnecessary(packet_t *p, fid_t id, const char *filename,
                             const checksum_t checksum) {
    p->hdr.type = CHECK_IS_NECESSARY;
    p->hdr.len = sizeof(p->hdr) + sizeof(p->value.check);
    p->hdr.id = id;
    p->hdr.seqno = -1;
    strncpy(p->value.check.filename, filename, MAX_FILENAME_LENGTH);
    memcpy(p->value.check.checksum, checksum, SHA_DIGEST_LENGTH);
}

void packet_keepit(packet_t *p, fid_t id) {
    p->hdr.type = KEEP_IT;
    p->hdr.len = sizeof(p->hdr);
    p->hdr.id = id;
    p->hdr.seqno = -1;
}

void packet_deleteit(packet_t *p, fid_t id) {
    p->hdr.type = DELETE_IT;
    p->hdr.len = sizeof(p->hdr);
    p->hdr.id = id;
    p->hdr.seqno = -1;
}

void packet_prepare(packet_t *p, fid_t id, const char *filename, int length,
                    uint32_t nparts) {
    p->hdr.type = PREPARE_FOR_BLOB;
    p->hdr.len = sizeof(p->hdr) + sizeof(p->value.prep);
    p->hdr.id = id;
    p->hdr.seqno = -1;
    p->value.prep.filelength = length;
    p->value.prep.nparts = nparts;
    strncpy(p->value.prep.filename, filename, MAX_FILENAME_LENGTH);
}

void packet_section(packet_t *p, fid_t id, uint32_t partno, uint32_t offset,
                    uint32_t size, const uint8_t *data) {
    p->hdr.type = BLOB_SECTION;
    p->hdr.len = sizeof(p->hdr) + sizeof(p->value.section) -
                 sizeof(p->value.section.data) + size;
    p->hdr.id = id;
    p->value.section.partno = partno;
    p->value.section.start = offset;
    p->hdr.seqno = -1;
    memcpy(p->value.section.data, data, size);
}

int packet_t::datalen() {
    assert(hdr.type == BLOB_SECTION);
    /*
     * len = sizeof hdr + sizeof section + size - sizeof data
     */
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
            for (int i = 0; i < MAX_FILENAME_LENGTH && value.check.filename[i];
                 i++)
                ss << value.check.filename[i];
            ss << endl;
            ss << "Checksum: ";
            for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
                ss << hex << (int)value.check.checksum[i];
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
            ss << "File Length: " << value.prep.filelength << endl;
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
