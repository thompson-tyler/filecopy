#include "messenger.h"

#include <cstring>
#include <sstream>

using namespace std;

string Messenger::Packet::str() const {
    stringstream ss;
    ss << "|vvvvvvvvvvv>" << endl;
    ss << "|> Packet    " << endl;
    ss << "|> ------    " << endl;
    ss << "|> Type:     " << head.type == MESSAGE ? "Message" : "Ack" << endl;
    ss << "|> Checksum: " << head.checksum << endl;
    ss << "|> Seq num:  " << head.seqNum << endl;
    ss << "|> Data:     " << data << endl;
    ss << "|^^^^^^^^^^^>" << endl;
    return ss.str();
}

Messenger::Messenger(C150DgmSocket *sock) {
    this->sock = sock;
    this->sock->turnOnTimeouts(TIMEOUT);
    this->seqNum = 0;
}

Messenger::~Messenger() { delete sock; }

void Messenger::sendPacket(Packet p) { sock->write((char *)&p, sizeof(p)); }

void Messenger::sendAck(seqNum_t seqNum) {
    Packet ack;
    ack.head.type = ACK;
    ack.head.seqNum = seqNum;
    sendPacket(ack);
}

void Messenger::insertChecksum(Packet *p) {
    // Checksums are calculated on a packet with zero as its existing checksum
    memset(p->head.checksum, 0, SHA_DIGEST_LENGTH);
    SHA1((const unsigned char *)p, sizeof(*p),
         (unsigned char *)p->head.checksum);
}

// Returns true if packet's checksum is valid
// After a failed checksum, doing anything with the packet is UNDEFINED BEHAVIOR
bool Messenger::verifyChecksum(Packet *p) {
    // Extract packet checksum
    unsigned char incomingChecksum[SHA_DIGEST_LENGTH];
    memcpy(incomingChecksum, p->head.checksum, SHA_DIGEST_LENGTH);

    // Recalculate checksum
    insertChecksum(p);
    bool checksumGood =
        memcmp(incomingChecksum, p->head.checksum, SHA_DIGEST_LENGTH) == 0;
    if (!checksumGood) memset(p, 0, sizeof(*p));  // Destroy bad packet
    return checksumGood;
}

void Messenger::send(string message) {
    c150debug->printf(C150APPLICATION, "Sending sequence #%d\n", seqNum);
    // Build packet
    Packet p;
    p.head.type = MESSAGE;
    p.head.seqNum = seqNum;

    // leave room for null terminator
    if (message.length() >= sizeof(p.data)) {
        fprintf(stderr, "Tried to send a message that's too long\n");
        exit(EXIT_FAILURE);
    }

    // null terminates
    strlcpy(p.data, message.c_str(), sizeof(p.data));
    insertChecksum(&p);

    // Send packet
    bool receivedAck = false;
    while (!receivedAck) {
        sendPacket(p);
        Packet ack;
        ssize_t readlen = sock->read((char *)&ack, sizeof(ack));
        if (readlen < 0) {
            fprintf(stderr, "read failed during Messenger::send\n");
            exit(EXIT_FAILURE);
        } else if (!sock->timedout() && readlen == 0) {
            fprintf(stderr, "read returned 0 during Messenger::send\n");
            exit(EXIT_FAILURE);
        } else if (readlen != sizeof(ack)) {
            c150debug->printf(C150APPLICATION,
                              "Got a packet with unexpected size %d\n",
                              readlen);
        } else if (ack.head.type == ACK && ack.head.seqNum == seqNum) {
            receivedAck = true;
        }
    }

    // Update sequence number
    seqNum++;
}

string Messenger::read() {
    // Read packet
    bool read = false;
    Packet p;
    while (!read) {
        ssize_t readlen = sock->read((char *)&p, sizeof(p));
        if (readlen < 0) {
            fprintf(stderr, "read failed during Messenger::read\n");
            exit(EXIT_FAILURE);
        } else if (!sock->timedout() && readlen == 0) {
            fprintf(stderr, "read returned 0 during Messenger::read\n");
            exit(EXIT_FAILURE);
        } else if (readlen != sizeof(p)) {
            c150debug->printf(C150APPLICATION,
                              "Got a packet with unexpected size %d\n",
                              readlen);
        } else if (p.head.type == MESSAGE) {
            // Verify checksum // TODO verify this actually works, and uncomment
            // if (!verifyChecksum(&p)) {
            //     c150debug->printf(C150APPLICATION,
            //                       "Got a packet with invalid checksum\n");
            //     continue;
            // }
            if (p.head.seqNum == seqNum) {
                read = true;
            } else if (p.head.seqNum < seqNum) {
                // Send ACK for old packet
                sendAck(p.head.seqNum);
            } else if (p.head.seqNum > seqNum) {
                // Other machine is ahead of us, this should never happen
                fprintf(stderr, "Received packet with seqNum %d, expected %d\n",
                        p.head.seqNum, seqNum);
                exit(EXIT_FAILURE);
            }
        }
    }

    // Send ACK
    sendAck(seqNum);

    c150debug->printf(C150APPLICATION, "Read sequence #%d\n", seqNum);

    // Update sequence number
    seqNum++;

    return string(p.data);
}
