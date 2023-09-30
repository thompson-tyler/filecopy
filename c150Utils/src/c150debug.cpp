// --------------------------------------------------------------
//
//                        c150debug.cpp
//
//        Author: Noah Mendelsohn         
//   
//        Provides debug output services for Tufts COMP 150-IDS
//        projects
//
//        All classes and interfaces are in the C150NETWORK namespace.
//        
//        Classes for general use:
//
//           DebugStream
//        
//        
//        NEEDSWORK
//           using fprintf to stderr
//        
//        
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------


// Note: following should bring in most Unix
// networking and TCP/IP .h files

#include "c150debug.h"
#include <iomanip>
#include <cstring>         // for strcmp

using namespace std;

C150NETWORK::DebugStream *c150debug = new C150NETWORK::DebugStream(cerr);  // The pointer everyone uses to find the stream


namespace C150NETWORK {

// ************************************************************************
//       STATIC METHODS FOR DebugStream Class
// ************************************************************************

      void DebugStream::setDefaultLogger(DebugStream *ds) {
	setDefaultLogger(*ds);
      };
      

      void DebugStream::setDefaultLogger(DebugStream& ds) {
	if (c150debug != NULL) {
	  delete c150debug;      // release any old logger
	}
	c150debug = &ds;
      };
      

// ************************************************************************
//       METHODS FOR DebugStream Class
// ************************************************************************



  // --------------------------------------------
  //   Constructor for DebugStream Class
  // --------------------------------------------

  // By convention, we always log output
  // sent with the C150ALWAYSLOG flag.
  // Other types of output can be enabled using
  // enableLogging
  // 
  DebugStream::DebugStream (ostream& os) : str(os) {
    init(os);                  // call shared constructor logic
  };

  DebugStream::DebugStream(ostream *osp) : str(*osp) {
    init(*osp);                  // call shared constructor logic
  }

  // shared constructor logic
  void DebugStream::init(ostream& os) {
    os << unitbuf;         // causes stream to flush after each <<
    // note that unitbuf is a c++ stream manipulator, like setw
    mask = 0;
    disableTimestamp();
    enableLogging(C150ALWAYSLOG);  // The C150ALWAYSLOG channel is always enabled
    setPrefix();             // no prefix
    setIndent("");           // no indent
    if (c150debug == NULL) c150debug = this;
  };



  // --------------------------------------------
  //   printTimeStamp
  //
  //   writes a formatted timestamp into
  //   the provided output stream
  // --------------------------------------------
  /* NOW IN C150UTILITY
   void DebugStream::printTimeStamp(ostream& os) {

      time_t t = time(0);   // get time now
      struct tm *now = localtime( & t );
      //day
      os << setfill('0') << setw(2) << (now->tm_mon + 1) << '/' << setw(2) <<  now->tm_mday  << '/' << setw(4) << (now->tm_year + 1900) << ' ';
      // time to the second
      os << setw(2) << now->tm_hour << ':' << setw(2) << now->tm_min << ':' << setw(2) << now->tm_sec ;

      // milliseconds
      struct timeval detail_time;
      gettimeofday(&detail_time,NULL);
      os << '.'  << setw(6)<< detail_time.tv_usec << setfill(' ');


     
   } ;

  */
  // --------------------------------------------
  //
  //    log(logmask, s)
  //
  //    If any of the bits supplied in the
  //    mask designate enabled channels,
  //    then write the output, possibly
  //    prefixed and timestamped.
  //
  // --------------------------------------------


  void DebugStream::log(const uint32_t logmask, const string& s) {
    bool needColon = false;
    bool prefixing = isPrefixing();
    char *disableDebugLog;

    disableDebugLog = getenv(C150DISABLEDEBUGLOG);   // DISABLEDEBUGLOG==true?
    if ((disableDebugLog == NULL || strcmp(disableDebugLog,"true")) &&
	(logChannelEnabled(logmask))) {

      // Format is TIMESTAMP_PREFIX: where _ is a space,
      // both timestamp and prefix are optional,
      // and colon is written only if 
      // at least one of the two is present

      if (isTimestampEnabled()) {
	needColon = true;
        printTimeStamp(str);
	if (prefixing) 
	  str << ' '; // space between timestamp and prefix
      }
      if (prefixing) {
        str << getPrefix();  // write the prefix
	needColon = true;
      }

      if (needColon)
	str << ": ";

      // write the indent

      str << indent;

      // Write the actual message

      str << s;

      // Force a line end

      str << endl;
      
    }
  };

  // --------------------------------------------
  //
  //    printf(logmask,fmt,args...)
  //
  //   NEEDSWORK: comment
  //
  //
  // --------------------------------------------



  void DebugStream::printf(const uint32_t logmask, const char *fmt, ...) {
    char buffer[256];
    va_list arg_ptr;

    //
    // Use the varargs form of printf to format
    // the data into buffer
    //
    va_start(arg_ptr, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg_ptr);
    va_end(arg_ptr);
    
    string outputString(buffer);  // convert from c to C++ string

    log(logmask, outputString);   // log it
  };




} // end namespace C150NETWORK


