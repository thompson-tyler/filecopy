#include "message.h"

#include <cstdio>
#include <cstdlib>

#include "packet.h"
#include "settings.h"

#ifndef MAX_FILENAME_SIZE
#define MAX_FILENAME_SIZE 80
#endif

string extractFilename(Packet *fromPacket, uint8_t **msgstart);

Message::Message() {
    m_type = SOS;
    m_filename = "";
}

Message::~Message() {
    if (m_type == BLOB_SECTION) free(m_value.section.data);
}

const MessageType Message::type() { return m_type; }
const string Message::filename() { return m_filename; }

Message Message::ofCheckIsNecessary(string filename,
                                    unsigned char checksum[SHA_DIGEST_LENGTH]) {
    m_type = CHECK_IS_NECESSARY;
    m_filename = filename;
    memcpy(m_value.check.checksum, checksum, SHA_DIGEST_LENGTH);
    return *this;
}

Message Message::ofKeepIt(string filename) {
    m_type = KEEP_IT;
    m_filename = filename;
    return *this;
}

Message Message::ofDeleteIt(string filename) {
    m_type = DELETE_IT;
    m_filename = filename;
    return *this;
}

Message Message::ofPrepareForBlob(string filename, uint32_t nparts) {
    m_type = PREPARE_FOR_BLOB;
    m_filename = filename;
    m_value.prep.nparts = nparts;
    return *this;
}

Message Message::ofBlobSection(string filename, uint32_t partno, uint32_t len,
                               const uint8_t *data) {
    m_type = PREPARE_FOR_BLOB;
    m_filename = filename;
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
    if (m_type != CHECK_IS_NECESSARY) return nullptr;
    return &m_value.prep;
}

const BlobSection *Message::getBlobSection() {
    if (m_type != CHECK_IS_NECESSARY) return nullptr;
    return &m_value.section;
}

Message::Message(Packet *fromPacket) {
    uint8_t *valuestart = nullptr;
    m_type = fromPacket->hdr.type;
    m_filename = extractFilename(fromPacket, &valuestart);
    switch (m_type) {
        case SOS:
        case ACK:
        case KEEP_IT:
        case DELETE_IT:  // no extra data
            break;
        case CHECK_IS_NECESSARY:
            memcpy(&m_value.check, valuestart, sizeof(m_value.check));
            break;
        case PREPARE_FOR_BLOB:
            memcpy(&m_value.prep, valuestart, sizeof(m_value.prep));
            break;
        case BLOB_SECTION:  // [ u32 as partno | u8[...] ]
            uint32_t *val = (uint32_t *)valuestart;
            m_value.section.partno = val[0];
            m_value.section.len =
                fromPacket->hdr.len - ((valuestart - fromPacket->data) +
                                       sizeof(m_value.section.partno));
            m_value.section.data = (uint8_t *)malloc(m_value.section.len);
            memcpy(m_value.section.data, val + 1, m_value.section.len);
            break;
    }
}

Packet Message::toPacket() const {
    if (m_filename.length() > MAX_FILENAME_SIZE) {
        fprintf(stderr, "Filename %s exceeded MAX_FILENAME_SIZE %u\n",
                m_filename.c_str(), MAX_FILENAME_SIZE);
        exit(EXIT_FAILURE);
    }

    Packet p;
    p.hdr.type = m_type;
    p.hdr.seqno = -1;

    uint8_t filenamelen = m_filename.length();
    uint8_t *valuestart = p.data + filenamelen + 1;

    // [ u8 as filenamelen | u8[filenamelen] as filename | u8[...] as val ]
    p.data[0] = filenamelen;
    memcpy(p.data + 1, m_filename.c_str(), filenamelen);

    switch (m_type) {
        case SOS:
        case ACK:
        case KEEP_IT:
        case DELETE_IT:
            break;
        case CHECK_IS_NECESSARY:
            memcpy(valuestart, &m_value.check, sizeof(m_value.check));
            break;
        case PREPARE_FOR_BLOB:
            memcpy(valuestart, &m_value.prep, sizeof(m_value.prep));
            break;
        case BLOB_SECTION:  // [ u32 as partno | u8[...] ]
            uint32_t *val = (uint32_t *)valuestart;
            val[0] = m_value.section.partno;
            memcpy(val + 1, m_value.section.data, m_value.section.len);
            break;
    }
    return p;
}

string Message::toString() {
    stringstream ss;
    ss << "Type:\n";
    ss << messageTypeToString(m_type) << endl;
    ss << "Values:\n";
    ss << "Filename:\n";
    ss << m_filename << endl;
    switch (m_type) {
        case SOS:
        case ACK:
        case KEEP_IT:
        case DELETE_IT:
            break;
        case CHECK_IS_NECESSARY:
            ss << "Checksum:\n";
            for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
                ss << hex << m_value.check.checksum[i];
            }
            ss << endl;
        case PREPARE_FOR_BLOB:
            ss << "Number of sections:\n";
            ss << m_value.prep.nparts << endl;
        case BLOB_SECTION:
            ss << "Section number:\n";
            ss << m_value.section.partno << endl;
            ss << "Section len:\n";
            ss << m_value.section.len << endl;
            ss << "Data:\n";
            for (uint32_t i = 0; i < m_value.section.len; i++) {
                ss << m_value.section.data[i];
            }
            ss << endl;
            break;
    }
    return ss.str();
}

// Packet data structure is always
// [ u8 as filenamelen | u8[filenamelen] as filename | u8[...] as msgstart ]
string extractFilename(Packet *fromPacket, uint8_t **msgstart) {
    uint8_t *data = fromPacket->data;

    // save room for nullterm
    char filename[MAX_FILENAME_SIZE + 1] = {0};
    uint8_t filenamelen = data[0];

    if (filenamelen > MAX_FILENAME_SIZE) {
        fprintf(
            stderr,
            "Filename %s is %u, longer than MAX_FILENAME_SIZE %u in packet\n",
            data + 1, filenamelen, MAX_FILENAME_SIZE);
        exit(EXIT_FAILURE);
    }

    memcpy(filename, data + 1, filenamelen);
    filename[MAX_FILENAME_SIZE] = '\0';  // just for sanity
    if (msgstart != nullptr) *msgstart = data + 1 + filenamelen;
    return string(filename);
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
        default:
            return string("SOS");
    }
}

// stupid compiler is happy now

Message::MessageValue::MessageValue() { memset(this, 0, sizeof(*this)); }
Message::MessageValue::~MessageValue() {}
