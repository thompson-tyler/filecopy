#ifndef DISKIO_H
#define DISKIO_H

#include <openssl/sha.h>

#include <string>

#include "c150nastyfile.h"
#include "settings.h"

typedef unsigned char checksum_t[SHA_DIGEST_LENGTH];

// reads file into a buffer, buffer_pp should be nullptr
// returns length of new buffer or -1 if failed
// ALLOCATES NEW MEMORY FOR BUFFER if successful, make sure to free
int fileToBuffer(C150NETWORK::C150NastyFile *nfp, string srcfile,
                 uint8_t **buffer_pp,
                 unsigned char checksum[SHA_DIGEST_LENGTH]);

bool bufferToFile(C150NETWORK::NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                  uint32_t bufferlen);

// Creates an empty file with the given filename
// If the file already exists, it is truncated
void touch(C150NETWORK::NASTYFILE *nfp, string fname);

// // all guaranteed safe
// int filesize(char *fname);
// int filechecksum(C150NETWORK::NASTYFILE *nfp, char *fname, checksum_t
// checksum); int fileread(C150NETWORK::NASTYFILE *nfp, char *fname, int offset,
// int len,
//              uint8_t **buffer_pp);
// int filewrite(C150NETWORK::NASTYFILE *nfp, char *fname, int offset, int len,
//               uint8_t *buffer);

// utils
void checkDirectory(char *dirname);
bool isFile(string fname);
string makeFileName(string dir, string name);
string makeTmpFileName(string dir, string name);

#endif
