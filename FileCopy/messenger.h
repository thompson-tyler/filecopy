#ifndef MESSENGER_H
#define MESSENGER_H

#include <openssl/sha.h>

#include <string>

#include "c150dgmsocket.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"

using namespace C150NETWORK;  // for all the comp150 utilities

class Messenger {
   private:
    typedef uint32_t seqNum_t;
    enum PacketType {
        MESSAGE,
        ACK,
    };
    struct Header {
        char checksum[SHA_DIGEST_LENGTH];
        PacketType type;
        seqNum_t seqNum;
    };
    struct Packet {
        Header head;
        char data[MAXDGMSIZE - sizeof(Header)];
        string str() const;
    };

    const static int TIMEOUT = 1000;

    C150DgmSocket *sock;
    seqNum_t seqNum;

    void sendPacket(Packet p);
    void sendAck(seqNum_t seqNum);
    void insertChecksum(Packet *p);
    bool verifyChecksum(Packet *p);

   public:
    Messenger(C150DgmSocket *sock);
    ~Messenger();
    void send(std::string message);
    std::string read();
};

#endif
