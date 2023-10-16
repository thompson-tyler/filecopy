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

// trick the c++ compiler into giving full space
struct file_entry_t {
    char filename[MAX_FILENAME_LENGTH] = {0};
};

struct files_t {
    C150NETWORK::C150NastyFile *nfp = nullptr;
    int nastiness = 0;
    char *dirname;
    unordered_map<int, file_entry_t> files;
};

// allocates and populates all fields
void files_register_fromdir(files_t *fs, char *dirname,
                            C150NETWORK::C150NastyFile *nfp, int nastiness);

// add a file and it's metadata to the filebuffer
bool files_register(files_t *fs, int id, const char *filename);

// returns nel of packets in sections_out
int files_topackets(files_t *fs, int id, packet_t *prep_out,
                    packet_t **sections_out, packet_t *check_is_necessary);

// buffer_in != nullptr && 0 <= start <= end <= length
// guaranteed safe!
//
// stores in TMP file
void files_storetmp(files_t *fs, int id, int offset, int nbytes,
                    const void *buffer_in);
bool files_checktmp(files_t *fs, int id, const checksum_t checksum_in);

// renames TMP file to permanent
void files_savepermanent(files_t *fs, int id);

// try delete file and re-initiliaze registry entry
void files_remove(files_t *fs, int id);

#endif
