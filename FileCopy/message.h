#ifndef MESSAGE_H
#define MESSAGE_H

#include <openssl/sha.h>

#include <cstring>
#include <iostream>

struct Packet;

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

struct CheckIsNecessary {
    unsigned char checksum[SHA_DIGEST_LENGTH];
};

// Possible kinds of message values
struct PrepareForBlob {
    uint32_t nparts;
};

struct BlobSection {
    uint32_t partno;
    uint32_t len;
    uint8_t *data;
};

class Message {
   public:
    Message();  // defaults to SOS
    ~Message();

    // conversions with packets (preferred over manual)
    Message(Packet *fromPacket);
    Packet toPacket();

    const MessageType type();
    // all messages coincidentally reference a filename
    // so drew this out, could instead have been a uuid but whatever
    const std::string filename();

    // returns nullptr if invalid access
    const CheckIsNecessary *getCheckIsNecessary();
    const PrepareForBlob *getPrepareForBlob();
    const BlobSection *getBlobSection();

    std::string toString();  // for debug

    /*  Instance constructors e.g.
     * `Message m = Message().ofDeleteIt("myfile");` */

    /* client side */
    Message ofCheckIsNecessary(std::string filename,
                               unsigned char checksum[SHA_DIGEST_LENGTH]);
    Message ofKeepIt(std::string filename);
    Message ofDeleteIt(std::string filename);
    Message ofPrepareForBlob(std::string filename, uint32_t nparts);
    Message ofBlobSection(std::string filename, uint32_t partno, uint32_t size,
                          uint8_t *data);

    /* server side */
    Message intoAck();
    Message intoSOS();

   private:
    union MessageValue {
        CheckIsNecessary check;
        PrepareForBlob prep;
        BlobSection section;
        MessageValue();
        ~MessageValue();
    };

    std::string m_filename;
    MessageType m_type;
    MessageValue m_value;
};

std::string messageTypeToString(MessageType m);

#endif
