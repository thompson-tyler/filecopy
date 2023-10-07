#ifndef FILECACHE_H
#define FILECACHE_H

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
    // returns true if needs SOS, this function is idempotent — re-receiving an
    // old message will never cause harm
    bool tell(const Message *msg);

   private:
    struct FileStatus {
        enum { FileGood, FileBad, FileWaiting } status;
        union {
            int n_parts;
        };
    };

    // no need to be careful about repeatedly calling these
    bool idempotentFileIsGood(const std::string filename, seqNum_t seqno);
    void idempotentKeepBlob(const std::string id, seqNum_t seqno);
    void idempotentDeleteBlob(const std::string id, seqNum_t seqno);
    void idempotentPrepareForBlob(const std::string id, ssize_t nparts,
                                  seqNum_t seqno);
    void idempotentStoreBlobChunk(const std::string id, ssize_t partno,
                                  char *data, uint32_t numBytes);

    /*************************************************************************
     * filename -> ( FileStatus, min seq for redo, [ filecontents1, ... ] )  *
     *************************************************************************/
    unordered_map<std::string,
                  std::tuple<FileStatus, seqNum_t, std::vector<char *>>>
        m_cache;
};

#endif
