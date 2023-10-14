#include "cache.h"

#include <cstdio>
#include <cstdlib>

#include "c150debug.h"
#include "files.h"

using namespace C150NETWORK;
using namespace std;

// bounce :: packet_t -> packet_t
// Performs incoming packet action and constructs response packet in place.
void bounce(files_t *fs, cache_t *cache, packet_t *p) {
    bool is_ack = false;
    fid_t id = p->hdr.id;
    seq_t seqno = p->hdr.seqno;
    payload_u *value = &p->value;
    switch (p->hdr.type) {
        case SOS:
        case ACK:
            return;
        case CHECK_IS_NECESSARY:
            is_ack = idempotent_checkfile(fs, cache, id, seqno,
                                          value->check.filename,
                                          value->check.checksum);
            break;
        case KEEP_IT:
            is_ack = idempotent_savefile(fs, cache, id, seqno);
            break;
        case DELETE_IT:
            is_ack = idempotent_deletefile(fs, cache, id, seqno);
            break;
        case PREPARE_FOR_BLOB:
            is_ack = idempotent_prepareforfile(
                fs, cache, id, seqno, value->prep.nparts, value->prep.filename);
            break;
        case BLOB_SECTION:
            is_ack = idempotent_storesection(
                fs, cache, id, seqno, value->section.partno,
                value->section.start, p->datalen(), value->section.data);
            break;
    }
    p->hdr.type = is_ack ? ACK : SOS;
    p->hdr.len = sizeof(p->hdr);
}

#define SOS false
#define ACK true

struct entry_t {
    seq_t seqno;
    int n_parts;
    bool *recvparts;
    bool verified;
};

void entry_init(entry_t *e) {
    assert(e);
    e->seqno = -1;
    e->n_parts = 0;
    e->recvparts = nullptr;
    e->verified = false;
}

// initializes size to 40 files
cache_t *cache_new() {
    cache_t *c = (cache_t *)malloc(sizeof(*c));
    assert(c);
    c->cap = 40;
    c->nel = 0;
    c->entries = (entry_t *)malloc(sizeof(*c->entries) * c->cap);
    assert(c->entries);
    for (int i = 0; i < c->cap; i++) entry_init(&c->entries[i]);
    return c;
}

void cache_free(cache_t **cache_pp) {
    assert(cache_pp && *cache_pp);
    cache_t *c = *cache_pp;
    for (int i = 0; i < c->nel; i++) free(c->entries->recvparts);
    free(c->entries);
    *cache_pp = nullptr;
}

void cache_resize_if_necessary(cache_t *cache, int id) {
    if (cache->cap < id) {  // add new space for ids
        int newcap = cache->cap / 2;
        cache->entries = (entry_t *)realloc(cache->entries,
                                            sizeof(*cache->entries) * newcap);
        assert(cache->entries);
        for (int i = cache->cap; i < newcap; i++)
            entry_init(&cache->entries[i]);
        cache->cap = newcap;
    }
}

bool idempotent_prepareforfile(files_t *fs, cache_t *cache, fid_t id,
                               seq_t seqno, int n_parts, const char *filename) {
    assert(fs && cache);
    cache_resize_if_necessary(cache, id);
    if (cache->entries[id].seqno > seqno) return SOS;
    if (cache->entries[id].seqno == seqno) return ACK;

    // got a new prepare!!
    files_register(fs, id, filename);
    // realloc in case we've already tried
    cache->entries[id].recvparts =
        (bool *)realloc(cache->entries[id].recvparts, sizeof(bool) * n_parts);
    cache->entries[id].n_parts = n_parts;
    for (int i = 0; i < n_parts; i++) cache->entries[id].recvparts[i] = false;

    // new seqno means we are prepared for file
    cache->entries[id].seqno = seqno;
    return ACK;
}

bool idempotent_storesection(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                             int partno, int offset, int len,
                             const char *data) {
    assert(fs && cache);
    if (id > cache->nel || cache->entries[id].seqno > seqno ||
        partno >= cache->entries[id].n_parts)
        return SOS;

    // already got it!
    if (cache->entries[id].recvparts[partno]) return ACK;

    files_storetmp(fs, id, offset, len, data);
    cache->entries[id].recvparts[partno] = true;
    return ACK;
}

bool idempotent_checkfile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                          const char *filename,
                          const unsigned char *checksum_in) {
    assert(fs && cache);
    if (id > cache->nel || cache->entries[id].seqno > seqno) return SOS;

    // SOS if missing parts
    for (int i = 0; i < cache->entries[id].n_parts; i++)
        if (!cache->entries[id].recvparts[i]) return SOS;

    // maybe we just checked it
    if (cache->entries[id].seqno == seqno)
        return cache->entries[id].verified ? ACK : SOS;

    // do check
    cache->entries[id].verified = files_checktmp(fs, id, checksum_in);

    // new seqno means we know the answer of end2end check
    cache->entries[id].seqno = seqno;
    return cache->entries[id].verified ? ACK : SOS;
}

bool idempotent_savefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno) {
    assert(fs && cache);
    if (id > cache->nel || cache->entries[id].seqno > seqno ||
        !cache->entries[id].verified)
        return SOS;

    // just did it!
    if (cache->entries[id].seqno == seqno) return ACK;

    files_savepermanent(fs, id);

    // new seqno means we have saved the file
    cache->entries[id].seqno = seqno;
    return ACK;
}

bool idempotent_deletefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno) {
    assert(fs && cache);
    if (id > cache->nel || cache->entries[id].seqno > seqno ||
        cache->entries[id].verified)
        return SOS;

    // just did it!
    if (cache->entries[id].seqno == seqno) return ACK;

    files_remove(fs, id);
    free(cache->entries[id].recvparts);
    entry_init(&cache->entries[id]);

    // new seqno means we have deleted the file
    cache->entries[id].seqno = seqno;
    return ACK;
}
