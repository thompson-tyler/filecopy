// --------------------------------------------------------------
//
//                        c150grading.h
//
//        Author: Noah Mendelsohn
//
//        Provides grading services that students
//        must invoke to have their projects graded.
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

#ifndef __C150GRADING_H_INCLUDED__
#define __C150GRADING_H_INCLUDED__

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/time.h>

using namespace std;

namespace C150NETWORK {

// ************************************************************************
//                    GLOBAL FUNCTIONS
// ************************************************************************

void GRADEME(int argc, char *argv[]);

// ************************************************************************
//                    GLOBAL VARIABLES
// ************************************************************************

extern ofstream *GRADING;
} // namespace C150NETWORK
#endif
