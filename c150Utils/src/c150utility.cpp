// --------------------------------------------------------------
//
//                        c150utility.cpp
//
//        Author: Noah Mendelsohn         
//   
//        Provides utility services that may be helpful with a 
//        variety of Tufts COMP 150-IDS projects.
//
//        All classes, methods, etc. are in the C150NETWORK namespace.
//        
//        Functions for general use:
//
//           printTimestamp
//           cleanString, cleanchar
//
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------


// Note: following should bring in most Unix
// networking and TCP/IP .h files

#include "c150utility.h"
#include <iomanip>
#include <sstream>
#include <string>


using namespace std;

namespace C150NETWORK {

  //
  // printTimeStamp
  //
  //   Returns the current time formatted like 07/29/2012 15:53:41.956079
  //

   void printTimeStamp(ostream& os) {

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

  //
  //   cleanChar
  //
  //     Turns non-printable and control characters (including \n) into '.'
  //
  int cleanChar(const int c) {

    return (c >= ' ' && c <= '~') ? c : '.';
    
  };    

  void printTimeStamp(string& s) {
    ostringstream oss;
    printTimeStamp(oss);
    s = oss.str();
  };


  //
  //   cleanString
  //
  //     Replaces all non printing and control characters in the
  //     supplied string, including null and \n, with '.'
  //
  //     Note that the string is modified in place  
  //
  //     Two forms are provided. The first form
  //     takes a string reference, and returns
  //     a reference to the string, which is modified
  //     in place. You can either ignore the return
  //     value, or use it for chaining as in:
  //         s.cleanString().anotherMethod()
  //
  //     The second form takes start and end iterators
  //     and may be useful e.g. for cleaning up substrings.
  //

  string& cleanString(string& s) {
    transform(s.begin(), s.end(), s.begin(), cleanChar);
    return s;
  };

  void cleanString(string::iterator start,string::iterator end) {
    transform(start, end, start, cleanChar);
  };

} // end namespace C150NETWORK


