# Makefile for COMP 117 End-to-End assignment
#
#    Copyright 2012 - Noah Mendelsohn
#
# The targets below are for samples that may be helpful, but:
#
#   YOUR MAIN JOB IN THIS ASSIGNMENT IS TO CREATE TWO 
#   NEW PROGRAMS: fileclient and fileserver
#
#   YOU WILL MODIFY THIS MAKEFILE TO BUILD THEM TOO
#
# Useful sample program targets:
#
#
#    nastyfiletest - demonstrates the c150nastyfile class that you 
#                    MUST use to read and write your files
#
#    sha1test -   sample code for computing SHA1 hash
#
#    makedatafile -  sometimes when testing file copy programs
#                    it's useful to have files in which it's
#                    relatively easy to spot changes.
#                    This program generates sample data files.
#
#  Maintenance targets:
#
#    Make sure these clean up and build your code too
#
#    clean       - clean out all compiled object and executable files
#    all         - (default target) make sure everything's compiled
#

# Do all C++ compies with g++
CPP = g++
CPPFLAGS = -g -Wall -Werror -I$(C150LIB) -std=c++11 -Wno-stringop-truncation

# Where the COMP 150 shared utilities live, including c150ids.a and userports.csv
# Note that environment variable COMP117 must be set for this to work!

C150LIB = $(COMP117)/files/c150Utils/
C150AR = $(C150LIB)c150ids.a

LDFLAGS = -lssl -lcrypto
INCLUDES = $(C150LIB)c150dgmsocket.h $(C150LIB)c150nastydgmsocket.h $(C150LIB)c150network.h $(C150LIB)c150exceptions.h $(C150LIB)c150debug.h $(C150LIB)c150utility.h

OBJ := cache.o messenger.o packet.o files.o utils.o

TESTS = $(patsubst %.cpp,%,$(wildcard tests/*.cpp))

default: fileclient fileserver

all: default nastyfiletest makedatafile sha1test

fileclient: fileclient.cpp $(C150AR) $(INCLUDES) $(OBJ)
	$(CPP) -o fileclient $(CPPFLAGS) fileclient.cpp $(C150AR) $(OBJ) $(LDFLAGS)

fileserver: fileserver.cpp $(C150AR) $(INCLUDES) $(OBJ)
	$(CPP) -o fileserver $(CPPFLAGS) fileserver.cpp $(C150AR) $(OBJ) $(LDFLAGS)

#
# Build the nastyfiletest sample
#
nastyfiletest: nastyfiletest.cpp  $(C150AR) $(INCLUDES)
	$(CPP) -o nastyfiletest  $(CPPFLAGS) nastyfiletest.cpp $(C150AR)

#
# Build the profile test
#
profile: profile.cpp $(C150AR) $(INCLUDES) $(OBJ)
	$(CPP) -o profile $(CPPFLAGS) profile.cpp $(C150AR) $(OBJ) $(LDFLAGS)


#
# Build the sha1test
#
sha1test: sha1test.cpp
	$(CPP) -o sha1test sha1test.cpp -lssl -lcrypto

#
# Build the makedatafile 
#
makedatafile: makedatafile.cpp
	$(CPP) -o makedatafile makedatafile.cpp 

#
# To get any .o, compile the corresponding .cpp
#
%.o: %.cpp $(INCLUDES)
	$(CPP) -c $(CPPFLAGS) $< -o $@

.PHONY: tests

tests: $(TESTS)

tests/%: tests/%.cpp $(OBJ) $(INCLUDES) $(C150AR)
	$(CPP) -o $@ $< $(CPPFLAGS) $(C150AR) $(OBJ) $(LDFLAGS)


#
# Delete all compiled code in preparation
# for forcing complete rebuild#

clean:
	 rm -f nastyfiletest sha1test makedatafile fileclient fileserver \
	 	 *.o main endtoend


