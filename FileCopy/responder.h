#ifndef RESPONDER_H
#define RESPONDER_H

#include "c150dgmsocket.h"
#include "c150nastydgmsocket.h"
#include "filecache.h"
#include "packet.h"

class ServerResponder {
   public:
    ServerResponder();
    ~ServerResponder();

    // given a client packet the corresponding packet responds with a matching
    // seq number — also hands off the incoming packet to the Filecache to do
    // idempotent stuff
    Packet bounce(Packet p);

   private:
    Filecache m_cache;
};

void serverListen(C150NETWORK::C150DgmSocket *sock,
                  C150NETWORK::C150NastyFile *nfp, ServerResponder responder);

#endif
