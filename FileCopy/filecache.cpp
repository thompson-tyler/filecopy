#include "filecache.h"

#include "diskio.h"

using namespace C150NETWORK;

/*
 *  STATE    | WHEN SEQNO WAS LAST SET
 *  -----------------------------------------------------
 *  PARTIAL  | first received prepareForFile since PARTIAL
 *  TMP      | all sections were completed
 *             -then-> the when the last attempt to verify the file was
 *  VERIFIED | when the file was first correctly verified
 *  SAVED    | when the file was first saved
 * */

#define ACK true
#define SOS false

Filecache::Filecache(std::string dir, C150NETWORK::C150NastyFile *nfp) {
    m_dir = dir;
    m_nfp = nfp;
}

Filecache::FileSection::FileSection() {
    data = nullptr;
    len = 0;
}

// no need to be careful about repeatedly calling these
bool Filecache::idempotentCheckfile(const std::string filename,
                                    unsigned char checksum[SHA_DIGEST_LENGTH],
                                    seq_t seqno) {
    if (!m_cache.count(filename)) return SOS;
    auto &entry = m_cache[filename];
    switch (entry.status) {
        case FileStatus::PARTIAL:
            return SOS;
        case FileStatus::TMP:
            // We have already done the check, but it is still in TMP
            // If it's an old message the client can figure it out
            if (seqno <= entry.seqno) return SOS;
            // During TMP, seqno is always the most recent check
            entry.seqno = seqno;
            // Do check
            uint8_t *buffer;
            uint8_t diskChecksum[SHA_DIGEST_LENGTH];
            fileToBuffer(m_nfp, m_dir, filename, &buffer, diskChecksum);
            free(buffer);  // ignore the buffer for now
            if (memcmp(diskChecksum, checksum,
                       SHA_DIGEST_LENGTH))  // failed
                return SOS;
            entry.status = FileStatus::VERIFIED;
            return ACK;
        default:  // if verified or saved
            return ACK;
    }
}

bool Filecache::idempotentSaveFile(const std::string filename, seq_t seqno) {
    if (!m_cache.count(filename)) return SOS;
    auto &entry = m_cache[filename];
    switch (entry.status) {
        case FileStatus::PARTIAL:
            return SOS;
        case FileStatus::TMP:
            return SOS;
        case FileStatus::VERIFIED:
            rename(makeTmpFileName(m_dir, filename).c_str(),
                   makeFileName(m_dir, filename).c_str());
            entry.status = FileStatus::SAVED;
            return ACK;
        case FileStatus::SAVED:
            return ACK;
    }
    return ACK;
}

bool Filecache::idempotentDeleteTmp(const std::string filename, seq_t seqno) {
    if (!m_cache.count(filename)) return SOS;
    auto &entry = m_cache[filename];
    switch (entry.status) {
        case FileStatus::PARTIAL:
            return ACK;
        case FileStatus::TMP:
            remove(makeTmpFileName(m_dir, filename).c_str());
            entry.seqno = seqno;
            for (auto &section : entry.sections) {
                free(section.data);
                section.data = nullptr;
                section.len = 0;
            }
            entry.status = FileStatus::PARTIAL;
            return ACK;
        case FileStatus::VERIFIED:
            return SOS;
        case FileStatus::SAVED:
            return SOS;
    }
    return ACK;
}

bool Filecache::idempotentPrepareForFile(const std::string filename,
                                         seq_t seqno, uint32_t nparts) {
    // make a new registry for the file in the cache
    if (!m_cache.count(filename) ||          // either doesn't exist or
        (m_cache[filename].seqno < seqno &&  // is partial and a NEW message
                                             // is telling us to restart
         m_cache[filename].status == FileStatus::PARTIAL)) {
        vector<FileSection> sections(nparts);
        CacheEntry entry = {FileStatus::PARTIAL, seqno, sections};
        m_cache[filename] = entry;
        for (auto &section : m_cache[filename].sections) {
            free(section.data);
            section.data = nullptr;
            section.len = 0;
        }
    }

    return ACK;
}

bool Filecache::idempotentStoreFileChunk(const std::string filename,
                                         seq_t seqno, uint32_t partno,
                                         uint8_t *data, uint32_t len) {
    if (!m_cache.count(filename)) {
        return SOS;
    }

    auto &entry = m_cache[filename];
    switch (entry.status) {
        case FileStatus::PARTIAL:
            if (entry.seqno < seqno && entry.sections[partno].data == nullptr) {
                free(entry.sections[partno].data = data);
                entry.sections[partno].data = data;
                entry.sections[partno].len = len;
            }

            // if any sections are still null, we are still partial
            for (auto &section : entry.sections)
                if (section.data == nullptr) return ACK;

            // if we get here, we have all the sections
            entry.status = FileStatus::TMP;
        default:
            return ACK;
    }
}
