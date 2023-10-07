#ifndef FILECACHE_H
#define FILECACHE_H

#include <_types/_uint8_t.h>

#include <tuple>
#include <unordered_map>
#include <vector>

#include "message.h"
#include "messenger.h"

/***
 * Implementation requires a bit more consideration and thought
 */

class Filecache {
   public:
    // tell the filecache what to do
    // returns:
    //  zero => drop the packet
    //  pos  => respond with ack
    //  neg  => respond with SOS
    int tell(const Message *msg);

   private:
    struct FileStatus {
        enum { Verified, Tmp, Partial } status;
        union {
            uint8_t *expected_checksum[SHA_DIGEST_LENGTH];
        } meta;
    };

    // no need to be careful about repeatedly calling these
    bool idempotentFileIsGood(const std::string filename, seq_t seqno);
    void idempotentSaveFile(const std::string filename, seq_t seqno);
    void idempotentDeleteTmp(const std::string filename, seq_t seqno);
    void idempotentPrepareForFile(const std::string filename, seq_t seqno,
                                  ssize_t nparts);
    void idempotentStoreBlobChunk(const std::string filename, seq_t seqno,
                                  ssize_t partno, uint8_t *data, uint32_t len);

    /*************************************************************************
     * filename -> ( FileStatus, min seq for redo, [ filecontents1, ... ] )  *
     *************************************************************************/
    unordered_map<std::string,
                  std::tuple<FileStatus, seq_t, std::vector<uint8_t *>>>
        m_cache;
};

#endif
