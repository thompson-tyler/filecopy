#ifndef FILES_H
#define FILES_H

#include <assert.h>
#include <openssl/sha.h>

#include <cstdlib>
#include <unordered_map>

#include "c150nastyfile.h"
#include "packet.h"
#include "settings.h"

typedef unsigned char checksum_t[SHA_DIGEST_LENGTH];

#define FULLNAME (FILENAME_LENGTH + DIRNAME_LENGTH)

// trick the c++ compiler into giving full space
struct file_entry_t {
    char filename[FULLNAME] = {0};
};

// doesn't store acutal file contents
struct files_t {
    C150NETWORK::C150NastyFile *nfp = nullptr;
    int nastiness = 0;
    char dirname[DIRNAME_LENGTH];
    unordered_map<int, file_entry_t> files;
};

/*
 * files_t object functions
 **/

// allocates and populates all fields
void files_register_fromdir(files_t *fs, char *dirname,
                            C150NETWORK::C150NastyFile *nfp, int nastiness);

// add a file and it's metadata to the filename collector
// params:
//      filename doesn't include directory name and is null terminated
//      allow_touch == true means that filename have doesn't already exist
// returns:
//      true if filename is a file
//      false otherwise
bool files_register(files_t *fs, int id, const char *filename,
                    bool allow_touch);

/*
 * filesystem IO wrappers
 **/

// returns nel of packets in sections_out
int files_topackets(files_t *fs, int id, packet_t *prep_out,
                    packet_t **sections_out, packet_t *check_is_necessary);

// params:
//      buffer_in != nullptr
//
// returns:
//      true if write is guaranteed safe and correct
//      false if disk "bugs" cannot be remedied
//
// stores in <dirname>/<filename>.tmp file
//
bool files_writetmp(files_t *fs, int id, int nbytes, const void *buffer_in,
                    const checksum_t checksum_in);

// renames <dirname>/<filename>.tmp to <dirname>/<filename>
void files_savepermanent(files_t *fs, int id);

// removes <dirname>/<filename>.tmp
void files_remove(files_t *fs, int id);

#endif
