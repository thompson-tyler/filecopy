
#include "messenger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "c150debug.h"
#include "c150network.h"
#include "files.h"
#include "packet.h"
#include "settings.h"
#include "utils.h"

using namespace C150NETWORK;

bool filesend(packet_t *prep_out, packet_t *sections_out, messenger_t *m);
bool end2end(packet_t *check, int id, messenger_t *m);

// TODO: factor in nastiness
bool made_progress(int prev, int curr, int nastiness) {
    (void)nastiness;
    return prev > curr;  // bare minimum
}

// TODO: factor in nastiness
bool throttle(int nel, int nastiness) {
    (void)nastiness;
    return nel;
}

packet_t **assign_sequences(messenger_t *m, packet_t *packets, int n_packets) {
    packet_t **seqmap = (packet_t **)malloc(sizeof(&packets) * n_packets);
    for (int i = 0; i < n_packets; i++) {
        packets[i].hdr.seqno = m->global_seqcount++;
        seqmap[packets[i].hdr.seqno % n_packets] = &packets[i];
    }
    return seqmap;
}

bool send(messenger_t *m, packet_t *packets, int n_packets) {
    // sanity checks
    assert(m && packets);
    assert(n_packets > 0);

    // initialize tmp packets and seqmap
    int minseqno = m->global_seqcount;
    packet_t **seqmap = assign_sequences(m, packets, n_packets);
    packet_t p;

    int unanswered = n_packets;
    int max_group = throttle(MAX_SEND_GROUP, m->nastiness);
    for (int resends = MAX_RESEND_ATTEMPTS; resends > 0; resends--) {
        // send group of unanswered packets
        int sent = 0;
        for (int i = 0; i < n_packets && sent < max_group; i++) {
            if (seqmap[i] == nullptr) continue;
            m->sock->write((char *)seqmap[i], seqmap[i]->hdr.len);
            sent++;
        }

        int unanswered_prev = unanswered;
        while (!m->sock->timedout() && unanswered) {  // read all incoming
            int len = m->sock->read((char *)&p, MAX_PACKET_SIZE);
            if (len != p.hdr.len || p.hdr.seqno < minseqno) continue;
            if (p.hdr.type == SOS && seqmap[p.hdr.seqno % n_packets]) {
                free(seqmap);
                return false;
            } else if (p.hdr.type == ACK && seqmap[p.hdr.seqno % n_packets]) {
                seqmap[p.hdr.seqno % n_packets] = nullptr;
                unanswered--;
            }
        }

        if (unanswered == 0) {
            free(seqmap);
            return true;
        } else if (made_progress(unanswered_prev, unanswered, m->nastiness))
            resends = MAX_RESEND_ATTEMPTS;  // earned more slack

        errp(
            "attempting resend after %d resends, after sending %d and reading "
            "in %d\n",
            resends, sent, unanswered_prev - unanswered);
    }

    free(seqmap);
    throw C150NetworkException("Network after MAX_RESEND_ATTEMPTS");
}

void transfer(files_t *fs, messenger_t *m) {
    int n_files = fs->files.size();
    int attempts = 0;
    for (int id = 0; id < n_files; attempts++) {  // <== NOT id++
        packet_t prep, *sections, check;
        files_topackets(fs, id, &prep, &sections, &check);
        if (
#ifndef JUST_END_TO_END
            filesend(&prep, sections, m) &&
#endif
            end2end(&check, id, m)) {
            attempts = 0;  // onto the next one
            id++;
        } else if (attempts >= MAX_SOS_COUNT)
            throw C150NETWORK::C150NetworkException(
                "Failed file transfer after MAX SOS COUNT");
    }
}

bool end2end(packet_t *check, int id, messenger_t *m) {
    bool endtoend = send(m, check, 1);
    packet_t p;
    if (endtoend)
        packet_keepit(&p, id);
    else
        packet_deleteit(&p, id);
    return send(m, &p, 1);
}

bool filesend(packet_t *prep_out, packet_t *sections_out, messenger_t *m) {
    bool success = send(m, prep_out, 1) &&
                   send(m, sections_out, prep_out->value.prep.nparts);
    free(sections_out);
    return success;
}
