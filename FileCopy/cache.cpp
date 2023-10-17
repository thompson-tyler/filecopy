#include "cache.h"

#include <alloca.h>
#include <openssl/sha.h>

#include <cstdio>
#include <cstdlib>

#include "c150debug.h"
#include "files.h"
#include "utils.h"

using namespace C150NETWORK;
using namespace std;

// bounce :: packet_t -> packet_t
// Performs incoming packet action and constructs response packet in place.
void bounce(files_t *fs, cache_t *cache, packet_t *p, long count) {
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
                fs, cache, id, seqno, value->prep.filelength,
                value->prep.nparts, value->prep.filename);
            break;
        case BLOB_SECTION:
            is_ack = idempotent_storesection(
                fs, cache, id, seqno, value->section.partno,
                value->section.offset, p->datalen(), value->section.data);
            break;
    }
    p->hdr.type = is_ack ? ACK : SOS;
    p->hdr.len = sizeof(p->hdr);
    p->value.total.totalcount = count;
}

#define SOS false
#define ACK true

bool bad_seqno(cache_t *c, int id, int seqno) {
    return c->entries[id].seqno == -1 || c->entries[id].seqno > seqno;
}

void cache_free(cache_t *cache) {
    assert(cache);
    for (auto it : cache->entries) {
        free(it.second.recvparts);
        free(it.second.buffer);
    };
}

bool idempotent_prepareforfile(files_t *fs, cache_t *cache, fid_t id,
                               seq_t seqno, int length, int n_parts,
                               const char *filename) {
    assert(fs && cache);
    if (cache->entries[id].seqno > seqno)
        return SOS;
    else if (cache->entries[id].seqno == seqno)
        return ACK;

    // got a brand new prepare!!
    if (cache->entries[id].recvparts == nullptr)
        files_register(fs, id, filename, true);

    cache->entries[id].n_parts = n_parts;
    // realloc in case we've already tried
    cache->entries[id].recvparts =
        (bool *)realloc(cache->entries[id].recvparts, sizeof(bool) * n_parts);
    cache->entries[id].buflen = length;
    cache->entries[id].buffer =
        (uint8_t *)realloc(cache->entries[id].buffer, length);
    for (int i = 0; i < n_parts; i++) cache->entries[id].recvparts[i] = false;

    // new seqno means we are prepared for file
    cache->entries[id].seqno = seqno;
    return ACK;
}

bool idempotent_storesection(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                             int partno, int offset, int len,
                             const uint8_t *data) {
    assert(fs && cache);
    if (bad_seqno(cache, id, seqno) || partno >= cache->entries[id].n_parts ||
        cache->entries[id].recvparts == nullptr)
        return SOS;

    // already got it!
    if (cache->entries[id].recvparts[partno]) return ACK;

    // write it down!
    memcpy(cache->entries[id].buffer + offset, data, len);
    cache->entries[id].recvparts[partno] = true;
    return ACK;
}

bool idempotent_checkfile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                          const char *filename,
                          const unsigned char *checksum_in) {
    assert(fs && cache);

    // for files in the directory that never received a prepare msg
    if (cache->entries[id].seqno == -1)
        files_register(fs, id, filename, false);
    else if (cache->entries[id].seqno > seqno)
        return SOS;

    // SOS if missing parts, skipped for new files
    for (int i = 0; i < cache->entries[id].n_parts; i++)
        if (cache->entries[id].recvparts[i] == false) return SOS;

    // maybe we just checked it
    if (cache->entries[id].seqno == seqno)
        return cache->entries[id].verified ? ACK : SOS;

    // do check
    checksum_t checksum;
    SHA1(cache->entries[id].buffer, cache->entries[id].buflen, checksum);
    bool success = memcmp(checksum, checksum_in, SHA_DIGEST_LENGTH) == 0;

    // writing can still trigger SOS
    if (success) {
        success = files_writetmp(fs, id, cache->entries[id].buflen,
                                 cache->entries[id].buffer, checksum);
    }
    cache->entries[id].verified = success;
    errp("CHECK SUCCEEDED: %s\n", success ? "TR" : "FL");

    // new seqno means we know the answer of end2end check
    cache->entries[id].seqno = seqno;

    return success ? ACK : SOS;
}

bool idempotent_savefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno) {
    assert(fs && cache);
    if (bad_seqno(cache, id, seqno) || !cache->entries[id].verified) return SOS;

    // just did it!
    if (cache->entries[id].seqno == seqno) return ACK;

    errp("COMPLETED id: %d %s\n", id, fs->files[id].filename);
    files_savepermanent(fs, id);

    // new seqno means we have saved the file
    cache->entries[id].seqno = seqno;
    return ACK;
}

bool idempotent_deletefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno) {
    assert(fs && cache);
    if (bad_seqno(cache, id, seqno) || cache->entries[id].verified) return SOS;

    // just did it!
    if (cache->entries[id].seqno == seqno) return ACK;

    files_remove(fs, id);

    for (int i = 0; i < cache->entries[id].n_parts; i++)
        cache->entries[id].recvparts[i] = false;

    // new seqno means we have deleted the file
    cache->entries[id].seqno = seqno;
    return ACK;
}

#undef ACK
#undef SOS
