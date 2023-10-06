#include <cstdlib>
#include <iostream>

#include "c150debug.h"
#include "c150nastydgmsocket.h"
#include "messenger.h"

using namespace C150NETWORK;  // for all the comp150 utilities
using namespace std;

const int NASTINESS = 4;

void setUpDebugLogging(const char *logname, int argc, char *argv[]);

void printUsage(char *progname) {
    cerr << "Usage: " << progname << " [server|client]" << endl;
    exit(1);
}

void runClient() {
    fprintf(stderr, "Running client\n");

    char host[] = "comp117-02";
    C150DgmSocket *sock = new C150NastyDgmSocket(NASTINESS);
    sock->setServerName(host);

    sock->write(host, sizeof(host));

    Messenger *messenger = new Messenger(sock);

    while (true) {
        string message;
        getline(cin, message);
        messenger->send(message);
    }
}

void runServer() {
    fprintf(stderr, "Running server\n");

    C150DgmSocket *sock = new C150NastyDgmSocket(NASTINESS);

    Messenger *messenger = new Messenger(sock);

    while (true) {
        string message = messenger->read();
        cout << message << endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    try {
        if (strcmp(argv[1], "server") == 0) {
            setUpDebugLogging("serverdebug.txt", argc, argv);
            runServer();
        } else if (strcmp(argv[1], "client") == 0) {
            setUpDebugLogging("clientdebug.txt", argc, argv);
            runClient();
        } else {
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }
    } catch (C150NetworkException &e) {
        // Write to debug log
        fprintf(stderr, "Caught C150NetworkException: %s\n",
                e.formattedExplanation().c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void setUpDebugLogging(const char *logname, int argc, char *argv[]) {
    //
    //           Choose where debug output should go
    //
    // The default is that debug output goes to cerr.
    //
    // Uncomment the following three lines to direct
    // debug output to a file. Comment them
    // to default to the console.
    //
    // Note: the new DebugStream and ofstream MUST live after we return
    // from setUpDebugLogging, so we have to allocate
    // them dynamically.
    //
    //
    // Explanation:
    //
    //     The first line is ordinary C++ to open a file
    //     as an output stream.
    //
    //     The second line wraps that will all the services
    //     of a comp 150-IDS debug stream, and names that filestreamp.
    //
    //     The third line replaces the global variable c150debug
    //     and sets it to point to the new debugstream. Since c150debug
    //     is what all the c150 debug routines use to find the debug stream,
    //     you've now effectively overridden the default.
    //
    ofstream *outstreamp = new ofstream(logname);
    DebugStream *filestreamp = new DebugStream(outstreamp);
    DebugStream::setDefaultLogger(filestreamp);

    //
    //  Put the program name and a timestamp on each line of the debug log.
    //
    c150debug->setPrefix(argv[0]);
    c150debug->enableTimestamp();

    //
    // Ask to receive all classes of debug message
    //
    // See c150debug.h for other classes you can enable. To get more than
    // one class, you can or (|) the flags together and pass the combined
    // mask to c150debug -> enableLogging
    //
    // By the way, the default is to disable all output except for
    // messages written with the C150ALWAYSLOG flag. Those are typically
    // used only for things like fatal errors. So, the default is
    // for the system to run quietly without producing debug output.
    //
    c150debug->enableLogging(C150APPLICATION | C150NETWORKTRAFFIC |
                             C150NETWORKDELIVERY);
}
