#ifndef RESPONDER_H
#define RESPONDER_H

#include "c150dgmsocket.h"
#include "c150nastydgmsocket.h"
#include "filecache.h"
#include "packet.h"

class ServerResponder {
   public:
    ServerResponder(Filecache *m_cache);

    // given a client packet the corresponding packet responds with a matching
    // seq number — also hands off the incoming packet to the Filecache to do
    // idempotent stuff
    //
    // modifies packet IN PLACE
    void bounce(Packet *p);

   private:
    Filecache *m_cache;
};

// main server call
void listen(C150NETWORK::C150DgmSocket *sock, C150NETWORK::C150NastyFile *nfp,
            std::string dir);

#endif
