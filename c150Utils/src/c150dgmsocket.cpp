// --------------------------------------------------------------
//
//                        C150DgmSocket.cpp
//
//        Author: Noah Mendelsohn         
//   
//        Implements a server that accepts a single connection
//        from one client, serves that client, and then terminates.
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


// Note: following should bring in most Unix
// networking and TCP/IP .h files

#include "c150dgmsocket.h"
#include <sstream>
#include <algorithm>   // for min function used in packet formatting
#include <string>

using namespace std;

unsigned short getUserPort() {return 7003;};

namespace C150NETWORK {

// ************************************************************************
//       METHODS FOR C150DgmSocket Class
// ************************************************************************


  // --------------------------------------------
  //   Constructor for C150DgmSocket Class
  //
  //     * Get a datagram socket from the OS
  //     * Get and check port number for this
  //       COMP150IDS user
  //     * Remember those, and initialize
  //       other fields
  // --------------------------------------------

  C150DgmSocket::C150DgmSocket ()  {
    //
    // Get a datagram socket from the operating system
    //
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
     throw C150NetworkException("C150DgmSocket::C150DgmSocket(constructor): could not create datagram socket. Error string=\"" + string(strerror(errno)) + "\"");
   }

    //
    //  COMP 150IDS students each have their
    //  own UDP ports. Find the port for
    //  this student.
    //
    // This can throw exception if user port can't
    // be determined
    //
    userport = getUserPort();         // the port for this c150 user name

    
    if (userport < 1024) {              // just to be extra safe, make sure
                                        // we NEVER use important system
                                        // ports -- could happen if user
                                        // port assignment tables get messed up
       stringstream ss;                 // format the message
       ss << "C150DgmSocket::C150DgmSocket(constructor): port number " << userport << 
	 "is in the 'well known or system port' range and can't be used for class assignments.\nSee https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.txt";
       throw C150NetworkException(ss.str());
   }

   c150debug->printf(C150NETWORKLOGIC,"C150DgmSocket::C150DgmSocket: user port is %d", (int)userport);
    
    //
    // Zero out internet addresses
    //
    memset(&other_end, 0, sizeof(struct sockaddr_in));
    memset(&this_end, 0, sizeof(struct sockaddr_in));

