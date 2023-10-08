#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstring>
#include <iostream>

struct Packet;

// All message types, doesn't discriminate on origin of client vs server
// clang-format off
enum MessageType {
    SOS                = -1,         // check via `packet.header.type < 0`
    ACK                = 0b01000000, // check via `packet.header.type & ACK`
                                     
    // these don't have to be bit flags, but why not
    CHECK_IS_NECESSARY = 0b00000001, 
    KEEP_IT            = 0b00000010,
    DELETE_IT          = 0b00000100,
    PREPARE_FOR_BLOB   = 0b00001000,
    BLOB_SECTION       = 0b00010000,
};
// clang-format on

// possible message values
struct CheckIsNecessary {
    std::string filename;
};

struct KeepIt {
    std::string filename;
};

struct DeleteIt {
    std::string filename;
};

struct PrepareForBlob {
    std::string filename;
    uint32_t nparts;
};

struct BlobSection {
    std::string filename;
    uint32_t partno;
    uint32_t size;
    uint8_t *data;
};

// Messages inherit from this virtual class
class Message {
   public:
    Message();
    Message(Packet *fromPacket);  // a big switch statement inside here
    ~Message();

    Packet toPacket();

    const MessageType type();

    // returns nullptr if invalid access
    const CheckIsNecessary *getCheckIsNecessary();
    const KeepIt *getKeepIt();
    const DeleteIt *getDeleteIt();
    const PrepareForBlob *getPrepareForBlob();
    const BlobSection *getBlobSection();

    std::string toString();  // for debug

    /*  Instance constructors e.g.
     * `Message m = Message().ofDeleteIt("myfile");` */

    /* client side */
    void ofCheckIsNecessary(std::string filename);
    void ofKeepIt(std::string filename);
    void ofDeleteIt(std::string filename);
    void ofPrepareForBlob(std::string filename, uint32_t nparts);
    void ofBlobSection(std::string filename, uint32_t partno, uint32_t size,
                       uint8_t *data);

    /* server side */
    // modifies an existing message to add an ACK bit or SOS bit
    void toggleAck();
    void toggleSOS();

   private:
    union MessageValue {
        CheckIsNecessary check;
        KeepIt keep;
        DeleteIt del;
        PrepareForBlob prep;
        BlobSection section;
        MessageValue() { memset(this, 0, sizeof(*this)); }
    };

    MessageType m_type;
    MessageValue m_value;
};

#endif
