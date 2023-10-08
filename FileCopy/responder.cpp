#include "responder.h"

#include "message.h"

using namespace std;
using namespace C150NETWORK;

ServerResponder::ServerResponder(Filecache *cache) { m_cache = cache; }

// m
void ServerResponder::bounce(Packet *p) {
    Message m = Message(p);
    seq_t seqno = p->hdr.seqno;

    // defaults always to SOS
    bool shouldAck = false;

    switch (m.type()) {
        case SOS:
            shouldAck = false;
            break;
        case ACK:
            shouldAck = true;
            break;
        case KEEP_IT:
            shouldAck = m_cache->idempotentSaveFile(m.filename(), seqno);
            break;
        case DELETE_IT:
            shouldAck = m_cache->idempotentDeleteTmp(m.filename(), seqno);
            break;
        case CHECK_IS_NECESSARY:
            shouldAck = m_cache->idempotentCheckfile(
                m.filename(), seqno, m.getCheckIsNecessary()->checksum);
            break;
        case PREPARE_FOR_BLOB:
            shouldAck = m_cache->idempotentPrepareForFile(
                m.filename(), seqno, m.getPrepareForBlob()->nparts);
        case BLOB_SECTION:
            const BlobSection *section = m.getBlobSection();
            shouldAck = m_cache->idempotentStoreFileChunk(
                m.filename(), seqno, section->partno, section->data,
                section->len);
            break;
    }

    // pretty simple, since filename is always in the same place
    p->hdr.type = shouldAck ? ACK : SOS;
}

// For the time being, the listener responds to every packet.
// Anything else is an optimization that shouldn't be made prematurely.
void listen(C150DgmSocket *sock, C150NastyFile *nfp, string dir) {
    Filecache cache = Filecache(dir, nfp);
    ServerResponder responder = ServerResponder(&cache);

    uint8_t *buffer = (uint8_t *)malloc(MAX_PACKET_SIZE + 20);
    Packet p;

    while (true) {
        unsigned long len = sock->read((char *)buffer, MAX_PACKET_SIZE);
        if (len > MAX_PACKET_SIZE || len < sizeof(Packet)) {
            fprintf(stderr, "read a short packet strange");
            continue;
        }
        p = Packet(buffer);
        responder.bounce(&p);
        p.toBuffer(buffer);
        sock->write((const char *)buffer, p.totalSize());
    }

    free(buffer);
}
