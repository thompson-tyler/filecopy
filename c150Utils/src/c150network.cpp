// --------------------------------------------------------------
//
//                        c150network.cpp
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

// --------------------------------------------------------------

#include "c150network.h"
#include <cstdlib>         // for getenv()
#include <cstring>         // for strcmp
#include <cstdlib>         // for strspn & getenv
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>        // for getlogin_r

using namespace std;

namespace C150NETWORK {

  unsigned short portFromUserID(const string &uname);   // fwd declaration

//
//                     getUserPort
//
//
//        Returns a port number, usable for TCP or UDP.
//
//
//        The port is determined based on the user's loging
//        name, unless the Environment variable C150IDSUSERPORT
//        is set, in which case the port is provided from that.
//
//        If the environment variable C150IDSALLOWALLHOSTS has
//        any value other than 'true' (lowercase), then
//        an exception will be thrown if running on any machine
//        not listsed in the allowed hosts file.
//

  unsigned short getUserPort() {
    char *overridePort;
    char *allowHosts;
    char hostname[100];
    unsigned short port;
    stringstream sss;
    char loginName[100];    // 100 should be plenty


    //
    // Check if we are on a legal host
    //
    allowHosts = getenv(C150ALLOWHOSTSENV);   // C150ALLOWALLHOSTS=true?
    if (allowHosts == NULL || strcmp(allowHosts,"true")) {
      if (gethostname(hostname, sizeof(hostname)) < 0) {
        throw C150NetworkException("getUserPort: error getting host name");
      }
      if ((strcmp(hostname,"comp117-01.eecs.tufts.edu") !=0) && 
	  (strcmp(hostname,"comp117-02.eecs.tufts.edu") !=0) &&
	  (strcmp(hostname,"comp117-01") !=0) && 
	  (strcmp(hostname,"comp117-02") !=0)) {
        sss << "getUserPort: you are on host " << hostname << " but COMP 150-IDS networking programs can only be tested on virtual hosts comp117-01 and comp117-02";
	throw C150NetworkException(sss.str());
      }
    }

    // 
    // We are on a legal host -- see if there is an override for the port
    //

    overridePort = getenv(C150USERPORTOVERRIDEENV);  //C150IDSUSERPORT
    if (overridePort != NULL) {
      if (strspn(overridePort, "0123456789") != strlen(overridePort)) { // all digits?
	 sss << "getUserPort: " << "environment variable " << C150USERPORTOVERRIDEENV << " has non-numeric value " << overridePort;
       throw C150NetworkException(sss.str());
      }

     // Override port is legal numeric value
     port = atoi(overridePort);
     c150debug -> printf(C150NETWORKLOGIC,"getUserPort found legal port override, assigning port=%d",port);
      
     return port;
    } 
 
    //
    // We are legal host, and there's no port override
    // Look up the user in the config file
    //

    else {
      getlogin_r(loginName, sizeof(loginName));
      return portFromUserID(loginName);
    }

  }

//
//  portFromUserID
//
//     Looks in the file named userports.csv to find a port
//     number for the supplied login name.
//
//     NEEDSWORK: file should not be hardwired
//     NEEDSWORK: file directory should be found from environment variable
//


unsigned short
portFromUserID(const string &uname) {
    string line;
    const char *portString;
    size_t commaPos;
    char *c150idsHome;
    stringstream ss;

    c150idsHome = getenv(C150IDSHOMEENV);   // C150ALLOWALLHOSTS=true?
    if (c150idsHome == NULL || strlen(c150idsHome)<1) {
      ss << "getUserPort: environment variable " << C150IDSHOMEENV <<
	" must be set";
      throw C150NetworkException(ss.str());
    }

    
    // NEEDSWORK: File name should be looked up
    string userportfile = c150idsHome;    // base directory for all comp150 stuff
    userportfile += C150USERPORTFILE;     // concatenate to make full file name

    //
    // Open the file with userid -> port mappings
    //
    ifstream portfile;
    portfile.open(userportfile.c_str());
    if(!portfile.is_open()) {
	//NEEDSWORK: environment variable name?
      ss << "getUserPort: user port file " << userportfile << " not found. "
	      "Check if environment variable " << C150IDSHOMEENV << 
	      " is set properly.";
      throw C150NetworkException(ss.str());
    }
      
    //
    // Search for a line that starts with our userid
    //
    // The .csv file may or may not have a header line, but
    // unless there is a user with the ID LOGIN, we should
    // be OK. NEEDSWORK
    //
    while(getline(portfile, line)) {
      commaPos = line.find(',');
      if(commaPos == string::npos) 
	throw C150NetworkException("getUserPort: user port file has line without comma");
      // See if this is the line for our user id
      // NEEDSWORK: Fix DOS line endings
      if (line.substr(0,commaPos) == uname) {
	portString = line.substr(commaPos+1).c_str();
        if (strspn(portString, "0123456789") != strlen(portString))
	  throw C150NetworkException("getUserPort: user port file has non-numeric port specifier (make sure there are no DOS line endings)");
	return atoi(portString);          // found it, so return it!
      }
      
    }

    // 
    // We searched the whole file, but didn't match the user
    //
    string msg("getUserPort: no port file entry for user login name: " );
    throw C150NetworkException(msg + uname);    
  }
}
