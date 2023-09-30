// --------------------------------------------------------------
//
//                        c150utility.h
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
//     
// --------------------------------------------------------------

#ifndef __C150UTILITY_H_INCLUDED__  
#define __C150UTILITY_H_INCLUDED__  


#include <string>
#include <iostream>
#include <sys/time.h>
#include <algorithm>

using namespace std;

namespace C150NETWORK {

  // ************************************************************************
  //                    GLOBAL FUNCTIONS   
  // ************************************************************************

  // provides a formatted timestamp 
  // NEEDSWORK: should probably implement << instead or in addition

  void printTimeStamp(ostream& os);
  void printTimeStamp(string& s);

  // removes all control characters from a string
  // result is guaranteed printable.

  int cleanChar(const int c);    // returns a clean version of the supplied character

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

  string &cleanString(string& s);
  void cleanString(string::iterator start,string::iterator end );

}




#endif


