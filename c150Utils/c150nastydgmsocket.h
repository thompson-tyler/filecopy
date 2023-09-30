// --------------------------------------------------------------
//
//                        C150NastyDgmSocket.h
//
//        Author: Noah Mendelsohn         
//   
//        Implements a socket that can act as either a server
//        or a client. Most behavior is inherited from the
//        base class C150DgmSocket. The exception is that
//        when reading, this version will selectively
//        drop, reorder, or duplicate packets.
//        
//        All classes and interfaces are in the C150NETWORK namespace.
//        
//        Classes for general use:
//
//           C150NastyDgmSocket
//
//        Class for internal use:
//
//           C150NastyPacket 
//        
//        
//        NEEDSWORK
//
//         * Inconsistent member names. Some are camelCase
//           and some use the underbar_convention. Should
//           be made consistent.
//     
//        Copyright: 2012 Noah Mendelsohn
//     
// --------------------------------------------------------------

#ifndef __C150C150NastyDgmSocket_H_INCLUDED__  
#define __C150C150NastyDgmSocket_H_INCLUDED__ 


#include "c150dgmsocket.h"
#include <stack>


namespace C150NETWORK {

  // ------------------------------------------------------------------
  //
  //                        C150NastyPacket
  //
  //   When we're delaying packets, we queue each one up
  //   in an instance of this class
  //
  // ------------------------------------------------------------------

    //
    //  Maximum data size we'll write in a packet
    //

  class C150NastyPacket  {
  public:
    ssize_t len;              // length of valid data
    char data[MAXDGMSIZE];        // data is stored here
    C150NastyPacket(const char *buf, const ssize_t lenToRead);
  };

  // ------------------------------------------------------------------
  //
  //                        C150NastyDgmSocket
  //
  //   Provides same services as C150DgmSocket, except
  //   for delaying, dropping, and garbling packets.
  //
  //   Exceptions thrown:
  //
  //         See base class documentation.
  //
  // ------------------------------------------------------------------

    //
    //  Maximum data size we'll write in a packet
    //

  class C150NastyDgmSocket : public C150DgmSocket  {

  private:

    //
    // This class cycles through a set of 
    // instructions for keeping, delaying, or losing
    // packets. The instructions
    // are in the nastymachine array.
    // The #defines below are special values
    // Positive numbers indicate the number
    // of packets to slip this one (I.e.
    // the number of additional successful
    // receipts that must occur
    // before this packet can be delivered.
    // The state machine increments
    // every time a packet is read
    // from the base class
    //
    // NOTE: Every C150DELAYPACKET must
    // be eventually follwed by a
    // C150DELIVERDELAYEDPACKET
    //  
#define C150DELAYPACKET  (-2)
#define C150NASTYLOSEPACKET  (-1)
#define C150DELIVERNORMAL  (0)
#define C150DELIVERDELAYEDPACKET (1)

    int nastyState;          // the index in the state machine of the
                             // current state

    // NEEDSWORK: Following initialization is for testing
    // Should read from a file maybe?

    static int nastyMachine0[];
    static int nastyMachine0size;

    static int nastyMachine1[];
    static int nastyMachine1size;

    static int nastyMachine2[];
    static int nastyMachine2size;

    static int nastyMachine3[];
    static int nastyMachine3size;

    static int nastyMachine4[];
    static int nastyMachine4size;

#define C150NUMNASTYMACHINES 5
    static  int *nastyStateMachines[C150NUMNASTYMACHINES];

    static int nastyStateMachineSizes[C150NUMNASTYMACHINES]; // number of entries

    int *nastyStateMachine;
    int nastyStateMachineSize;

    
    
    
    std::stack<C150NastyPacket*> delayedPackets;

  protected:
    inline int peekNastyState() {return nastyStateMachine[nastyState];};
    inline int getNastyState() {
      int retval = nastyStateMachine[nastyState];
      bumpNastyState();
      return retval;
    };
    inline void bumpNastyState() {nastyState = (nastyState+1)%nastyStateMachineSize; };
 

  public:
    //
    //  The application or server implementation should override the following methods,
    //  which are called automatically when the state of a connection
    //  changes

    C150NastyDgmSocket(int stateMachineID);                      // constructor
    virtual ~C150NastyDgmSocket();             // destructor
    

    //
    // Overridden methods used in clients and servers
    //

    virtual ssize_t read(char *buf, ssize_t lenToRead);
  };
}


#endif


