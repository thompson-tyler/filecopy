
#include "messenger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include <algorithm>
#include <cstdio>

#include "c150debug.h"
#include "c150grading.h"
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
    assert(m && packets);
    assert(n_packets > 0);

    // initialize tmp packets and seqmap
    int minseqno = m->seqcount;
    packet_t **seqmap = assign_sequences(m, packets, n_packets);
    packet_t p;
    int nasty = m->nastiness;

    int unanswered = n_packets;
    for (int resends = RESENDS(nasty); resends && unanswered; resends--) {
        // send as large a group as possible of remaining unanswered packets
        int sent = 0;
        for (int i = 0; i < n_packets && sent < SEND_GROUP(nastiness); i++) {
            if (seqmap[i] == nullptr) continue;  // already sent and received
            m->sock->write((char *)seqmap[i], seqmap[i]->hdr.len);
            sent++;
        }

        int unanswered_prev = unanswered;

        do {  // read incoming
            int len = m->sock->read((char *)&p, MAX_PACKET_SIZE);
            if (len != p.hdr.len || p.hdr.seqno < minseqno ||
                (p.hdr.type & (ACK | SOS)) == 0)
                continue;
            if (p.hdr.seqno > m->seqcount ||  // SOS or msg from future? abort!
                (p.hdr.type == SOS && seqmap[p.hdr.seqno % n_packets])) {
                *GRADING << "SOS received, aborting packet transfer" << endl;
                m->seqcount = max(m->seqcount, p.value.seqmax);  // goto future
                free(seqmap);
                return false;
            } else if (p.hdr.type == ACK && seqmap[p.hdr.seqno % n_packets]) {
                seqmap[p.hdr.seqno % n_packets] = nullptr;  // mark it recvd
                unanswered--;                               // we got one!
            }
        } while (!m->sock->timedout() && unanswered);  // keep em' comin

        int received = unanswered_prev - unanswered;

        // cut me some slack, im trying
        if (received) resends = RESENDS(nasty);
        errp(
            "%d remaining resends, %d remaining packets, after sending %d and "
            "catching %d\n",
            resends, unanswered, sent, received);
        // log when we lost packetsq
        if (received < sent) {
            *GRADING << "Didn't get ACKs for " << sent - received
                     << " packets, resending them." << endl;
        }
    }

    free(seqmap);
    if (unanswered) {
        *GRADING << "Server is not responding, aborting." << endl;
        throw C150NetworkException("Network failure\n");
    }
    return true;
}

void transfer(files_t *fs, messenger_t *m) {
    // Try to send all files
    // If one fails to send >= MAX_SOS times, it is skipped
    int attempts = 0;
    auto files = fs->files;
    for (auto it = files.begin(); it != files.end(); attempts++) {
        // it is a pair of (id, file_entry_t), so grab the id and filename
        int id = it->first;
        string filename = it->second.filename;
        *GRADING << "File: " << filename << ", beginning transmission, attempt "
                 << attempts + 1 << endl;

        // Generate all packets we need to transfer this sucker
        packet_t prep, *sections = nullptr, check;
        if (files_topackets(fs, id, &prep, &sections, &check) < 0) {
            *GRADING << "File: " << filename
                     << " failed to read, retrying "
                        "transfer."
                     << endl;
            continue;
        }
        if (filesend(&prep, sections, m)) {
            *GRADING << "File: " << filename
                     << " received, beginning end-to-end check" << endl;
            if (end2end(&check, id, m)) {
                *GRADING << "File: " << filename
                         << " end-to-end check succeeded" << endl;
                attempts = 0, it++;  // onto the next one
                continue;
            } else {
                *GRADING << "File: " << filename << " end-to-end check failed"
                         << endl;
            }
        }
        if (attempts >= MAX_SOS) {
            *GRADING << "File: " << filename
                     << " hit max number of failures. Skipping it." << endl;
            attempts = 0, it++;  // onto the next one
            continue;
        }
        errp("Got SOS trying again.\n");
    }
}

bool end2end(packet_t *check, int id, messenger_t *m) {
    errp("sending CHECK_IS_NECESSARY:\n%s", check->tostring().c_str());
    bool endtoend = send(m, check, 1);
    if (endtoend)
        packet_keepit(check, id);
    else
        packet_deleteit(check, id);
    endtoend ? errp("sent KEEP_IT\n") : errp("sent DELETE_IT\n");
    return endtoend && send(m, check, 1);
}

bool filesend(packet_t *prep_out, packet_t *sections_out, messenger_t *m) {
    errp("sending PREPARE && SECTIONS:\n%s", prep_out->tostring().c_str());
    bool succ = send(m, prep_out, 1);
    if (!succ) {
        *GRADING << "Received SOS on prepare file, retrying transfer." << endl;
        errp("RECEIVED SOS ON PREPARE FILE\n");
    } else
        succ = send(m, sections_out, prep_out->value.prep.nparts);
    if (!succ) {
        *GRADING
            << "Received SOS when sending file sections, retrying transfer."
            << endl;
        errp("RECEIVED SOS ON SECTIONS\n");
    }
    free(sections_out);
    return succ;
}
