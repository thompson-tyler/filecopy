
#include "messenger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "c150debug.h"
#include "c150network.h"
#include "files.h"
#include "packet.h"
#include "settings.h"

// TODO: factor in nastiness

bool made_progress(int prev, int curr, int nastiness) {
    (void)nastiness;
    return prev > curr;  // bare minimum
}

bool throttle(int nel, int nastiness) {
    (void)nastiness;
    return nel;
}

bool messenger_send(messenger_t *m, packet_t *packets, int n_packets) {
    // sanity checks
    assert(m && packets);
    if (n_packets == 0) return true;

    // initialize packets and seqmap
    packet_t **seqmap = (packet_t **)malloc(sizeof(&packets) * n_packets);
    int minseqno = m->global_seqcount;
    for (int i = 0; i < n_packets; i++) {
        packets[i].hdr.seqno = m->global_seqcount++;
        seqmap[packets[i].hdr.seqno % n_packets] = &packets[i];
    }

    packet_t p;
    int unanswered = n_packets;
    int max_group = throttle(MAX_SEND_GROUP, m->nastiness);
    for (int resends = MAX_RESEND_ATTEMPTS; resends > 0; resends--) {
        // send group of unanswered packets
        for (int i = 0, sent = 0; i < n_packets && sent < max_group; i++) {
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
    }

    throw C150NETWORK::C150NetworkException(
        "Network failed after many resend attempts");
}

void transfer(files_t *fs, messenger_t *m) {
    int n_files = fs->n_files;
    int attempts = 0;
    for (int id = 0; id < n_files; id++) {
        if (filesend(fs, id, m) && end2end(fs, id, m))
            attempts = 0;  // onto the next one
        else if (attempts >= MAX_SOS_COUNT)
            throw C150NETWORK::C150NetworkException(
                "Failed file transfer after MAX SOS COUNT");
        else {  // OR retry
            id--;
            attempts++;
        }
    }
}

bool end2end(files_t *fs, int id, messenger_t *m) {
    packet_t p;
    checksum_t checksum;
    files_calc_checksum(fs, id, checksum);
    packet_checkisnecessary(&p, id, fs->files[id].filename, checksum);
    bool endtoend = messenger_send(m, &p, 1);
    if (endtoend)
        packet_keepit(&p, id);
    else
        packet_deleteit(&p, id);
    return messenger_send(m, &p, 1);
}

bool filesend(files_t *fs, int id, messenger_t *m) {
    packet_t prep_out;
    packet_t *sections_out;
    int npackets = files_topackets(fs, id, &prep_out, &sections_out);
    bool success = messenger_send(m, &prep_out, 1) &&
                   messenger_send(m, sections_out, npackets);
    free(sections_out);
    return success;
}
