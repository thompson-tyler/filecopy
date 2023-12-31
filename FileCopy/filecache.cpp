#include "filecache.h"

#include <cstdio>

#include "c150debug.h"
#include "diskio.h"

using namespace C150NETWORK;
using namespace std;

/*
 *  STATUS   | SEQNO WAS LAST SET WHEN
 *  -------------------------------------------------------------------
 *  PARTIAL  | received a prepareForFile since last NON-PARTIAL
 *  TMP      | final section was received
 *           OR
 *           | last attempted to verify the file
 *  VERIFIED | when the file was first correctly verified
 *  SAVED    | when the file was first saved
 *
 * */

#define ACK true
#define SOS false

// small readability adjustment

Filecache::Filecache(string dir, C150NastyFile *nfp) {
    m_dir = dir;
    m_nfp = nfp;
}

// returns true if file is good
bool Filecache::filecheck(string filename, const checksum_t checksum) {
    uint8_t *buffer = nullptr;
    uint8_t diskChecksum[SHA_DIGEST_LENGTH];

    // fileToBuffer guarantees no nastyfile issues
    int len = fileToBuffer(m_nfp, filename, &buffer, diskChecksum);

    if (len == -1) return SOS;  // file doesn't exist

    free(buffer);
    return (memcmp(diskChecksum, checksum, SHA_DIGEST_LENGTH)) ? SOS : ACK;
}

// no need to be careful about repeatedly calling these
bool Filecache::idempotentCheckfile(int id, seq_t seqno, const string filename,
                                    const checksum_t checksum) {
    // If we haven't heard of this ID, try to perform the check on the filename
    // anyway. This is kind of a hack that lets us verify pre-existing files.
    if (!m_cache.count(id)) {
        cerr << "got filecheck for unknown id " << id << endl;
        vector<FileSegment> nosegments;
        // cache if success (note we check the actual file not .tmp)
        if (filecheck(makeFileName(m_dir, filename), checksum)) {
            m_cache[id] = {FileStatus::SAVED, seqno, filename, nosegments};
            return ACK;
        }
        // move to tmp file
        m_cache[id] = {FileStatus::TMP, seqno, filename, nosegments};
        rename(makeFileName(m_dir, filename).c_str(),
               makeTmpFileName(m_dir, filename).c_str());
        return SOS;
    }

    auto &entry = m_cache[id];
    cerr << "doing check for " << entry.filename << endl;
    switch (entry.status) {
        case FileStatus::PARTIAL:  // something is wrong
            cerr << "failed to check file " << filename
                 << " because it has not finished transferring" << endl;
            return SOS;
        case FileStatus::TMP:
            // We have already done the check, but it is still in TMP,
            // Instead, if it's an old message the client can figure it out
            if (seqno <= entry.seqno) {
                cerr << "failed to check file " << filename
                     << " because it has an old seq number" << endl;
                return SOS;
            }

            // During TMP, seqno is always the most recent filecheck, or when
            // file was finished
            entry.seqno = seqno;

            // check the .tmp file
            if (!filecheck(makeTmpFileName(m_dir, filename), checksum)) {
                cerr << "failed to check file " << filename
                     << " because checksums didn't match" << endl;
                return SOS;
            }
            entry.status = FileStatus::VERIFIED;  // check success
            return ACK;
        default:  // if already verified or saved
            return ACK;
    }
}

bool Filecache::idempotentSaveFile(int id, seq_t seqno) {
    if (!m_cache.count(id)) return SOS;
    CacheEntry &entry = m_cache[id];
    switch (entry.status) {
        case FileStatus::VERIFIED:
            rename(makeTmpFileName(m_dir, m_cache[id].filename).c_str(),
                   makeFileName(m_dir, m_cache[id].filename).c_str());
            entry.status = FileStatus::SAVED;
            return ACK;
        case FileStatus::SAVED:
            return ACK;
        default:  // if TMP or PARTIAL, old messages will be ignored by client
            return SOS;
    }
    return ACK;
}

