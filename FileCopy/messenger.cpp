
#include "messenger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <algorithm>
#include <cstdio>

#include "c150debug.h"
#include "c150nastydgmsocket.h"
#include "c150network.h"
#include "files.h"
#include "settings.h"
#include "utils.h"

using namespace C150NETWORK;

bool filesend(packet_t *prep_out, packet_t *sections_out, messenger_t *m);
bool end2end(packet_t *check, int id, messenger_t *m);

packet_t **assign_sequences(messenger_t *m, packet_t *packets, int n_packets) {
    packet_t **seqmap = (packet_t **)malloc(sizeof(&packets) * n_packets);
    for (int i = 0; i < n_packets; i++) {
        packets[i].hdr.seqno = m->seqcount++;
        seqmap[packets[i].hdr.seqno % n_packets] = &packets[i];
    }
    return seqmap;
}

bool send(messenger_t *m, packet_t *packets, int n_packets) {
    // sanity checks
    assert(m && packets);
    assert(n_packets > 0);

    // initialize tmp packets and seqmap
    int minseqno = m->seqcount;
    packet_t **seqmap = assign_sequences(m, packets, n_packets);
    packet_t p;
    int nasty = m->nastiness;

    int unanswered = n_packets;
    for (int resends = RESENDS(nasty); resends && unanswered; resends--) {
        // send group of unanswered packets
        int sent = 0;
        for (int i = 0; i < n_packets && sent < SEND_GROUP(nastiness); i++) {
            if (seqmap[i] == nullptr) continue;
            m->sock->write((char *)seqmap[i], seqmap[i]->hdr.len);
            sent++;
        }

        int unanswered_prev = unanswered;

        do {  // read all incoming
            int len = m->sock->read((char *)&p, MAX_PACKET_SIZE);
            if (len != p.hdr.len || p.hdr.seqno < minseqno ||
                (p.hdr.type & (ACK | SOS)) == 0)
                continue;
            if (p.hdr.seqno > m->seqcount ||  // message from future?
                (p.hdr.type == SOS && seqmap[p.hdr.seqno % n_packets])) {
                m->seqcount = max(m->seqcount, p.value.seqmax + 1);
                return free(seqmap), false;
            } else if (p.hdr.type == ACK && seqmap[p.hdr.seqno % n_packets]) {
                seqmap[p.hdr.seqno % n_packets] = nullptr;
                unanswered--;
            }
        } while (!m->sock->timedout() && unanswered);

        // cut me some slack, im trying
        if (unanswered_prev > unanswered) resends = RESENDS(nasty);
        errp("%d remaining resends, after sending %d and catching %d\n",
             resends, sent, unanswered_prev - unanswered);
    }

    free(seqmap);
    if (unanswered) throw C150NetworkException("Network failure\n");
    return true;
}

void transfer(files_t *fs, messenger_t *m) {
    int attempts = 0;
    auto files = fs->files;
    for (auto it = files.begin(); it != files.end(); attempts++) {  // <== !!
        int id = it->first;
        packet_t prep, *sections = nullptr, check;
        files_topackets(fs, id, &prep, &sections,
                        &check);             // allocs sections
        if (filesend(&prep, sections, m) &&  // frees sections
            end2end(&check, id, m))
            attempts = 0, it++;  // onto the next one
        else if (attempts >= MAX_SOS)
            throw C150NetworkException("Hit SOS MAX. Transfer failed");
        else
            errp("got SOS trying again\n");
    }
}

bool end2end(packet_t *check, int id, messenger_t *m) {
    errp("SENDING REQ FOR CHECK:\n%s", check->tostring().c_str());
    bool endtoend = send(m, check, 1);
    endtoend ? errp("SENDING KEEP\n") : errp("SENDING DELETE\n");
    if (endtoend)
        packet_keepit(check, id);
    else
        packet_deleteit(check, id);
    return endtoend && send(m, check, 1);
}

bool filesend(packet_t *prep_out, packet_t *sections_out, messenger_t *m) {
    errp("SENDING FILE:\n%s", prep_out->tostring().c_str());
    bool succ = send(m, prep_out, 1);
    if (!succ)
        errp("RECEIVED SOS ON PREPARE FILE\n");
    else
        succ = send(m, sections_out, prep_out->value.prep.nparts);
    if (!succ) errp("RECEIVED SOS ON SECTIONS\n");
    free(sections_out);
    return succ;
}
