#ifndef PACKET_H
#define PACKET_H

#include <openssl/sha.h>

#include "c150dgmsocket.h"

enum MessageType { ACK, SOS, CHECK_IS_NECESSARY, KEEP_IT, DELETE_IT };

struct Packet {
    struct {
        MessageType type;
        u_int32_t seqno;
        u_int32_t size = C150NETWORK::MAXDGMSIZE;
        char checksum[SHA_DIGEST_LENGTH];
    } header;
    char data[C150NETWORK::MAXDGMSIZE - sizeof(header)];
};

bool Packet_check(Packet *packet, char *checksum);
int Packet_fill_checksum(Packet *packet);

/* size of buffer must be > MAXDGMSIZE */
void Packet_to_buffer(Packet *packet, char *buffer);
void Packet_from_buffer(char *buffer, Packet *packet);

#endif