bool Filecache::idempotentDeleteTmp(int id, seq_t seqno) {
    if (!m_cache.count(id)) return SOS;
    CacheEntry &entry = m_cache[id];
    switch (entry.status) {
        case FileStatus::PARTIAL:
            return ACK;
        case FileStatus::TMP:
            remove(makeTmpFileName(m_dir, m_cache[id].filename).c_str());
            entry.seqno = seqno;
            entry.deleteSections();
            entry.status = FileStatus::PARTIAL;
            return ACK;
        case FileStatus::VERIFIED:
            return SOS;
        case FileStatus::SAVED:
            return SOS;
    }
    return ACK;
}

bool Filecache::idempotentPrepareForFile(int id, seq_t seqno,
                                         const string filename,
                                         uint32_t nparts) {
    // Make a new empty registry for the file in the cache
    // The case we want to do this for is that either
    // 1. It doesn't already exist
    // OR
    // 2. The status is PARTIAL and a **NEW** message tells us to start over
    if (!m_cache.count(id) || m_cache[id].seqno < seqno) {
        c150debug->printf(C150APPLICATION,
                          "preparing for file %s, id %d with %d parts.\n",
                          filename.c_str(), id, nparts);
        // free any old date in the event this is a retransmission
        m_cache[id].deleteSections();
        // Build a new file cache entry
        vector<FileSegment> sections(nparts);
        CacheEntry entry = {FileStatus::PARTIAL, seqno, filename, sections};
        m_cache[id] = entry;
    }
    return ACK;
}

bool Filecache::idempotentStoreFileChunk(int id, seq_t seqno, uint32_t partno,
                                         uint8_t *data, uint32_t len) {
    if (!m_cache.count(id)) return SOS;

    CacheEntry &entry = m_cache[id];
    if (entry.status == FileStatus::PARTIAL) {
        c150debug->printf(
            C150APPLICATION,
            "Trying to insert %d bytes in section %d of cache %d.\n", len,
            partno, id);
        // add fresh section, if seqno is more recent than when file was first
        // announced
        if (entry.seqno < seqno && entry.sections[partno].data == nullptr) {
            entry.sections[partno].len = len;
            entry.sections[partno].data = (uint8_t *)malloc(len);
            memcpy(entry.sections[partno].data, data, len);
        }

        // if any sections are still null, we are still partial
        for (auto &section : entry.sections)
            if (section.data == nullptr) return ACK;

        // if no sections are null, move to TMP
        partialToTemp(id);
        entry.seqno = seqno;  // for TMP entries, seqno is the most recent
                              // filecheck or when file was finished
    }
    return ACK;
}

uint32_t Filecache::joinBuffers(vector<FileSegment> fs, uint8_t **buffer_pp) {
    uint32_t sumlen = 0;
    for (auto s : fs) sumlen += s.len;

    uint8_t *buffer = (uint8_t *)malloc(sumlen);

    sumlen = 0;
    for (auto s : fs) {
        memcpy(buffer + sumlen, s.data, s.len);
        sumlen += s.len;
    }

    *buffer_pp = buffer;
    return sumlen;
}

void Filecache::partialToTemp(int id) {
    CacheEntry &entry = m_cache[id];
    entry.status = FileStatus::TMP;

    // Move section data to a buffer
    uint8_t *buffer = nullptr;
    uint32_t buflen = joinBuffers(m_cache[id].sections, &buffer);
    m_cache[id].deleteSections();

    // Move buffer to a tmp file
    string tmpfile = makeTmpFileName(m_dir, m_cache[id].filename);
    touch(m_nfp, tmpfile);
    bufferToFile(m_nfp, tmpfile, buffer, buflen);
    free(buffer);
}

void Filecache::CacheEntry::deleteSections() {
    for (auto &section : sections) {
        free(section.data);
        section.data = nullptr;
        section.len = 0;
    }
}
