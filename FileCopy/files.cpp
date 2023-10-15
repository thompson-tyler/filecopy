#include "files.h"

#include "utils.h"

#define mktmpname(fname)                                              \
    (strcat(strncpy((char *)alloca(MAX_FILENAME_LENGTH + 4), (fname), \
                    MAX_FILENAME_LENGTH),                             \
            ".tmp"))
