#ifndef FILECACHE_H
#define FILECACHE_H

#include <openssl/sha.h>

#include <cstdlib>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "c150nastyfile.h"
#include "message.h"
#include "messenger.h"

/***
 * Implementation requires a bit more consideration and thought
 */

class Filecache {
   public:
    Filecache(std::string dir, C150NETWORK::C150NastyFile *nfp);

    // no need to be careful about repeatedly calling these
    bool idempotentCheckfile(const std::string filename,
                             unsigned char checksum[SHA_DIGEST_LENGTH],
                             seq_t seqno);
    bool idempotentSaveFile(const std::string filename, seq_t seqno);
    bool idempotentDeleteTmp(const std::string filename, seq_t seqno);
    bool idempotentPrepareForFile(const std::string filename, seq_t seqno,
                                  uint32_t nparts);
    bool idempotentStoreFileChunk(const std::string filename, seq_t seqno,
                                  uint32_t partno, uint8_t *data, uint32_t len);

    // tell the filecache what to do
    // returns:
    //  zero => drop the packet
    //  pos  => respond with ack
    //  neg  => respond with SOS
    int tell(const Message *msg);

   private:
    enum FileStatus { PARTIAL, TMP, VERIFIED, SAVED };
    struct FileSection {
        uint32_t len;
        uint8_t *data;
        FileSection();
    };

    struct CacheEntry {
        // ordered by maturity
        FileStatus status;
        seq_t seqno;
        std::vector<FileSection> sections;
    };
    /*
     * filename -> ( FileStatus, min seq for redo, [ section1, NULL, ... ])
     */
    unordered_map<std::string, CacheEntry> m_cache;
    std::string m_dir;
    C150NETWORK::C150NastyFile *m_nfp;
};

#endif
