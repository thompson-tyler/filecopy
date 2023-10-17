#ifndef FILECACHE_H
#define FILECACHE_H

#include <assert.h>
#include <openssl/sha.h>

#include <cstdlib>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "c150nastyfile.h"
#include "files.h"
#include "messenger.h"

struct entry_t {
    seq_t seqno;
    int n_parts;
    bool *recvparts;
    uint8_t *buffer;
    int buflen;
    bool verified;
    entry_t() {
        seqno = -1;
        n_parts = 0;
        recvparts = nullptr;
        buffer = nullptr;
        buflen = 0;
        verified = false;
    }
};

// The cache, purely exists as an optimization.
// Theoretically everything could work without it
// TODO: this should be a typedef
struct cache_t {
    unordered_map<int, entry_t> entries;
};

void cache_free(cache_t *cache);

// bounce :: packet_t -> packet_t
// Performs incoming packet action and constructs response packet in place.
void bounce(files_t *fs, cache_t *cache, packet_t *p, long count);

bool idempotent_prepareforfile(files_t *fs, cache_t *cache, fid_t id,
                               seq_t seqno, int lenght, int n_parts,
                               const char *filename);
bool idempotent_checkfile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                          const char *filename, const checksum_t checksum);
bool idempotent_storesection(files_t *fs, cache_t *cache, fid_t id, seq_t seqno,
                             int partno, int offset, int len,
                             const uint8_t *data);
bool idempotent_savefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno);
bool idempotent_deletefile(files_t *fs, cache_t *cache, fid_t id, seq_t seqno);

#endif
