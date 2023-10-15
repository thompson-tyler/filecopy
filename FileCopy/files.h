#ifndef FILES_H
#define FILES_H

#include <openssl/sha.h>

#include <cstdlib>

#include "c150nastyfile.h"
#include "packet.h"
#include "settings.h"

typedef unsigned char checksum_t[SHA_DIGEST_LENGTH];

struct files_t {
    C150NETWORK::C150NastyFile *nfp = nullptr;
    int nastiness = 0;
    int n_files = 0;
    struct {
        unsigned status = 0;  // undefined structure — depends on application
        char filename[MAX_FILENAME_LENGTH] = {0};
    } *files;
};

// allocates and populates all fields
files_t *files_register_fromdir(char *dirname, C150NETWORK::C150NastyFile *nfp,
                                int nastiness);

// add a file and it's metadata to the filebuffer
void files_register(files_t *fs, int id, const char *filename);
void files_calc_checksum(files_t *fs, int id, checksum_t checksum_out);

// deallocates files, (not *nfp)
void files_free(files_t *files);

// *buffer_out == nullptr && 0 <= start <= end <= length
// returns length of file section or -1 if an error
// guaranteed safe!
//
// loads from NON-tmp file
int files_load(files_t *fs, int id, int offset, int nbytes, void **buffer_out);

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

int files_length(files_t *fs, int id);

// returns nel of packets in sections_out
int files_topackets(files_t *fs, int id, packet_t *prep_out,
                    packet_t **sections_out);

#endif
