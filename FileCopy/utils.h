#ifndef FILECOPY_UTILS
#define FILECOPY_UTILS

void setup_logging(const char *logname, int argc, char *argv[]);
bool check_directory(const char *dirname);
bool is_files(const char *filename);
bool touch(const char *filename);

#endif
