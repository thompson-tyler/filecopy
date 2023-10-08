#ifndef FILECACHE_H
#define FILECACHE_H

#include <openssl/sha.h>

#include <cstdlib>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "c150nastyfile.h"
#include "messenger.h"

/***
 * Implementation requires a bit more consideration and thought
 */

class Filecache {
   public:
    Filecache(std::string dir, C150NETWORK::C150NastyFile *nfp);

    // no need to be careful about repeatedly calling these
    bool idempotentCheckfile(const std::string filename, seq_t seqno,
                             const unsigned char checksum[SHA_DIGEST_LENGTH]);
    bool idempotentSaveFile(const std::string filename, seq_t seqno);
    bool idempotentDeleteTmp(const std::string filename, seq_t seqno);
    bool idempotentPrepareForFile(const std::string filename, seq_t seqno,
                                  uint32_t nparts);
    bool idempotentStoreFileChunk(const std::string filename, seq_t seqno,
                                  uint32_t partno, uint8_t *data, uint32_t len);

   private:
    bool filecheck(string filename,
                   const unsigned char checksum[SHA_DIGEST_LENGTH]);

    enum FileStatus { PARTIAL, TMP, VERIFIED, SAVED };
    struct FileSegment {
        uint32_t len = 0;
        uint8_t *data = nullptr;
    };

    struct CacheEntry {
        // ordered by maturity
        FileStatus status;
        seq_t seqno;
        std::vector<FileSegment> sections;
    };

    uint32_t joinBuffers(vector<FileSegment> fs, uint8_t **buffer);

    /*
     * filename -> ( FileStatus, min seq for redo, [ section1, NULL, ... ])
     */
    unordered_map<std::string, CacheEntry> m_cache;
    std::string m_dir;
    C150NETWORK::C150NastyFile *m_nfp;
};

#endif
