#ifndef DISKIO_H
#define DISKIO_H

#include <openssl/sha.h>

#include "c150nastyfile.h"

// reads file into a buffer, buffer_pp should be nullptr
// returns length of new buffer or -1 if failed
// ALLOCATES NEW MEMORY FOR BUFFER if successful, make sure to free
int fileToBuffer(C150NETWORK::C150NastyFile *nfp, string srcfile,
                 uint8_t **buffer_pp,
                 unsigned char checksum[SHA_DIGEST_LENGTH]);

bool bufferToFile(C150NETWORK::NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                  uint32_t bufferlen);

// utils
void checkDirectory(char *dirname);
bool isFile(string fname);
string makeFileName(string dir, string name);
string makeTmpFileName(string dir, string name);

#endif
