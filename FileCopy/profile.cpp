#include <openssl/sha.h>

#include <cstdio>
#include <cstring>

#include "c150exceptions.h"
#include "c150grading.h"
#include "c150nastydgmsocket.h"
#include "c150nastyfile.h"
#include "c150utility.h"
#include "files.h"
#include "packet.h"
#include "utils.h"

using namespace C150NETWORK;
using namespace std;

#define SHA_LEN SHA_DIGEST_LENGTH

void print_checksum(unsigned char checksum[SHA_DIGEST_LENGTH]) {
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        printf("%02x", checksum[i]);
    }
    printf("\n");
}

int main() {
    try {
        int file_nastiness = 5;
        C150NastyFile *nfp = new C150NastyFile(file_nastiness);
        char dirname[] = "SRC/";
        char filename[] = "warandpeace.txt";
        files_t fs;
        files_register_fromdir(&fs, dirname, nfp, file_nastiness);

        char *filepath = strcat(dirname, filename);

        int num_checksums = 20;
        uint8_t *buffer = nullptr;

        // Compute the real checksum using regular file pointer
        unsigned char real_checksum[SHA_LEN];
        FILE *f = fopen(filepath, "rb");
        assert(f);
        fseek(f, 0, SEEK_END);
        int filelength = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = (uint8_t *)malloc(filelength);
        assert(buffer);
        fread(buffer, 1, filelength, f);
        fclose(f);
        SHA1(buffer, filelength, real_checksum);
        free(buffer);
        buffer = nullptr;

        int successes = 0;
        for (int i = 0; i < num_checksums; i++) {
            packet_t prep, *sections, check;
            int id = (int)fnv1a_hash(filename, strlen(filename));
            files_topackets(&fs, id, &prep, &sections, &check);
            unsigned char checksum[SHA_LEN];
            memcpy(checksum, check.value.check.checksum, SHA_LEN);

            // assert(nfp->fopen(filepath, "rb"));
            // nfp->fseek(0, SEEK_END);
            // int nbytes = nfp->ftell();
            // buffer = (uint8_t *)malloc(nbytes);
            // assert(buffer);
            // nfp->fseek(0, SEEK_SET);
            // int len = nfp->fread(buffer, 1, nbytes);
            // assert(len == nbytes);
            // nfp->fclose();

            // SHA1(buffer, nbytes, checksums[i]);

            printf("Read #%d: ", i + 1);
            print_checksum(checksum);
            if (memcmp(checksum, real_checksum, SHA_LEN) == 0) {
                successes++;
            }
            free(sections);
        }
        printf("success/total = %d/%d\n", successes, num_checksums);
        printf("Successes: %f\n", (float)successes / num_checksums);
        printf("Failure: %f\n",
               (float)(num_checksums - successes) / num_checksums);
        delete nfp;
    } catch (C150Exception &e) {
        printf("Caught C150Exception: %s\n", e.formattedExplanation().c_str());
    }
}