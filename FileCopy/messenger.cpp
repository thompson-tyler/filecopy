#include "messenger.h"

Messenger::Messenger(C150DgmSocket *sock)
{
    this->sock = sock;
    this->sock->turnOnTimeouts(TIMEOUT);
    this->seqNum = 0;
}

Messenger::~Messenger() { delete sock; }

void Messenger::sendPacket(Packet p) { sock->write((char *)&p, sizeof(p)); }

void Messenger::sendAck(seqNum_t seqNum)
{
    Packet ack;
    ack.head.type = ACK;
    ack.head.seqNum = seqNum;
    sendPacket(ack);
}

void Messenger::insertChecksum(Packet *p)
{
    SHA1((const unsigned char *)p, sizeof(*p),
         (unsigned char *)p->head.checksum);
}

bool Messenger::verifyChecksum(Packet *p)
{
    // Remove and save packet checksum
    unsigned char checksum[SHA_DIGEST_LENGTH];
    memcpy(checksum, p->head.checksum, SHA_DIGEST_LENGTH);
    memset(p->head.checksum, 0, SHA_DIGEST_LENGTH);
    // Recompute checksum
    unsigned char newChecksum[SHA_DIGEST_LENGTH];
    SHA1((const unsigned char *)p, sizeof(*p), newChecksum);
    // Restore packet checksum
    memcpy(p->head.checksum, checksum, SHA_DIGEST_LENGTH);
    // Compare checksums
    return memcmp(checksum, newChecksum, SHA_DIGEST_LENGTH) == 0;
}

void Messenger::send(string message)
{
    c150debug->printf(C150APPLICATION, "Sending sequence #%d\n", seqNum);
    // Build packet
    Packet p;
    p.head.type = MESSAGE;
    p.head.seqNum = seqNum;
    if (message.length() >= sizeof(p.data)) {
        fprintf(stderr, "Tried to send a message that's too long\n");
        exit(EXIT_FAILURE);
    }
    strcpy(p.data, message.c_str());
    insertChecksum(&p);

    // Send packet
    bool acked = false;
    while (!acked) {
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
            acked = true;
        }
    }

    // Update sequence number
    seqNum++;
}

string Messenger::read()
{
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
