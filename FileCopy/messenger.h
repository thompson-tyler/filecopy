#ifndef MESSENGER_H
#define MESSENGER_H

#include <assert.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "c150dgmsocket.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "files.h"
#include "packet.h"
#include "settings.h"

/* UDP wrapper */

struct messenger_t {
    C150NETWORK::C150DgmSocket *sock;
    int nastiness;
    int global_seqcount;
};

/* safe UDP send and recv */

// false if recv SOS : true if recv ACK
bool send(messenger_t *m, packet_t *packets, int n_packets);

/* high level filecopy code */

void transfer(files_t *fs, messenger_t *m);

#endif
