#ifndef MESSENGER_H
#define MESSENGER_H

#include "c150dgmsocket.h"
#include "files.h"
#include "packet.h"

/* UDP wrapper */

struct messenger_t {
    C150NETWORK::C150DgmSocket *sock;
    int nastiness;
    int global_seqcount;
};

/* safe UDP send and recv */

/* false if recv SOS : true if recv ACK */
bool send(messenger_t *m, packet_t *packets, int n_packets, bool strict);

/* high level filecopy code */
void transfer(files_t *fs, messenger_t *m);

#endif
