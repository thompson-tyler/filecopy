
#include "messenger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

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
        packets[i].hdr.seqno = m->global_seqcount++;
        seqmap[packets[i].hdr.seqno % n_packets] = &packets[i];
    }
    return seqmap;
}

bool send(messenger_t *m, packet_t *packets, int n_packets) {
    send(m, packets, n_packets, false);
}

bool send(messenger_t *m, packet_t *packets, int n_packets, bool strict) {
    // sanity checks
    assert(m && packets);
    assert(n_packets > 0);

    // initialize tmp packets and seqmap
    int minseqno = m->global_seqcount;
    packet_t **seqmap = assign_sequences(m, packets, n_packets);
    packet_t p;
    int nasty = m->nastiness;

    int unanswered = n_packets;
    for (int resends = RESENDS(nasty); resends > 0 && unanswered; resends--) {
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
            if (len != p.hdr.len || p.hdr.seqno < minseqno) continue;
            if (!strict && p.hdr.seqno > m->global_seqcount) {
                m->global_seqcount = p.hdr.seqno;  // try to recover
                goto cleanup;
            } else if (p.hdr.type == SOS && seqmap[p.hdr.seqno % n_packets])
                goto cleanup;
            else if (p.hdr.type == ACK && seqmap[p.hdr.seqno % n_packets]) {
                seqmap[p.hdr.seqno % n_packets] = nullptr;
                unanswered--;
            }
        } while (!m->sock->timedout() && unanswered);

        // cut me some slack, im trying
        if (unanswered_prev > unanswered) resends = RESENDS(nasty);
        errp("%d remaining resends, after sending %d and catching %d\n",
             resends, sent, unanswered_prev - unanswered);
    }

    if (unanswered) {
        free(seqmap);
        throw C150NetworkException("Network failure\n");
    }
cleanup:
    free(seqmap);
    return unanswered <= 0;
}

void transfer(files_t *fs, messenger_t *m) {
    int n_files = fs->files.size();
    int attempts = 0;
    for (int id = 0; id < n_files; attempts++) {  // <== IMPORTANT! NOT id++
        packet_t prep, *sections = nullptr, check;
        files_topackets(fs, id, &prep, &sections, &check);  // allocs sections
        if (
#ifndef JUST_END_TO_END
            filesend(&prep, sections, m) &&  // frees sections
#endif
            end2end(&check, id, m)) {
            attempts = 0;  // onto the next one
            id++;
        } else if (attempts >= MAX_SOS)
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
