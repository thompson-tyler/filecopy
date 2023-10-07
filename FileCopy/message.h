#ifndef MESSAGE_H
#define MESSAGE_H

#include "packet.h"

// TODO: reimplement using a tagged union pattern

// All message types, doesn't discriminate on origin of client vs server
// clang-format off
enum MessageType {
    SOS                = -1,         // check via `packet.header.type == SOS`
    ACK                = 0b10000000, // check via `packet.header.type & ACK`
                                     
    // these don't have to be bit flags, but why not
    CHECK_IS_NECESSARY = 0b00000001, 
    KEEP_IT            = 0b00000010,
    DELETE_IT          = 0b00000100,
    PREPARE_FOR_BLOB   = 0b00001000,
    BLOB_SECTION       = 0b00010000,

    // probably unnecessary, but might be handy to know for debugging
    SERVER_BIT         = 0b01000000,
};
// clang-format on

// Messages inherit from this virtual class
class Message {
   public:
    Message(Packet *fromPacket);  // a big switch statement inside here
    virtual MessageType type();
    virtual Packet toPacket();
    virtual std::string toString();
};

/*
 * SERVER SIDE
 */

class Sos : Message {
   public:
    MessageType type() { return MessageType::SOS; }
};

// On a bit level ack is just any packet with a special bit hack
class Ack : Message {
   public:
    Ack();
    // returns the same packet it received just with an additional ACK bit set
    Ack(Packet *fromPacket);

    MessageType type() { return MessageType::ACK; }
    Packet *p = nullptr;
};

/*
 * CLIENT SIDE
 */

class CheckIsNecessary : Message {
   public:
    CheckIsNecessary(std::string filename);
    MessageType type() { return MessageType::CHECK_IS_NECESSARY; }
    std::string filename;
};

class KeepIt : Message {
   public:
    KeepIt(std::string filename);
    MessageType type() { return MessageType::KEEP_IT; }
    std::string filename;
};

class DeleteIt : Message {
   public:
    DeleteIt(std::string filename);
    MessageType type() { return MessageType::DELETE_IT; }
    std::string filename;
};

class PrepareForBlob : Message {
   public:
    PrepareForBlob(std::string filename, uint32_t nparts);
    MessageType type() { return MessageType::PREPARE_FOR_BLOB; }
    std::string filename;
    uint32_t nparts;
};

class BlobSection : Message {
   public:
    BlobSection(std::string filename, uint32_t partno);
    MessageType type() { return MessageType::BLOB_SECTION; }
    std::string filename;
    uint32_t partno;
    uint32_t size;
    char *data;
};

#endif
