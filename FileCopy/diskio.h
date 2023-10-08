#ifndef DISKIO_H
#define DISKIO_H

#include <openssl/sha.h>

#include "c150nastyfile.h"

// reads file into a buffer, tries to free whereever buffer was pointing
// returns length of new buffer
uint32_t fileToBuffer(C150NETWORK::C150NastyFile *nfp, string dir,
                      string filename, uint8_t **buffer_pp,
                      unsigned char checksum[SHA_DIGEST_LENGTH]);
uint32_t fileToBuffer(C150NETWORK::C150NastyFile *nfp, string srcfile,
                      uint8_t **buffer_pp,
                      unsigned char checksum[SHA_DIGEST_LENGTH]);

bool bufferToFile(C150NETWORK::NASTYFILE *nfp, string dir, string filename,
                  uint8_t *buffer, uint32_t bufferlen);
bool bufferToFile(C150NETWORK::NASTYFILE *nfp, string srcfile, uint8_t *buffer,
                  uint32_t bufferlen);

// utils
void checkDirectory(char *dirname);
bool isFile(string fname);
string makeFileName(string dir, string name);
string makeTmpFileName(string dir, string name);

#endif
