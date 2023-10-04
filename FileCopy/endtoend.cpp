#include "c150debug.h"
#include "c150nastydgmsocket.h"
#include <cstdlib>
#include <fstream>
#include <openssl/sha.h>

using namespace C150NETWORK; // for all the comp150 utilities
using namespace std;

enum MessageType {
    // server messages
    HERES_A_BLOB,
    HERES_A_SECTION,
    // client messages
    GIVE_NEXT_BLOB,
    SECTION_LOOKS_GOOD,
    SECTION_LOOKS_BAD,
    RESEND_BLOB,
    // both
    DONE,
};

struct Packet {
    Header hdr;
    union {
        struct {
            int uuid;
            int part_number;
        } startBlob;
    } data;
};

Packet p = getpacket();
if (p.hdr.type == HERES_A_BLOB) {
    p.data.startBlob;
}

struct Header {
    int checksum;
    MessageType type;
    int size;
};

struct Packet {
    Header header;
    char data[BUFSIZ];
};

const int NASTINESS = 0;
const size_t PACKET_SIZE = sizeof(Packet);

int main(int argc, char *argv[])
{
    ssize_t readlen;              // amount of data read from socket
    char messageBuf[PACKET_SIZE]; // received message data

    if (argc != 2) {
        fprintf(stderr, "usage: %s [server|client]\n", argv[0]);
        exit(1);
    }

    C150DgmSocket *sock;
    try {
        sock = new C150NastyDgmSocket(NASTINESS);
    } catch (C150NetworkException &e) {
        // Write to debug log
        fprintf(stderr, "Caught C150NetworkException: %s\n",
                e.formattedExplanation().c_str());
        // In case we're logging to a file, write to the console too
        // cerr << argv[0]
        //      << ": caught C150NetworkException: " << e.formattedExplanation()
        //      << endl;
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        while (1) {
            Packet packet;
            readlen = sock->read(&packet, PACKET_SIZE);

            if (readlen == 0) {
                break;
            } else if (readlen < 0) {
                fprintf(stderr, "read failed\n");
                exit(1);
            } else {
                printf("Received message: %s\n", packet.data);
            }
        }

        printf("Exiting server\n");

    } else if (strcmp(argv[1], "client") == 0) {

        char host[] = "comp117-02";
        // Tell the DGMSocket which server to talk to
        sock->setServerName(host);

        while (1) {
            printf("Enter message: ");
            char msg[128];
            fgets(msg, sizeof(msg), stdin);

            if (strcmp(msg, "quit\n") == 0)
                break;

            Packet packet;
            packet.header.type = MessageType::GIVE_NEXT_BLOB;
            packet.header.size = strlen(msg) + 1;
            memcpy(packet.data, msg, packet.header.size);

            sock->write((char *)&packet, PACKET_SIZE);
        }

        printf("Exiting client\n");
    } else {
        fprintf(stderr, "usage: %s [server|client]\n", argv[0]);
        exit(1);
    }
}