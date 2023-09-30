// --------------------------------------------------------------
//
//                        C150DgmSocket.h
//
//        Author: Noah Mendelsohn         
//   
//        This class provides a convenience wrapper around a single
//        Linux UDP socket. Specifically, it provides a model
//        in which:
//
//           * Servers receive on and clients talk to a known port, which
//             in this class is determined base on the COMP 150
//             student's user name -- you don't have to (can't)
//             set the port explicitly.
//
//           * Servers automatically receive packets from any
//             IP address, but only on the known port.
//
//           * Servers automatically write to the last
//             address from which they received a packet.
//             NEEDSWORK: this is useful in simple cases,
//             but isn't good enough when one server is
//             interleaving work for multiple clients.
//
//           * Clients listen on a port automatically assigned
//             by the OS
//
//           * Clients use a simple method to supply the
//             name of the server to talk to.
//
//           * Indeed, the server/client role is determined 
//             automatically. An application that sets a 
//             server name to talk to and then writes before
//             reading is a client.An application that reads
//             before writing is a server.
//
//           * Simple interfaces are provided for giving
//             a read timeout value, and then for querying
//             whether a timeout occurred when attempting
//             to read. The default is that reads hang
//             forever.
//
//           * The c150debug framework enables logging
//             of every read/write and message
//             reception, including part of each packet's
//             data. The framework allows the application
//             to select classes of data to be logged,
//             and to determine whether logging is to
//             a file or the C++ cerr stream.
//
//           * Packet sizes are (perhaps arbitrarily) limited
//             to MAXDGMSIZE. This is typically set as
//             512, which is considered a good practice
//             limit for UDP.
//
//        EXCEPTIONS THROWN:
//
//              Except for timeouts, which can be queried with 
//              the timedout() method, network errors 
//              are reflected by throwing C150NetworkException.
//
//              Note that writes larger than
//              MAXDGMSIZE will throw an exception.
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
//            actually a client (set server name,
//            then did a write before reading.)
//            
//        C++ NAMESPACE
//        
//            All classes and interfaces are in 
//            the C150NETWORK namespace.
//        
//        CLASSES FOR GENERAL USE:
//
//           C150DgmSocket
//        
//     
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------

#ifndef __C150C150DgmSocket_H_INCLUDED__  
#define __C150C150DgmSocket_H_INCLUDED__ 


// Note: following should bring in most Unix
// networking and TCP/IP .h files

#include "c150network.h"
#include "c150debug.h"

using namespace std;

namespace C150NETWORK {


  // ------------------------------------------------------------------
  //
  //                        C150DgmSocket
  //
  //   Provides the services of a Unix/Linux socket, but with
  //   convenience methods for setting addresses, managing
  //   timeouts, etc.
  //
  //   For now, we use the same class for servers, clients
  //   and other datagram programs. 
  //   
  //   To make servers and clients easy to write, we play some
  //   tricks. If you call setServerName() before writing,
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

  class C150DgmSocket  {
  private:

    
    // NEEDSWORK: Private or protected copy constructor and assignment, but low 
    // priority -- probably nobody will be dumb enough to try copying or assinging these.
    // I hope.

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

    unsigned short userport;         // the port for this c150 user name
                                     // this is in local, not network
                                     // byte order!


  protected:
  // possible states of a C150DgmSocket

  static const char* stateNames[];

  enum stateEnum {
    uninitialized,                   // don't know if we're acting as
                                     // server or client
    probableclient,                  // acting as client -- server
                                     // addr is set and 
                                     // we will make sure
                                     // write is done before
                                     // read and then designate
                                     // as client
    client,                          // was probable client, that
                                     // went ahead and 
    server                           // acted as server --
                                     // read issued before write
  } state;

  //
  //  Following method is called internally
  //  once it's established this is a server, not
  //  a client
  //

  virtual void bindThisEndforServer();
                                     // for servers, establish
                                     // local sockaddr_in
                                     // from which to read


  public:

    //
    //  Constructor and destructor
    //

    C150DgmSocket();                 // constructor
    virtual ~C150DgmSocket() noexcept(false);        // destructor

    //
    // Methods used in clients
    //

    virtual void setServerName(char *servername);  // name of server to talk to 

    //
    // Methods used in clients and servers
    //

    virtual ssize_t read(char *buf, ssize_t lenToRead);
    virtual void write(const char *buf, ssize_t lenToWrite);

    //
    // Timeout management
    //

    virtual void turnOnTimeouts(int msecs);
    virtual void turnOffTimeouts();
    inline bool timeoutIsSet() {return (timeout_length.tv_sec + timeout_length.tv_usec)>0;};
    inline bool timedout() {return timeoutHasHappened;};

  };
}


#endif


