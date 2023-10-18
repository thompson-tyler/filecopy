#ifndef FILECOPY_UTILS
#define FILECOPY_UTILS

void setup_logging(const char *logname, int argc, char *argv[]);
void check_directory(const char *dirname);
bool is_file(const char *filename);
void touch(const char *filename);
unsigned long fnv1a_hash(const void *data, int size);
void print_hash(const unsigned char *hash);

#define errp(...) fprintf(stderr, __VA_ARGS__)

#endif