    //
    // Initialize other variables
    //
    timeoutHasHappened = false;
    state = uninitialized;
  };


  // --------------------------------------------
  //   Destructor
  // --------------------------------------------

   C150DgmSocket::~C150DgmSocket() noexcept(false) {
     if (sockfd > 0) {
       if (close(sockfd) < 0) {
         throw C150NetworkException("C150DgmSocket::~C150DgmSocket(destructor): could not close socket. Error string=\"" + string(strerror(errno)) + "\"");	 
       };
       
     };
   }; 

  // --------------------------------------------
  //
  //    setServerName()
  //
  //  This method is called at startup when we'll
  //  be acting as a client. It converts the server
  //  name to an IP address (or fails) and records
  //  that as "other_end".
  //
  //  Also sets state "probableclient", which
  //  will prevent us from accidently later
  //  being used as server too.
  //
  // --------------------------------------------

  void C150DgmSocket::setServerName(char *servername) {
    struct hostent *server;

    // this should be the first step a client does
    if (state != uninitialized) {
     throw C150NetworkException("C150DgmSocket::setServerName: called after being established in state '%s'.\nThis method should only be used on a newly created C150DgmSocket" + string(stateNames[state]));
   }
    
    // look up server name
    server = gethostbyname(servername);
    if (server == NULL) {
     throw C150NetworkException("C150DgmSocket::setServerName: couldn't resolve host name: " + string(servername));
   }

   // set up other_end structure to remember server info
   // Set to a network socket instead of a Unix Domain

   other_end.sin_family = AF_INET;
   bcopy((char *)server->h_addr, (char *)&other_end.sin_addr.s_addr, server->h_length);
   other_end.sin_port = htons(userport);

   // Any code that sets a server name is presumed to be a client.
   // We'll only be sure when the code proves it by doing a write
   // before doing a read

   state = probableclient;
  }; 

  // --------------------------------------------
  //
  //    bindThisEndforServer()
  //
  //  Once we discover we're a server, then we need a sockaddr_in
  //  to say we read from any Internet address, but only on
  //  the user's port, and we bind to that.
  //
  // --------------------------------------------

  void C150DgmSocket::bindThisEndforServer() {
     this_end.sin_family = AF_INET;
     this_end.sin_addr.s_addr = INADDR_ANY;
     this_end.sin_port = htons(userport);
     
     //bind socket
     if (bind(sockfd, (struct sockaddr *) &this_end, sizeof(this_end)) < 0)
     {
       stringstream userPortasStream;     // this is just to get the port as a string
       userPortasStream << userport;      // the nonsense you have to go through in C++!!
     

       throw C150NetworkException("C150DgmSocket::bindThisEndforServer: couldn't bind socket...userport=" + userPortasStream.str() + ", Error string=\"" + string(strerror(errno)) + "\"");
     }
  };
    
  // --------------------------------------------
  //
  //    turnOnTimeouts()
  //
  //  Read() operations will end with 0 length
  //  and timedout() == true aftet this lenght of time
  //
  //  NOTE: this routine applies only to 
  //  receive operations like read. It would
  //  be possible to implement a similar
  //  service for writes using SO_SNDTIMEO
  //  on the socket.
  //
  //  Argument:
  //
  //    msecs number of milliseconds for timeout
  //
  // --------------------------------------------

  void C150DgmSocket::turnOnTimeouts(int msec) {

    int fullSeconds =  msec / 1000;

    if (msec <= 0) {
	    throw C150NetworkException("C150DgmSocket::turnOnTimeouts: timeout must be > 0");
    }

    timeout_length.tv_sec = fullSeconds ;      // no timeout set yet
    timeout_length.tv_usec = ((msec - fullSeconds*1000) * 1000);      

    c150debug->printf(C150NETWORKLOGIC,"Setting timeout to %d milliseconds (%d seconds + %d usec)",(int)msec, (int)fullSeconds, (int)timeout_length.tv_usec);

    // tell the socket about the timeout
    if (setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_length, sizeof(timeout_length)) < 0) {
      throw C150NetworkException("C150DgmSocket::turnOnTimeouts: error on setsockopt. Error string=\"" + string(strerror(errno)) + "\"");
    }



  };
    
  // --------------------------------------------
  //
  //    turnOffTimeouts()
  //
  //  Read() operations will not time out
  //  after this call.
  //
  // --------------------------------------------

  void C150DgmSocket::turnOffTimeouts() {
    c150debug->printf(C150NETWORKLOGIC,"Turning off timeouts");

    timeout_length.tv_sec = 0 ;      // no timeout set yet
    timeout_length.tv_usec = 0;      

    // tell the socket about the timeout
    if (setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_length, sizeof(timeout_length)) < 0) {
      throw C150NetworkException("C150DgmSocket::turnOffTimeouts: error on setsockopt. Error string=\"" + string(strerror(errno)) + "\"");
    }

  };
    


  // --------------------------------------------
  //
  //    read()
  //
  //    Reads from socket into user supplied
  //    buffer.
  //
  //    If we were a presumed client, then
  //    it's a mistake to call read()
  //    before write, so we check that.
  //    
  //    If this is a first-time read call when
  //    our state is uninitialized, then
  //    we know we're a server, and we
  //    set up to read from any host, but
  //    on the user's port.
  //    
  //    If all that's ok, we now know if we're
  //    server or client
  //    
  //    1) If we're server, we set up to
  //       note the other_end when reading
  //       so the next write(s) will go to the
  //       correct client.
  //    
  //    2) If we're a client, we just read
  //       back from the server.
  //    
  //    
  // --------------------------------------------

  ssize_t C150DgmSocket::read(char *buf, ssize_t lenToRead) {
    ssize_t readlen;

    // not OK to read first if we're a client
    if (state == probableclient) {
       throw C150NetworkException("C150DgmSocket::read: presumed client had set server address, but is now attempting a read before writing");       
    };

    // if this is first read/write call, then we're a server
    if (state == uninitialized) {
      bindThisEndforServer();  // listen to any client
      state = server;
    };

    //
    // Defensive programming, this check should never trigger
    // NEEDSWORK: should use assert
    //
    if (state != server && state != client) {
      throw C150Exception("C150DgmSocket::read: internal error -- inconsistent state. Should have been server or client");       
    }

    //
    // Receive the message
    //
    //  If we're a server, then set up to record the
    //  address of the sender in other_end
    //

    timeoutHasHappened = false;

    c150debug->printf(C150NETWORKLOGIC,"C150DgmSocket::read: Ready to accept datagram");

    if (state == server) {
      socklen_t remoteAddrLen = sizeof(other_end);  // we set up the length
                                             // of the other_end buffer
                                             // recvfrom may update
                                             // this length!
      readlen = recvfrom(sockfd, buf, lenToRead, 0, (struct sockaddr *)&other_end, &remoteAddrLen );
    } else {                                 // state == client
      readlen = recvfrom(sockfd, buf, lenToRead, 0, NULL, NULL);
    }

    //
    // Read is complete -- see if it worked
    //
    c150debug->printf(C150NETWORKLOGIC,"C150DgmSocket::read: Dropped through recvfrom with len=%d",(int)readlen);
    if (readlen < 0) {            // if recvfrom returned error
      // we'll assume any of these is a timeout if we've set up a timeout
      if (timeoutIsSet() && ((errno == EAGAIN) || (errno == EINTR) || errno == ETIMEDOUT)) {
        timeoutHasHappened = true;
        readlen = 0;
        c150debug->printf(C150NETWORKTRAFFIC  | C150NETWORKDELIVERY,"C150DgmSocket::read: returning timeout to application");
      } else { 
        throw C150NetworkException("C150DgmSocket::read: error reading datagram. Error string=\"" + string(strerror(errno)) + "\"");
      }
    } else {
      string formattedPacket = string(buf, min((int)readlen,100));  // we'll display up to 50 chars of the actual packet
      cleanString(formattedPacket);                           // change non-printing chars to .
      c150debug->printf(C150NETWORKTRAFFIC | C150NETWORKDELIVERY,"C150DgmSocket::read: successful read with len=%d |%s|",(int)readlen,formattedPacket.c_str());
    };                        // end readlen < 0

    // return amount read, or 0 if timeout has occurred
    // to distinguish timeout from true zero length 
    // (assuming timeouts have been enabled at all)
    // caller should use the timedout() method.
    return readlen;
  };

  // --------------------------------------------
  //
  //    write()
  //
  //    Writes message.
  //
  //    If we were a presumed client, then
  //    it's good that write is called before
  //    read. We'll update our state to
  //    note we're really a client.
  //    
  //    Because of the way our states are
  //    managed, other_end should always
  //    be set by now, either to the server's
  //    address, or to the clients.
  //    
  // --------------------------------------------

  void C150DgmSocket::write(const char *buf, ssize_t lenToWrite) {
    ssize_t writeLen = 0;
    stringstream msg;     // used to assemble the message 
 
    //
    //  If state is probableclient, then this call to write
    //  confirms that we are one
    //
    if (state == probableclient) {
      state = client;
    };

    


    //
    // It's good practice not to send UDP packets
    // that are too large
    //
    if (lenToWrite > MAXDGMSIZE) {
      msg << "C150DgmSocket::write: attempting to write "  << lenToWrite << ", which is larger than the C150DgmSocket limit of " <<  MAXDGMSIZE << " bytes";
      throw C150NetworkException(msg.str());
    }

    //
    // Try writing
    //
    // NEEDSWORK: Not sure cast of other_end below is quite right
    string formattedPacket = string(buf, min((int)lenToWrite,100));  // we'll display up to 50 chars of the actual packet
    cleanString(formattedPacket);                           // change non-printing chars to .
    c150debug->printf(C150NETWORKTRAFFIC,"C150DgmSocket::write: attempting to send packet with len=%d |%s|",(int)lenToWrite,formattedPacket.c_str());
    writeLen = sendto(sockfd, buf, lenToWrite, 0, (sockaddr *)&other_end, (sizeof (struct sockaddr_in)));

    //
    // If length is positive but short, only some of our message
    // was accepted. For this course, we'll just punt
    //
    if (writeLen >= 0 && writeLen < lenToWrite) {
      msg << "C150DgmSocket::write: attempted sendto() of  " << lenToWrite << "bytes, but system could only send " <<  writeLen << " bytes";
      throw C150NetworkException(msg.str());
    }

    //
    // If length is negative, then we've encountered an error
    //
    if (writeLen < 0) {
      throw C150NetworkException("C150DgmSocket::write: error on sendto. Error string=\"" + string(strerror(errno)) + "\"");
    }

    //
    // Success! The data was written.
    //
    c150debug->printf(C150NETWORKTRAFFIC  | C150NETWORKDELIVERY,"C150DgmSocket::write: Success writing packet with len=%d |%s|",(int)lenToWrite,formattedPacket.c_str());
    return; 

    };

// ************************************************************************
//      STATIC MEMBERS OF C150DgmSocket class
// ************************************************************************

  // map from state enumeration to description strings
  // this array is used only to format debugging messages

  const char* C150DgmSocket::stateNames[] = {
    "uninitialized",
    "probable client",
    "client",
    "server"
  };


} // end namespace C150NETWORK


