#ifndef RESPONDER_H
#define RESPONDER_H

#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "filecache.h"
#include "packet.h"

class ServerResponder {
   public:
    ServerResponder(Filecache *m_cache);

    // Given a packet ALWAYS responds either ACK or SOS with matching seqno
    // — hands off the incoming packet to the Filecache to do idempotent stuff
    //
    // modifies packet IN PLACE by changing type to ACK or SOS
    // so can safely get the filename and seqno back from ACK or SOS
    void bounce(Packet *p);

   private:
    Filecache *m_cache;
};

// main server call
void listen(C150NETWORK::C150DgmSocket *sock, C150NETWORK::C150NastyFile *nfp,
            std::string dir);

#endif
