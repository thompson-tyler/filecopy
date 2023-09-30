// --------------------------------------------------------------
//
//                        c150network.h
//
//        Author: Noah Mendelsohn         
//   
//        Utility services and exceptions for building
//        Tufts COMP 150-IDS networking projects.
//        
//        All classes, functions and interfaces are in the C150NETWORK namespace.
//        
//        InterfacesFunctions for general use:
//
//           getUserPort - return a TCP/UDP port number for this user
//        
//        Classes for use as exceptions
//
//           C150NetworkException 
//        
//     
// --------------------------------------------------------------

#ifndef __C150NETWORK_H_INCLUDED__  
#define __C150NETWORK_H_INCLUDED__  


#include <string>
// NEEDSWORK: Kerrisk implies following might be <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// for gethostbyname
#include <netdb.h> 
#include <errno.h>
#include <cstring>
#include <stdlib.h>
// Added unistd.h on 1/7/2015 -- seems to be neede by
// RHEL 6 compiler but not RHEL 5 for access to ::read, etc.
#include <unistd.h>
#include "c150debug.h"
#include "c150exceptions.h"
#include "c150grading.h"

using namespace std;

namespace C150NETWORK {

// ************************************************************************
//                            Functions
// ************************************************************************

//
//                     getUserPort
//
//
//        Returns a port number, usable for TCP or UDP.
//
//        The port is determined based on the user's login
//        name, unless the Environment variable C150IDSUSERPORT
//        is set, in which case the port is provided from that.
//
//        If the environment variable C150ALLOWALLHOSTS has
//        any value other than 'true' (lowercase), then
//        an exception will be thrown if running on any machine
//        not listsed in the allowed hosts file.
//
//        The C150IDSHOME environment variable should point
//        to a directory in which C150 course materials can be 
//        found.
//
//        The C150USERPORTFILE environment variable should name
//        the port file relative to that.
//

#define C150USERPORTOVERRIDEENV "C150IDSUSERPORT"
#define C150ALLOWHOSTSENV "C150ALLOWALLHOSTS"
#define C150IDSHOMEENV "COMP117"
#define C150USERPORTFILE "/files/c150Utils/userports.csv"

unsigned short getUserPort();


// ************************************************************************
//                            EXCEPTIONS
// ************************************************************************

  //
  //                     C150NetworkException
  //
  //   Error doing socket or network operation (may sometimes be
  //   thrown for ordinary files in cases where the code doesn't
  //   know what sort fo file descriptor is being accessed)
  //
  //   Derived classes can be created for more specific 
  //   networking exceptions. Each should 
  //
  class C150NetworkException : public C150Exception {
    private:

  protected:

      // constructor: used by derived classes to pass on their name and explanation
      C150NetworkException(string exceptionName, string explain): C150Exception(exceptionName,explain) {};
    public:
      // constructor: takes just an explanation
      C150NetworkException(string explain) : C150Exception("C150NetworkException",explain) {};
      
      
      // Use this constructor if you're really too lazy to provide an explanation (discouraged)
      C150NetworkException() : C150Exception("C150NetworkException", "C150NetworkException thrown with no explanation given") {};

      // best to have virtual destructor for classes with virtual methods
      virtual ~C150NetworkException() {};
    
  };

// ************************************************************************
//                            CLASSES
// ************************************************************************
}

#endif


