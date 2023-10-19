#ifndef MESSENGER_H
#define MESSENGER_H

#include "c150dgmsocket.h"
#include "files.h"
#include "packet.h"

/* UDP wrapper */

struct messenger_t {
    C150NETWORK::C150DgmSocket *sock;
    int nastiness;
    int seqcount;
};

/* safe UDP send and recv */

/*
 * purpose:
 *      guarantees that the server received and executed each
 *      packet's message
 * params:
 *      ptr to valid array of <n_packets> many packets
 * returns:
 *      false if recv SOS : true if recv ACK
 * */
bool send(messenger_t *m, packet_t *packets, int n_packets);

/*
 * purpose:
 *      send all the filenames in the collection from the client
 *
 *      for each file
 *         read from disk and split it into packets
 *         send a prepare message
 *         send all sections (could be concurrent)
 *         send request for checksum verification
 *           if SOS
 *             send delete it
 *           else
 *             send keep it
 *
 *     if any give SOS try that file again until MAX_SOS
 * params:
 *      a fs loaded ideally by using register_fromdir()
 * returns:
 *      false if recv SOS : true if recv ACK
 * */
void transfer(files_t *fs, messenger_t *m);

#endif
