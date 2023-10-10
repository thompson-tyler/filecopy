#include "packet.h"

#include <cstring>
#include <sstream>

using namespace std;

/* client side */
Packet Packet::ofCheckIsNecessary(int id, std::string filename,
                                  unsigned char checksum[SHA_DIGEST_LENGTH]) {}
Packet Packet::ofKeepIt(int id) {}
Packet Packet::ofDeleteIt(int id) {}
Packet Packet::ofPrepareForBlob(int id, std::string filename, uint32_t nparts) {
}
Packet Packet::ofBlobSection(int id, uint32_t partno, uint32_t size,
                             const uint8_t *data) {}
/* servPacket::er side */
Packet Packet::intoAck() {}
Packet Packet::intoSOS() {}

string Packet::toString() {}
