#include "files.h"

#include "utils.h"

#define mktmpname(fname)                                              \
    (strcat(strncpy((char *)alloca(MAX_FILENAME_LENGTH + 4), (fname), \
                    MAX_FILENAME_LENGTH),                             \
            ".tmp"))

files_t *files_register_fromdir(char *dirname, C150NETWORK::C150NastyFile *nfp,
                                int nastiness) {
    files_t *fs = (files_t *)malloc(sizeof(*fs));
    *fs = {
        .nfp = nfp,
        .nastiness = nastiness,
    };
    return fs;
}
