#include "responder.h"

#include "c150debug.h"
#include "message.h"

using namespace std;
using namespace C150NETWORK;

ServerResponder::ServerResponder(Filecache *cache) { m_cache = cache; }

// modifies packet in place
void ServerResponder::bounce(Packet *p) {
    Message m = Message(p);
    seq_t seqno = p->hdr.seqno;
    const CheckIsNecessary *check;
    const PrepareForBlob *prep;
    const BlobSection *section;

    bool shouldAck = false;  // defaults always to SOS

    switch (m.type()) {
        case SOS:
            shouldAck = false;
            break;
        case ACK:
            shouldAck = true;
            break;
        case KEEP_IT:
            shouldAck = m_cache->idempotentSaveFile(m.id(), seqno);
            break;
        case DELETE_IT:
            shouldAck = m_cache->idempotentDeleteTmp(m.id(), seqno);
            break;
        case CHECK_IS_NECESSARY:
            check = m.getCheckIsNecessary();
            shouldAck = m_cache->idempotentCheckfile(
                m.id(), seqno, check->filename, check->checksum);
            break;
        case PREPARE_FOR_BLOB:
            prep = m.getPrepareForBlob();
            shouldAck = m_cache->idempotentPrepareForFile(
                m.id(), seqno, prep->filename, prep->nparts);
        case BLOB_SECTION:
            section = m.getBlobSection();
            shouldAck = m_cache->idempotentStoreFileChunk(
                m.id(), seqno, section->partno, section->data, section->len);
            break;
    }
    c150debug->printf(C150APPLICATION, "Going to %s!\n",
                      shouldAck ? "ACK" : "SOS");

    // pretty simple, modify incoming packet hdr in place
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
        unsigned long len = sock->read((char *)buffer, sizeof(Packet));

        if (len > MAX_PACKET_SIZE || len < sizeof(Header)) {
            fprintf(stderr, "read an incorrectly sized packet, strange");
            continue;
        }

        p = Packet(buffer);
        c150debug->printf(C150APPLICATION, "Read a packet!\n%s",
                          p.toString().c_str());
        responder.bounce(&p);
        p.toBuffer(buffer);
        sock->write((const char *)buffer, p.totalSize());
        c150debug->printf(C150APPLICATION, "Responded!\n%s",
                          p.toString().c_str());
    }

    free(buffer);
}
