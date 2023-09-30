// --------------------------------------------------------------
//
//                        c150debug.h
//
//        Author: Noah Mendelsohn         
//   
//        Provides debug output services for Tufts COMP 150-IDS
//        projects
//
//        All classes and interfaces are in the C150NETWORK namespace.
//        
//        Classes for general use:
//2
//           DebugStream
//
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------

#ifndef __C150DEBUG_H_INCLUDED__  
#define __C150DEBUG_H_INCLUDED__  


#include <string>
#include <stack>
#include <iostream>
#include <sstream>
#include <inttypes.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include "c150utility.h"  // for printTimestamp

using namespace std;


#define C150DISABLEDEBUGLOG "C150DISABLEDEBUGLOG"

namespace C150NETWORK {

// ************************************************************************
//                    GLOBAL VARIABLES  
// ************************************************************************


  //
  //   DebugStream
  //
  //     Provides the services of debugging output to a supplied
  //     C++ ostream. A set of masks can be used to enable
  //     and disable classes of output, and also to enable
  //     timestamping.
  //     
  //     A prefix string can be set that is prepended to the
  //     beginning of the output, following the optional
  //     timestamp.
  //
  //     NEEDSWORK: this isn't really implemented 
  //          as a subclass of stream
  //
  //
  class DebugStream {
  private:
    ostream&  str;

    stack<string> prefixStack;
    //    string prefix;          // the prefix, if set, will be prepended
                            // to all output, following the possible
                            // timestamp. 
    bool timestampsEnabled;
    uint32_t mask;          // one bit on here for each style of output
    string indent;          // provides indendation after the colon

  protected:

    public:

      // constructor
      // the global c150debug variable is set to the first
      // stream created, but can be overridden by user
      // programs at any time after that
      // constructor
      DebugStream(ostream& os = cerr);
      DebugStream(ostream *os);
      void init(ostream& os);    // shared logic for both constructors

      // set default logger

      static void setDefaultLogger(DebugStream& ds);
      static void setDefaultLogger(DebugStream  *ds);
      
      // prefix control
      void setPrefix(const string& pref) {prefixStack.push(pref); };
      void setPrefix() {setPrefix("");};
      void popPrefix() {prefixStack.pop();}; 
      string getPrefix() {return (prefixStack.top());};
      bool isPrefixing() {return (getPrefix().length()>0);};

      // Indent is a fixed string, usually blanks
      // that goes right after the colon

      void setIndent(string s) { indent = s;};

      // timestamp control
      void enableTimestamp() {timestampsEnabled=true;};
      void disableTimestamp() {timestampsEnabled=false;};
      bool isTimestampEnabled() {return timestampsEnabled;};
      // NEEDSWORK
      // Note: timestamp should always end with extra blank char
      // This can be overridden to change timestamp format
      //      virtual void printTimestamp(ostream& os);

      // --------------------------------------------------
      //            MASKS TO CONTROL LOGGING 
      //
      //  NEEDSWORK: these should probably be C++ constants
      //             but I'm too lazy just now
      //
      //  Use these masks with enableLogging to determine
      //  what debugging output is produced. Or together (|)
      //  multiple flags to enable more output.
      //
      //  Quick tutorial: most applications, for debugging
      //  should:
      //
      //      enableLogging(C150NETWORKDELIVERY | C150APPLICATION)
      //
      //  Applications can use the C150APPLICATION flag, if desired,
      //  to add their own messages to the debug log. Applications can
      //  also define their own flags with values < 0x8000 (see below)
      //  
      //  Here are some hints: 
      //   It's suggested that C150ALWAYSLOG mask is on by 
      //   default. It should be used for emergency messages
      //   that apps will almost surely want to see.
      //   Apps can turn it off, but probably shouldn't
      //
      //   C150NETWORKLOGIC, C150NETWORKTRAFFIC, and
      //   C150NETWORKDELIVERY should typically
      //   be used as a hierarchy.
      //
      //   C150NETWORKDELIVERY is the most useful for
      //   most applications: it gives information
      //   on when packets are sent, received, dropped
      //   or delayed, but not much else.  
      //
      //   The other two add much more noise, but help
      //   with debugging the internal logic of 
      //   the 150 support framework.  C140NETWORKTRAFFIC
      //   tends to log most calls to the actual socket,
      //   and C150NETWORKLOGIC adds a bunch of tracking
      //   of the internal logic of the 150 framework.
      //   Most users won't need any of these.
      //
      //   C150NETWORKDEBUG is used for temporary debug
      //   messages and mostly shouldn't show up in stable code 
      //
      //  The mask is 32 bits. Here we define a few standard
      //  values, using the high order bits, leaving the
      //  low bits for users that want control of their
      //  programs
      //
      // --------------------------------------------------


// Should always come out -- rarely used, but
// turned on by default in the framework
#define C150ALWAYSLOG         0x80000000
// Should show messages sent and received from the underlying socket
// (note that some of these may be dropped or delayed before 
// they reach the application)
#define C150NETWORKTRAFFIC    0x40000000
// Should show message traffic actually sent to application
// the application, along with information on timeouts, etc.
#define C150NETWORKDELIVERY    0x20000000
// Track internal flow of the DgmSocket code
#define C150NETWORKLOGIC      0x10000000
// Only for temporary use solving particular bugs
#define C150NETWORKDEBUG      0x08000000
// File operations
#define C150FILEDEBUG      0x04000000
// RPC framework 
#define C150RPCDEBUG       0x02000000

// default flags for application logging
// In practice, apps can define and use
// any flag <= 8000
#define C150APPLICATION       0x00008000  

// Use this to cause a message to show if debuggin
// is on at all (use this rarely!)
#define C150ALLDEBUG          0xFFFFFFFF

      // --------------------------------------------------
      //           METHODS
      // --------------------------------------------------


      // logging mode control
      // output will be provided only for classes of output
      // that are enabled
      void  enableLogging(uint32_t logmask) {mask |= logmask;};
      void  disableLogging(uint32_t logmask) {mask &= ~logmask;};
      bool  logChannelEnabled(uint32_t logmask) {return logmask & mask;};

      // Log a line -- note, this method is NOT designed to be chained further
      // because we want to have it called for each line.
      // note that this method will write an end of line to terminate
      // the previous line each time it is called, except the first

      virtual void printf(const uint32_t logmask, const char *fmt, ...);
      virtual void log(const uint32_t logmask, const string& s);
      virtual void log(const uint32_t logmask, const ostringstream& ss) {log(logmask,ss.str());};

      // best to have virtual destructor for classes with virtual methods
      virtual ~DebugStream() {str << endl;};
  
  };

}

extern C150NETWORK::DebugStream *c150debug;  // shared pointer to default logging stream


#endif


