// --------------------------------------------------------------
//
//                        C150StreamSocket.h
//
//        Author: Noah Mendelsohn         
//   
//        This class provides a convenience wrapper around a single
//        Linux TCP Stream socket. THIS VERSION IS LIMITED
//        IN THAT SERVERS ACCEPT EXACTLY ONE CLIENT CONNECTION
//        AT A TIME. THERE IS NO PROVISION FOR MORE GENERAL SERVERS
//        THAT WORK WITH MULTIPLE CLIENTS SIMULTANEOUSLY.
//
//           * Servers listen on and clients connect to a known port, which
//             in this class is determined base on the COMP 150
//             student's user name -- you don't have to (can't)
//             set the port explicitly.
//
//           * Servers automatically receive connections from any
//             IP address, but only on the known port.
//
//           * Clients use a connect method to supply the
//             name of the server to talk to.
//
//           * Indeed, clients are programs that issue
//             a connect call. Servers issue listen()
//             and then (repeatedly) accept.
//
//           * Simple interfaces are provided for giving
//             a read timeout value, and then for querying
//             whether a timeout occurred when attempting
//             to read. The default is that reads hang
//             forever.
//
//           * NEEDSWORK: there is currently no
//             means provided of doing non-blocking reads
//             or writes.
//
//           * The c150debug framework enables logging
//             of every read/write and message
//             reception, connection attempts, etc.
//             including part of each packet's
//             data. The framework allows the application
//             to select classes of data to be logged,
//             and to determine whether logging is to
//             a file or the C++ cerr stream.
//
//
//        EXCEPTIONS THROWN:
//
//              Except for timeouts, which can be queried with 
//              the timedout() method, network errors 
//              are reflected by throwing C150NetworkException.
//
//
//        DEBUG FLAGS
//
//            As noted above, the c150debug framework is used.
//            The following flags are used for debug output:
//
//               C150NETWORKTRAFFIC - log packets sent/delivered
//               C150NETWORKLOGIC   - internal decision making
//                                    in this class
//
//            Except for subtle problems, C150NETWORKTRAFFIC
//            should be what you need, and won't generate
//            too much extra output.
//
//        NOTE
//
//            The state member variable is used to track
//            whether we're just initialized,
//            known to be server (read before writing),
//            probably a client (set a server name),
//            known to be a client (connect called),
//            then did a write before reading.)
//            
//        C++ NAMESPACE
//        
//            All classes and interfaces are in 
//            the C150NETWORK namespace.
//        
//        CLASSES FOR GENERAL USE:
//
//           C150StreamSocket
//        
//     
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------

#ifndef __C150C150StreamSocket_H_INCLUDED__  
#define __C150C150StreamSocket_H_INCLUDED__ 


// Note: following should bring in most Unix
// networking and TCP/IP .h files

#include "c150network.h"
#include "c150debug.h"

using namespace std;

namespace C150NETWORK {


  // ------------------------------------------------------------------
  //
  //                        C150StreamSocket
  //
  //   Provides the services of a Unix/Linux socket, but with
  //   convenience methods for setting addresses, managing
  //   timeouts, etc.
  //
  //   For now, we use the same class for servers, clients
  //   and other datagram programs. 
  //   
  //   To make servers and clients easy to write, we play some
  //   tricks. If you call connect() before writing,
  //   we'll assume you're a client getting read to talk
  //   to that server. 
  //
  //   If you issue a read() before writing, and without
  //   setting a server name, then we'll remember
  //   who sent you the message. When you write(),
  //   we'll default to writing there. So, you'll
  //   act like a server.
  //
  //
  // ------------------------------------------------------------------

    //
    //  Maximum data size we'll write in a packet
    //
  const ssize_t MAXDGMSIZE = 512;

  class C150StreamSocket  {
  private:

    
    // NEEDSWORK: Private or protected copy constructor and assignment, but low 
    // priority -- probably nobody will be dumb enough to try copying or assinging these.
    // I hope.

    int listenfd;                    // file descriptor on which we listen
    int sockfd;                      // file descriptor for the socket
    struct sockaddr_in other_end;    // used to remember last server/port
                                     // we heard from 

    struct sockaddr_in this_end;     // used at servers only to establish
                                     // that we read from any host
                                     // but only on the user's port

    struct timeval timeout_length;   // timeout when zero, then no timeout, otherwise
                                     // max lenght of time our socket
                                     // operations can take
                                     // NEEDSWORK: does this apply to writes?

    bool timeoutHasHappened;         // set false before each read
                                     // true after read if timedout

    bool eofHasBeenRead;             // true after 0 len read

    unsigned short userport;         // the port for this c150 user name
                                     // this is in local, not network
                                     // byte order!


  protected:
  // possible states of a C150StreamSocket

  static const char* stateNames[];

  // Make sure to update the stateNames table at the bottom of
  // c150streamsocket when updating these
  enum stateEnum {
    uninitialized,              // don't know if we're acting as
                                // server or client
    client,                     // connect was done...we're a client
    listening     ,             // server, but not with 
                                // active connection
    server                      // acted as server --
                                // read issued before write
  } state;



  public:

    //
    //  Constructor and destructor
    //

    C150StreamSocket();                      // constructor
    virtual ~C150StreamSocket() noexcept(false);             // destructor

    //
    // Methods used in clients
    //

    virtual void connect(char *servername);  // name of server to talk to 

    //
    // Methods used in servers
    //
    //   NEEDSWORK: the following supports only a single connection
    //   per C150StreamSocket object. We should have instead
    //   server classes that act as factories for multiple
    //   sockets.
    //

    virtual void listen();
         //Create a listening socket
    virtual void accept();
         // Accept one connection (can only be called when
         // in listening state, I.e. after calling listen()
         // or close

    //
    // Methods used in clients and servers
    //
    //  Note that read will block until at least one
    //  byte is available, but message boundaries
    //  are NOT preserved when using TCP streams.
    //

    virtual ssize_t read(char *buf, ssize_t lenToRead);
    virtual void write(const char *buf, ssize_t lenToWrite);
    virtual void close();  // go back to uninitialized state if
                           // client, listening state if server.
                           // no-op if uninitalized or listening
                           // already

    //
    // Timeout management
    //

    virtual void turnOnTimeouts(int msecs);
    virtual void turnOffTimeouts();
    inline bool timeoutIsSet() {return (timeout_length.tv_sec + timeout_length.tv_usec)>0;};
    inline bool timedout() {return timeoutHasHappened;};
    inline bool eof() {return eofHasBeenRead;};

  };
}


#endif


