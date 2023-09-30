// --------------------------------------------------------------
//
//                        C150StreamSocket.cpp
//
//        Author: Noah Mendelsohn         
//   
//        Implements a server that uses TCP streams.
//
//        NEEDSWORK: THIS HEADER COMMENT IS INCOMPLETE
//
//           * Servers receive on and clients talk to a known port, which
//             in this class is determined baseD on the COMP 150
//             student's user name -- you don't have to (can't)
//             set the port explicitly.
//
//           * Servers automatically receive connections from any
//             IP address, but only on the known port.
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
//           C150StreamSocket
//        
//     
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------


// Note: following should bring in most Unix
// networking and TCP/IP .h files

#include "c150streamsocket.h"
#include <sstream>
#include <algorithm>   // for min function used in packet formatting

using namespace std;

unsigned short getUserPort() {return 7003;};

namespace C150NETWORK {

// ************************************************************************
//       METHODS FOR C150StreamSocket Class
// ************************************************************************


  // --------------------------------------------
  //   Constructor for C150StreamSocket Class
  //
  //     * Get a stream socket from the OS
  //     * Get and check port number for this
  //       COMP150IDS user
  //     * Remember those, and initialize
  //       other fields
  // --------------------------------------------

  C150StreamSocket::C150StreamSocket ()  {
    //
    //  Won't know either of these until we decide client or server
    //
    listenfd = 0;                      // NEEDSWORK: could be confused with stdin    
    sockfd = 0;                        // NEEDSWORK: could be confused with stdin    

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
       ss << "C150StreamSocket::C150StreamSocket(constructor): port number " << userport << 
	 "is in the 'well known or system port' range and can't be used for class assignments.\nSee https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.txt";
       throw C150NetworkException(ss.str());
   }

   c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::C150StreamSocket: user port is %d", (int)userport);
    
    //
    // Zero out internet addresses
    //
    memset(&other_end, 0, sizeof(struct sockaddr_in));
    memset(&this_end, 0, sizeof(struct sockaddr_in));

