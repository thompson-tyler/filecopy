#include "message.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include "packet.h"
#include "settings.h"

Message::Message() {
    m_type = SOS;
    m_id = -1;
}

Message::~Message() {
    if (m_type == BLOB_SECTION) free(m_value.section.data);
}

const MessageType Message::type() { return m_type; }
const int Message::id() { return m_id; }

Message Message::ofCheckIsNecessary(int id, string filename,
                                    unsigned char checksum[SHA_DIGEST_LENGTH]) {
    m_type = CHECK_IS_NECESSARY;
    m_id = id;
    assert(filename.length() < MAX_FILENAME_LENGTH);
    memset(m_value.check.filename, 0, MAX_FILENAME_LENGTH);
    memcpy(m_value.check.filename, filename.c_str(), filename.length());
    memcpy(m_value.check.checksum, checksum, SHA_DIGEST_LENGTH);
    return *this;
}

Message Message::ofKeepIt(int id) {
    m_type = KEEP_IT;
    m_id = id;
    return *this;
}

Message Message::ofDeleteIt(int id) {
    m_type = DELETE_IT;
    m_id = id;
    return *this;
}

Message Message::ofPrepareForBlob(int id, std::string filename,
                                  uint32_t nparts) {
    m_type = PREPARE_FOR_BLOB;
    m_id = id;
    m_value.prep.nparts = nparts;
    memset(m_value.prep.filename, 0, MAX_FILENAME_LENGTH);
    memcpy(m_value.prep.filename, filename.c_str(), filename.length());
    return *this;
}

Message Message::ofBlobSection(int id, uint32_t partno, uint32_t len,
                               const uint8_t *data) {
    m_type = PREPARE_FOR_BLOB;
    m_id = id;
    m_value.section.partno = partno;
    m_value.section.data = (uint8_t *)malloc(len);
    memcpy(m_value.section.data, data, len);
    return *this;
}

Message Message::intoAck() {
    m_type = ACK;
    return *this;
}

Message Message::intoSOS() {
    m_type = SOS;
    return *this;
}

const CheckIsNecessary *Message::getCheckIsNecessary() {
    if (m_type != CHECK_IS_NECESSARY) return nullptr;
    return &m_value.check;
}

const PrepareForBlob *Message::getPrepareForBlob() {
    if (m_type != PREPARE_FOR_BLOB) return nullptr;
    return &m_value.prep;
}

const BlobSection *Message::getBlobSection() {
    if (m_type != BLOB_SECTION) return nullptr;
    return &m_value.section;
}

//
// PACKET MEMORY STRUCTURE
//  *fromPacket->data    *data
//                |        |
//                V        V
//       PREPARE  [ i32 id | u32 partno       | u8[len] data      ]
//       CHECK    [ i32 id | u8[SHA] checksum | u8[FILE] filename ]
//       SECTION  [ i32 id | u32 nparts       | u8[FILE] filename ]
//

Message::Message(Packet *fromPacket) {
    m_type = fromPacket->hdr.type;

    // see diagram above for visual of these pointers
    m_id = *(int *)fromPacket->data;
    uint8_t *data = fromPacket->data + sizeof(m_id);

    switch (m_type) {
        case SOS:
        case ACK:
        case KEEP_IT:
        case DELETE_IT:  // no extra data
            break;
        case CHECK_IS_NECESSARY:
            memcpy(&m_value.check, data, sizeof(m_value.check));
            break;
        case PREPARE_FOR_BLOB:
            memcpy(&m_value.prep, data, sizeof(m_value.prep));
            break;
        case BLOB_SECTION:
            m_value.section.partno = *(uint32_t *)data;
            m_value.section.len =
                fromPacket->hdr.len -
                (sizeof(m_id) + sizeof(m_value.section.partno));
            m_value.section.data = (uint8_t *)malloc(m_value.section.len);
            memcpy(m_value.section.data, data + sizeof(m_value.section.partno),
                   m_value.section.len);
            break;
    }
}

Packet Message::toPacket() const {
    Packet p;
    p.hdr.type = m_type;
    p.hdr.seqno = -1;
    p.hdr.len = sizeof(m_id);

    // see diagram above for visual of these pointers
    memcpy(p.data, &m_id, sizeof(m_id));
    uint8_t *data = p.data + sizeof(m_id);

    switch (m_type) {
        case SOS:
        case ACK:
        case KEEP_IT:
        case DELETE_IT:
            break;
        case CHECK_IS_NECESSARY:
            memcpy(data, &m_value.check, sizeof(m_value.check));
            p.hdr.len += sizeof(m_value.check);
            break;
        case PREPARE_FOR_BLOB:
            memcpy(data, &m_value.prep, sizeof(m_value.prep));
            p.hdr.len += sizeof(m_value.prep);
            break;
        case BLOB_SECTION:
            ((int *)data)[0] = m_value.section.partno;
            ((int *)data)[1] = m_value.section.len;
            int offset =
                sizeof(m_value.section.partno) + sizeof(m_value.section.len);
            memcpy(data + offset, m_value.section.data, m_value.section.len);
            p.hdr.len += offset + m_value.section.len;
            break;
    }
    return p;
}

string Message::toString() {
    stringstream ss;
    ss << "Type: " << messageTypeToString(m_type) << endl;
    ss << "Payload:\n";
    ss << "Id: " << m_id << endl;
    switch (m_type) {
        case SOS:
        case ACK:
        case KEEP_IT:
        case DELETE_IT:
            break;
        case CHECK_IS_NECESSARY:
            ss << "Filename: ";
            for (int i = 0;
                 i < MAX_FILENAME_LENGTH && m_value.check.filename[i]; i++)
                ss << (char)m_value.prep.filename[i];
            ss << endl;
            ss << "Checksum: 0x";
            for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
                ss << hex << m_value.check.checksum[i];
            ss << endl;
            break;
        case PREPARE_FOR_BLOB:
            ss << "Filename: ";
            fprintf(stderr, "%s\n", m_value.check.filename);
            for (int i = 0;
                 i < MAX_FILENAME_LENGTH && m_value.check.filename[i]; i++) {
                fprintf(stderr, "%c", m_value.check.filename[i]);
                ss << (char)m_value.check.filename[i];
            }
            fprintf(stderr, "\n");
            ss << endl;
            ss << "Number of sections:" << m_value.prep.nparts << endl;
            break;
        case BLOB_SECTION:
            ss << "Section number: " << m_value.section.partno << endl;
            ss << "Section len: " << m_value.section.len << endl;
            ss << "Data:\n";
            for (uint32_t i = 0; i < max(m_value.section.len, 50u); i++)
                ss << m_value.section.data[i];
            if (m_value.section.len > 50) ss << "...\n";
            ss << endl;
            break;
    }
    return ss.str();
}

// utils

string messageTypeToString(MessageType m) {
    switch (m) {
        case ACK:
            return string("ACK");
        case CHECK_IS_NECESSARY:
            return string("Check is necessary");
        case KEEP_IT:
            return string("Save file");
        case DELETE_IT:
            return string("Delete file");
        case PREPARE_FOR_BLOB:
            return string("Prepare for file");
        case BLOB_SECTION:
            return string("Section of file");
        case SOS:
        default:
            return string("SOS");
    }
}

// stupid compiler is happy now

Message::MessageValue::MessageValue() { memset(this, 0, sizeof(*this)); }
Message::MessageValue::~MessageValue() {}
