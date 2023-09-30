// --------------------------------------------------------------
//
//                        c150nastyfile.h
//
//        Author: Noah Mendelsohn         
//
//        A wrapper around normal Unix file operations.
//        "Nastiness" can be enabled to cause operations to
//        occasionally misbehave.
//   
//        Classes for general use:
//
//              C150NastyFile
//              C150FileException
//
//        Typedefs for general use:
//
//              NASTYFILE
//
//        Note: all classes, typedefs, etc. are in the 
//              C150NETWORK namespace
//        
//        NEEDSWORK
//
//     
// --------------------------------------------------------------

#ifndef __C150NASTYFILE_H_INCLUDED__  
#define __C150NASTYFILE_H_INCLUDED__ 

#include "c150debug.h"
#include "c150utility.h"
#include "c150exceptions.h"
#include <cstdio>
#include <cerrno>
#include <string>
#include <cstring>

using namespace std;

namespace C150NETWORK {


  // ------------------------------------------------------------------
  //
  //                        c150NastyFile
  //
  //     Wraps a Unix File Descriptor. Most operations are
  //     passed through, but when "nastiness" is > 0
  //     both reads and writes may be (intentionally) simulated as
  //     behaving incorrectly (writing bad data, etc.)
  //
  // ------------------------------------------------------------------



  class C150NastyFile  {

  private:
    // Prevent copying of these objects (destructor closes fd) 	  
    C150NastyFile(const C150NastyFile&);     // private copy const
    C150NastyFile& operator=(const C150NastyFile&); // private = op

  protected:
    int nastiness;        // 0 .. MAXNASTINESS -> how much trouble to cause
    string filename;      // file name for the file -- mainly for debug
    string mode;          //  mode in which we opened -- mainly for debug
    bool initialized;     // true if we have an open file to close
    FILE *fd;             // file descriptor for the open file, if any
                          // Man page says FILE *, but C++ says void *

    bool isOutputFile;    // true iff file used for ouput

    int readsDone, writesDone, seeksDone, tellsDone, rewindsDone, 
	    corruptionsDone, readCorruptionsDone;  // statistics

    static int readFileCount; // Used to track the number of input files
    static int writeFileCount; // Used to track the number of outputfiles


    void corruptIt(int fileCount);     // internal routine to corrupt the file
    void seedRand();           // called once to seed the random generator
    bool youLoseRandom(int percent); // randomly returns true
                          // percent% of the time
    bool youLoseRotor(int thisFile, int target, int wrap);

    //
    // Corruption methods to mess up files
    void writeNasty(long offset, const char *str); 
    void writeNasty2(); 
    

  public:
    //
    //  Constant
    //
    static int MAXNASTINESS;

    //
    //  Constructors
    //

    C150NastyFile(int nasty);

    //
    //  Unix/Linux FILE operations
    //
    //   All signatures are the same as for Unix equivalents, 
    //   except that FD is implied after fopen is done.
    //
    //   NOTE: FOPEN does NOT return a usable file descriptor.
    //   The return value NULL from fopen means an error, as is
    //   the case for the regular FOPEN. In case of success, 
    //   the return value will be a non-null integer that is not
    //   intended to be used for anything other than the error/noerror
    //   test.
    //
    //   All other return values are documented in the Unix man
    //   pages (or are simulated in the case that nastiness is turned on)
    //
    void *   fopen(const char *path, const char *mode);
    size_t   fread(void *ptr, size_t size, size_t nmem);
    size_t   fwrite(const void *ptr, size_t size, size_t nmemb);
    int fseek(long offset, int whence);
    long ftell();
    void rewind();
    int feof();
    int ferror();
    void clearerr();
    int fclose();
    

    //
    //  Destructor
    //

    virtual ~C150NastyFile();             // destructor

};


// ************************************************************************
//                            EXCEPTIONS
// ************************************************************************


  //
  //   C150FileException
  //
  //   Error doing file operation
  //
  class C150FileException : public C150Exception {
    private:

    public:
      // constructor: takes just an explanation
      C150FileException(string explain) : C150Exception("C150FileException", explain) {};
      
      // Use this constructor if you're really too lazy to provide an explanation (discouraged)
      C150FileException() : C150Exception("C150FileException", "C150FileException thrown with no explanation given") {};


      // best to have virtual destructor for classes with virtual methods
      virtual ~C150FileException() {};
    
  };

typedef C150NastyFile NASTYFILE;    // Similar in spirit to Unix FILE

}

#endif


