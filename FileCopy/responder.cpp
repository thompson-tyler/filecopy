#include "responder.h"

#include "c150debug.h"

using namespace std;
using namespace C150NETWORK;

ServerResponder::ServerResponder(Filecache *cache) { m_cache = cache; }

// modifies packet in place
void ServerResponder::bounce(Packet *p) {
    seq_t seqno = p->hdr.seqno;
    const CheckIsNecessary *check;
    const PrepareForBlob *prep;
    const BlobSection *section;

    bool shouldAck = false;  // defaults always to SOS

    switch (p->hdr.type) {
        case SOS:
            shouldAck = false;
            break;
        case ACK:
            shouldAck = true;
            break;
        case KEEP_IT:
            shouldAck = m_cache->idempotentSaveFile(p->hdr.fid, seqno);
            break;
        case DELETE_IT:
            shouldAck = m_cache->idempotentDeleteTmp(p->hdr.fid, seqno);
            break;
        case CHECK_IS_NECESSARY:
            check = p->value.check;
            shouldAck = m_cache->idempotentCheckfile(
                p->hdr.fid, seqno, check->filename, check->checksum);
            break;
        case PREPARE_FOR_BLOB:
            prep = p->value.prep;
            shouldAck = m_cache->idempotentPrepareForFile(
                p->hdr.fid, seqno, prep->filename, prep->nparts);
            break;
        case BLOB_SECTION:
            section = p->value.section;
            shouldAck = m_cache->idempotentStoreFileChunk(
                p->hdr.fid, seqno, section->partno, section->data,
                section->datalen());
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

    Packet p;
    while (true) {
        unsigned long len = sock->read((char *)&p, sizeof(Packet));
        if (len != p.hdr.len) {
            c150debug->printf(C150APPLICATION,
                              "Received a packet with length %lu but expected "
                              "length was %d\n",
                              len, p.hdr.len);
            continue;
        }

        c150debug->printf(C150APPLICATION, "Read a packet!\n%s",
                          p.toString().c_str());
        // modify in place
        responder.bounce(&p);
        sock->write((const char *)&p, p.hdr.len);
        c150debug->printf(C150APPLICATION, "Responded!\n%s",
                          p.toString().c_str());
    }
}
