#include "cache.h"

#include <alloca.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "c150debug.h"
#include "files.h"
#include "utils.h"

using namespace C150NETWORK;
using namespace std;

// Hands packet off to the cache and executes idempotent action
// Also updates seqmax if necessary
void bounce(files_t *fs, cache_t *cache, packet_t *p) {
    bool rsp_ack = false;
    fid_t id = p->hdr.id;
    seq_t seqno = p->hdr.seqno;
    cache->seqmax = max(cache->seqmax, p->hdr.seqno);
    payload_u *value = &p->value;
    switch (p->hdr.type) {
        case SOS:
            break;
        case ACK:
            rsp_ack = true;
            break;
        case CHECK_IS_NECESSARY:
            rsp_ack = idempotent_checkfile(fs, cache, id, seqno,
                                           value->check.filename,
                                           value->check.checksum);
            break;
        case KEEP_IT:
            rsp_ack = idempotent_savefile(fs, cache, id, seqno);
            break;
        case DELETE_IT:
            rsp_ack = idempotent_deletefile(fs, cache, id, seqno);
            break;
        case PREPARE_FOR_BLOB:
            rsp_ack = idempotent_prepareforfile(
                fs, cache, id, seqno, value->prep.filelength,
                value->prep.nparts, value->prep.filename);
            break;
        case BLOB_SECTION:
            rsp_ack = idempotent_storesection(
                fs, cache, id, seqno, value->section.partno,
                value->section.offset, p->datalen(), value->section.data);
            break;
    }
    p->hdr.type = rsp_ack ? ACK : SOS;
    p->hdr.len = sizeof(p->hdr) + sizeof(p->value.seqmax);
    p->value.seqmax = cache->seqmax;
}

#define BAD false
#define OK true

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
                               seq_t seqno, int filelength, int n_parts,
                               const char *filename) {
    assert(fs && cache && filename);
    if (cache->entries[id].seqno > seqno)
        return BAD;
    else if (cache->entries[id].seqno == seqno)
        return OK;

    // got a brand new prepare!!
    if (cache->entries[id].recvparts == nullptr)
        files_register(fs, id, filename, true);

    // uses realloc in case we've already tried
    cache->entries[id].n_parts = n_parts;
    cache->entries[id].recvparts =
        (bool *)realloc(cache->entries[id].recvparts, sizeof(bool) * n_parts);
    cache->entries[id].buflen = filelength;
    cache->entries[id].buffer =
        (uint8_t *)realloc(cache->entries[id].buffer, filelength);
    for (int i = 0; i < n_parts; i++) cache->entries[id].recvparts[i] = false;

    // new seqno means we are prepared for file
    cache->entries[id].seqno = seqno;
    return OK;
}

bool idempotent_storesection(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                             int partno, int offset, int len,
                             const uint8_t *data) {
    assert(fs && cache && data);
    if (bad_seqno(cache, id, seqno) || partno >= cache->entries[id].n_parts ||
        cache->entries[id].recvparts == nullptr)
        return BAD;

    // already got it!
    if (cache->entries[id].recvparts[partno]) return OK;

    // write it down!
    memcpy(cache->entries[id].buffer + offset, data, len);
    cache->entries[id].recvparts[partno] = true;
    return OK;
}

bool idempotent_checkfile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                          const char *filename,
                          const unsigned char *checksum_in) {
    assert(fs && cache && checksum_in);

    // for files in the directory that never received a prepare msg
    if (cache->entries[id].seqno == -1)
        files_register(fs, id, filename, false);
    else if (cache->entries[id].seqno > seqno)
        return BAD;

    // BAD if missing parts, skipped for new files
    for (int i = 0; i < cache->entries[id].n_parts; i++)
        if (cache->entries[id].recvparts[i] == false) return BAD;

    // maybe we just checked it
    if (cache->entries[id].seqno == seqno)
        return cache->entries[id].verified ? OK : BAD;

    // Check that incoming checksum matches what's in the buffer
    checksum_t buf_checksum;
    SHA1(cache->entries[id].buffer, cache->entries[id].buflen, buf_checksum);
    if (memcmp(checksum_in, buf_checksum, SHA_DIGEST_LENGTH) != 0) return BAD;

    // Write to tmp file
    // Note this can fail
    if (!files_writetmp(fs, id, cache->entries[id].buflen,
                        cache->entries[id].buffer, checksum_in))
        return BAD;

    // Do check against contents of tmp file
    // This step is the end-to-end check. If this passes, then the file is on
    // disk and intact
    if (!verify_hash_tmp(fs, id, checksum_in)) return BAD;

    cache->entries[id].seqno = seqno;
    cache->entries[id].verified = true;

    return OK;
}

bool idempotent_savefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno) {
    assert(fs && cache);
    if (bad_seqno(cache, id, seqno) || !cache->entries[id].verified) return BAD;

    // just did it!
    if (cache->entries[id].seqno == seqno) return OK;

    errp("COMPLETED id: %d %s\n", id, fs->files[id].filename);
    files_savepermanent(fs, id);

    // new seqno means we have saved the file
    cache->entries[id].seqno = seqno;
    return OK;
}

bool idempotent_deletefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno) {
    assert(fs && cache);
    if (bad_seqno(cache, id, seqno) || cache->entries[id].verified) return BAD;

    // just did it!
    if (cache->entries[id].seqno == seqno) return OK;

    files_remove(fs, id);

    for (int i = 0; i < cache->entries[id].n_parts; i++)
        cache->entries[id].recvparts[i] = false;

    // new seqno means we have deleted the file
    cache->entries[id].seqno = seqno;
    return OK;
}