    //
    // Initialize other variables
    //
    timeoutHasHappened = false;
    eofHasBeenRead = false;
    state = uninitialized;
  };


  // --------------------------------------------
  //   Destructor
  // --------------------------------------------

  C150StreamSocket::~C150StreamSocket() noexcept(false) {
     if (listenfd > 0) {
       if (::close(listenfd) < 0) {
         throw C150NetworkException("C150StreamSocket::~C150StreamSocket(destructor): could not close listening socket. Error string=\"" + string(strerror(errno)) + "\"");	 
       };
       
     };
     if (sockfd > 0) {
       if (::close(sockfd) < 0) {
         throw C150NetworkException("C150StreamSocket::~C150StreamSocket(destructor): could not close data socket. Error string=\"" + string(strerror(errno)) + "\"");	 
       };
       
     };
   }; 

  // --------------------------------------------
  //
  //    connect()
  //
  //  This method is called at startup when we'll
  //  be acting as a client. It converts the server
  //  name to an IP address (or fails) and records
  //  that as "other_end".
  //
  //  It then connects to the other end
  //
  //
  // --------------------------------------------

  void C150StreamSocket::connect(char *servername) {
    int on_value;                    // parm to setsockopt
    struct hostent *server;

    c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::connect(%s) called", servername);


    // this should be the first step a client does
    if (state != uninitialized) {
     throw C150NetworkException("C150StreamSocket::connect: called after being established in state '%s'.\nThis method should only be used on a newly created C150StreamSocket" + string(stateNames[state]));
   }
    
    //
    // Create the socket
    //
    sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
     {
       stringstream msg;     // this is just to get the port as a string
       msg << "C150StreamSocket::connect: error from socket() system call. Server hostname="  <<  servername  <<   " userPort="  <<  userport <<  ", Error string=\""  <<  string(strerror(errno))  <<  "\"";
       throw C150NetworkException(msg.str());
     }

     // Set socket to allow immediate reuse per
     // www.ibm.com/developerworks/library/l-sockpit/
     on_value = 1;
     if (::setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on_value, sizeof(on_value) ) < 0)  {

       stringstream msg;     // this is just to get the port as a string
       msg << "C150StreamSocket::connect: error from setsockopt() system call. Server hostname="  <<  servername  <<   " userPort="  <<  userport <<  ", Error string=\""  <<  string(strerror(errno))  <<  "\"";
       throw C150NetworkException(msg.str());
     }


    // look up server name
    server = gethostbyname(servername);
    if (server == NULL) {
     throw C150NetworkException("C150StreamSocket::connect: couldn't resolve host name: " + string(servername));
   }

   // set up other_end structure to remember server info
   // Set to a network socket instead of a Unix Domain

   other_end.sin_family = AF_INET;
   bcopy((char *)server->h_addr, (char *)&other_end.sin_addr.s_addr, server->h_length);
   other_end.sin_port = htons(userport);

   if (::connect(sockfd, (sockaddr *)&other_end, (sizeof (struct sockaddr_in))) < 0)
   {
       stringstream msg;     // this is just to get the port as a string
       msg << "C150StreamSocket::connect: error from connect() system call. Server hostname="  <<  servername  <<   " userPort="  <<  userport <<  ", Error string=\""  <<  string(strerror(errno))  <<  "\"";
     
       throw C150NetworkException(msg.str());
   }
     
   // Any code that calls connect is presumed to be a client.
   // We'll only be sure when the code proves it by doing a write
   // before doing a read

   state = client;
   eofHasBeenRead = false;    // defensive: constructor should do this

  }; 

  // --------------------------------------------
  //
  //    listen()
  //
  //  Establishes us as a server. Set up a 
  //  socket to listen for connections from
  //  client.
  //
  // --------------------------------------------


  void C150StreamSocket::listen() {
     
     c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::listen() called");

     //
     // this should be the first step a server does
     //
     if (state != uninitialized) {
       throw C150NetworkException("C150StreamSocket::listen: called after being established in state '%s'.\nThis method should only be used on a newly created C150StreamSocket" + string(stateNames[state]));
     }

     //
     // Establish us as a server
     //
     state = listening;
    
     //
     // Get a stream socket from the operating system
     //
     listenfd = socket(AF_INET, SOCK_STREAM, 0);
     if (listenfd < 0) {
      throw C150NetworkException("C150StreamSocket::listen: could not create listening socket. Error string=\"" + string(strerror(errno)) + "\"");
     }

     this_end.sin_family = AF_INET;
     this_end.sin_addr.s_addr = INADDR_ANY;
     this_end.sin_port = htons(userport);
     
     //bind socket
     if (bind(listenfd, (struct sockaddr *) &this_end, sizeof(this_end)) < 0)
     {
       stringstream userPortasStream;     // this is just to get the port as a string
       userPortasStream << userport;      // the nonsense you have to go through in C++!!
     

       throw C150NetworkException("C150StreamSocket::listen: couldn't bind socket...userport=" + userPortasStream.str() + ", Error string=\"" + string(strerror(errno)) + "\"");
     }

     //
     // Tell socket to listen for connections
     //
     //  Allow system to queue up two unaccepted connections NEEDSWORK
     //
     if (::listen(listenfd, 2) < 0)
     {
       stringstream userPortasStream;     // this is just to get the port as a string
       userPortasStream << userport;      // the nonsense you have to go through in C++!!
       throw C150NetworkException("C150StreamSocket::listen: error on listen()...userport=" + userPortasStream.str() + ", Error string=\"" + string(strerror(errno)) + "\"");
     }


  };

  // --------------------------------------------
  //
  //    accept()
  //
  //  Accepts a new client connection and 
  //  establishes us as a server. 
  //
  // --------------------------------------------


  void C150StreamSocket::accept() {
     
     c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::accept() called");

     //
     // Accept onlhy makes sense if we've done a listen and 
     // don't have a data connection open.
     //
     if (state != listening) {
       throw C150NetworkException("C150StreamSocket::accept: called after being established in state '%s'.\n...This method should only be used on a C150StreamSocket in the listening state\n...I.e.after calling listen() or after calling close() on a previous data connection" + string(stateNames[state]));
     }

     
     //
     //  Accept one connection
     //
     //   NEEDSWORK: Should have separate server class
     //   that can accept multiple connections.
     //
     socklen_t remoteAddrLen = sizeof(other_end);  // we set up the length
                                             // of the other_end buffer
                                             // recvfrom may update
                                             // this length!

     sockfd = ::accept(listenfd, (struct sockaddr *)&other_end, &remoteAddrLen);
     if (sockfd < 0) {
       sockfd = 0;                        // so destructor won't try to clean it up
       stringstream userPortasStream;     // this is just to get the port as a string
       userPortasStream << userport;      // the nonsense you have to go through in C++!!
       throw C150NetworkException("C150StreamSocket::listen: accept() failed...userport=" + userPortasStream.str() + ", Error string=\"" + string(strerror(errno)) + "\"");
     }

     c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::accept() new connection successfully accepted...entering server state");

     //
     // Establish us as a server
     //
     state = server;
     eofHasBeenRead = false;              // important, this could be our 2nd time around



  };

  // --------------------------------------------
  //
  //    close()
  //
  //  Go back to listening state if server, 
  //  uninitliazed otherwise. In any case,
  //  close any open socket, but not
  //  the listening socket.
  //
  // --------------------------------------------


  void C150StreamSocket::close() {
     
     c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::close() called");
     
     //
     // this should be called only on an open data connection
     //
     if (state == uninitialized || state == listening) {
       throw C150NetworkException("C150StreamSocket::close: called after being established in state '%s'.\nclose should only be called when there is an open data connection as server or client" + string(stateNames[state]));
     }
     
     //
     //  Close possible data connection
     //
     if (sockfd > 0) {
       if(::close(sockfd) < 0) {
       throw C150NetworkException("C150StreamSocket::close: close() failed. Error string=\"" + string(strerror(errno)) + "\"");
       }
     }
     sockfd = 0;

     // 
     // For servers, we're still listening, otherwise
     // as if we never started.
     //
     if (state == server)
       state = listening;             // listenfd is still open, we assume
     else
       state = uninitialized;

     // NEEDSWORK: Not clear we're doing everything we should here
     // to re-initalize for a second client connection...check the constructor

     eofHasBeenRead = false;
  
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

  void C150StreamSocket::turnOnTimeouts(int msec) {
    if (state != server && state != client) {
       throw C150NetworkException("C150StreamSocket::turnOnTimeouts: called after being established in state '%s'.\n...turnOnTimeouts is only usable when data connection is open as client or server" + string(stateNames[state]));
    }

    int fullSeconds =  msec / 1000;

    timeout_length.tv_sec = fullSeconds ;      // no timeout set yet
    timeout_length.tv_usec = ((msec - fullSeconds*1000) * 1000);      

    c150debug->printf(C150NETWORKLOGIC,"Setting timeout to %d milliseconds (%d seconds + %d usec)",(int)msec, (int)fullSeconds, (int)timeout_length.tv_usec);

    // tell the socket about the timeout
    if (setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_length, sizeof(timeout_length)) < 0) {
      throw C150NetworkException("C150StreamSocket::turnOnTimeouts: error on setsockopt. Error string=\"" + string(strerror(errno)) + "\"");
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

  void C150StreamSocket::turnOffTimeouts() {
    if (state != server && state != client) {
       throw C150NetworkException("C150StreamSocket::turnOffTimeouts: called after being established in state '%s'.\n...turnOffTimeouts is only usable when data connection is open as client or server" + string(stateNames[state]));
    }

    c150debug->printf(C150NETWORKLOGIC,"Turning off timeouts");

    timeout_length.tv_sec = 0 ;      // no timeout set yet
    timeout_length.tv_usec = 0;      

    // tell the socket about the timeout
    if (setsockopt( sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_length, sizeof(timeout_length)) < 0) {
      throw C150NetworkException("C150StreamSocket::turnOffTimeouts: error on setsockopt. Error string=\"" + string(strerror(errno)) + "\"");
    }

  };
    


  // --------------------------------------------
  //
  //    read()
  //
  //    Reads from socket into user supplied
  //    buffer. This is mostly a straight
  //    passthrough of the read() system
  //    call, except that timeout
  //    handing is through methods.
  //
  //    Note that len=0 means that the
  //    other side has sent EOF.
  //    
  // --------------------------------------------

  ssize_t C150StreamSocket::read(char *buf, ssize_t lenToRead) {
    ssize_t readlen;

    // not OK to read first if we're a client
    if (state == uninitialized) {
       throw C150NetworkException("C150StreamSocket::read called prior to initializing as server or client. Call either connect() or listen()/accept() first");       
    };

    //
    // Defensive programming, this check should never trigger
    // NEEDSWORK: should use assert
    // NEEDSWORK: needs clearer error message .. should say no connection
    //
    if (state != server && state != client) {
      throw C150Exception("C150StreamSocket::read: internal error -- inconsistent state. Should have been server or client");       
    }

    //
    // Receive the message
    //
    //  If we're a server, then set up to record the
    //  address of the sender in other_end
    //

    timeoutHasHappened = false;

    c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::read: Ready to read data");

    readlen = ::read(sockfd, buf, lenToRead);

    //
    // Read is complete -- see if it worked
    //
    c150debug->printf(C150NETWORKLOGIC,"C150StreamSocket::read: Dropped through read with len=%d",(int)readlen);

    if (readlen == 0)
      eofHasBeenRead = true;      // alert client to real EOF -- otherwise hard to tell from timeout

    else if (readlen < 0) {            // if recvfrom returned error
      // we'll assume any of these is a timeout if we've set up a timeout
      if (timeoutIsSet() && ((errno == EAGAIN) || (errno == EINTR) || errno == ETIMEDOUT)) {
        timeoutHasHappened = true;
        readlen = 0;
        c150debug->printf(C150NETWORKTRAFFIC  | C150NETWORKDELIVERY,"C150StreamSocket::read: returning timeout to application");
      } else { 
        throw C150NetworkException("C150StreamSocket::read: error reading stream. Error string=\"" + string(strerror(errno)) + "\"");
      }
    } else {
      string formattedPacket = string(buf, min((int)readlen,100));  // we'll display up to 100 chars of the actual packet
      cleanString(formattedPacket);                           // change non-printing chars to .
      c150debug->printf(C150NETWORKTRAFFIC | C150NETWORKDELIVERY,"C150StreamSocket::read: successful read with len=%d |%s|",(int)readlen,formattedPacket.c_str());
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
  //    Writes data
  //    
  // --------------------------------------------

  void C150StreamSocket::write(const char *buf, ssize_t lenToWrite) {
    ssize_t writeLen = 0;
    stringstream msg;     // used to assemble the message 
 
    
    //
    // Try writing
    //
    string formattedPacket = string(buf, min((int)lenToWrite,100));  // we'll display up to 100 chars of the actual packet
    cleanString(formattedPacket);                           // change non-printing chars to .
    c150debug->printf(C150NETWORKTRAFFIC,"C150StreamSocket::write: attempting to send packet with len=%d |%s|",(int)lenToWrite,formattedPacket.c_str());

    writeLen = ::write(sockfd, buf, lenToWrite);

    //
    // If length is positive but short, only some of our message
    // was accepted. For this course, we'll just punt
    //
    if (writeLen >= 0 && writeLen < lenToWrite) {
      msg << "C150StreamSocket::write: attempted sendto() of  " << lenToWrite << "bytes, but system could only send " <<  writeLen << " bytes";
      throw C150NetworkException(msg.str());
    }

    //
    // If length is negative, then we've encountered an error
    //
    if (writeLen < 0) {
      throw C150NetworkException("C150StreamSocket::write: error on sendto. Error string=\"" + string(strerror(errno)) + "\"");
    }

    //
    // Success! The data was written.
    //
    c150debug->printf(C150NETWORKTRAFFIC  | C150NETWORKDELIVERY,"C150StreamSocket::write: Success writing packet with len=%d |%s|",(int)lenToWrite,formattedPacket.c_str());
    return; 

  };

// ************************************************************************
//      STATIC MEMBERS OF C150StreamSocket class
// ************************************************************************

  // map from state enumeration to description strings
  // this array is used only to format debugging messages

  const char* C150StreamSocket::stateNames[] = {
    "uninitialized",
    "client",
    "listening",
    "server"
  };


} // end namespace C150NETWORK


