#ifndef MESSENGER_H
#define MESSENGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "c150dgmsocket.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "packet.h"
#include "settings.h"

/* UDP wrapper */

struct messenger_t {
    C150NETWORK::C150NastyDgmSocket *sock;
    int nastiness;
    int global_seqcount;
};

/* safe UDP send and recv */

// false if recv SOS : true if recv ACK
bool messenger_send(messenger_t *m, packet_t *packets, int n_packets);

/* high level filecopy code */

void transfer(files_t *fs, messenger_t *m);

bool end2end(files_t *fs, int id, messenger_t *m);
bool filesend(files_t *fs, int id, messenger_t *m);

#endif
