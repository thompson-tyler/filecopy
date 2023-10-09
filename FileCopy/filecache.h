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

    // No need to be careful about repeatedly calling these

    // responds SOS if file incomplete, malformed
    bool idempotentCheckfile(int id, seq_t seqno, const std::string filename,
                             const unsigned char checksum[SHA_DIGEST_LENGTH]);

    // responds SOS if file incomplete, malformed or not yet mentioned
    bool idempotentSaveFile(int id, seq_t seqno);

    // responds SOS if file already verified as correct and saved to disk
    bool idempotentDeleteTmp(int id, seq_t seqno);

    // always ACK
    bool idempotentPrepareForFile(int id, seq_t seqno,
                                  const std::string filename, uint32_t nparts);

    // responds SOS if file is not yet mentioned
    bool idempotentStoreFileChunk(int id, seq_t seqno, uint32_t partno,
                                  uint8_t *data, uint32_t len);

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
    std::unordered_map<int, CacheEntry> m_cache;
    std::unordered_map<int, std::string> m_idmap;

    std::string m_dir;
    C150NETWORK::C150NastyFile *m_nfp;
};

#endif
