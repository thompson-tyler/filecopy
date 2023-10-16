#ifndef FILECOPY_UTILS
#define FILECOPY_UTILS

void setup_logging(const char *logname, int argc, char *argv[]);
bool check_directory(const char *dirname);
bool is_file(const char *filename);
void touch(const char *filename);

#define errp(...) fprintf(stderr, __VA_ARGS__)

#endif
