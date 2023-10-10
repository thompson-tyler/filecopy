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
    memset(&value.check, 0, sizeof(value.check));

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
    hdr.len = sizeof(hdr) + sizeof(value.prep);

    assert(filename.length() < MAX_FILENAME_LENGTH);
    memset(&value.prep, 0, sizeof(value.prep));

    memcpy(value.prep.filename, filename.c_str(), filename.length());
    value.prep.nparts = nparts;
}

Packet Packet::ofBlobSection(int id, uint32_t partno, uint32_t size,
                             const uint8_t *data) {
    hdr.fid = id;
    hdr.type = BLOB_SECTION;
    hdr.len = sizeof(hdr) + sizeof(value.section.partno) + size;

    assert(hdr.len < MAX_PACKET_SIZE);
    memset(&value.section, 0, sizeof(value.section));

    value.section.partno = partno;
    memcpy(value.section.data, data, size);
}

/* server side */
Packet Packet::intoAck() { hdr.type = ACK; }

Packet Packet::intoSOS() { hdr.type = SOS; }

int Packet::datalen() {
    assert(hdr.type == BLOB_SECTION);
    return hdr.len - (sizeof(hdr) + sizeof(value.section.partno));
}

string Packet::toString() {
    stringstream ss;
    ss << "---------------\n";
    ss << "Packet\n";
    ss << "Seqno; " << hdr.seqno << " ID: " << hdr.fid << " Len: " << hdr.len
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
            ss << "Filename: "
               << string(value.check.filename, MAX_FILENAME_LENGTH) << endl;
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
            ss << "Filename: "
               << string(value.prep.filename, MAX_FILENAME_LENGTH) << endl;
            ss << "Number of parts: " << value.prep.nparts << endl;
            break;
        case BLOB_SECTION:
            ss << "Type: "
               << "Blob section\n";
            ss << "Section number: " << value.section.partno << endl;
            ss << "Data: "
               << string((char *)value.section.data, std::max(datalen(), 30))
               << endl;
            break;
    }
    ss << "---------------\n";
}
