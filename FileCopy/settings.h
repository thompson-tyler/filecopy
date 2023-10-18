#ifndef CONFIG_H
#define CONFIG_H

#define FILENAME_LENGTH 80
#define DIRNAME_LENGTH 80

/* some of these settings may become easier with increased nastiness */

// Files settings
#define DISK_RETRIES(nastiness) 64               // (10 + 5 * (nastiness))
#define MATCHING_READS(nastiness) 3 * nastiness  // nastiness
#define READ_CHUNK_SIZE 1024
#define CHUNK_SIZE_RANDOMNESS 50

// Application settings
#define CLIENT_TIMEOUT 1000
#define SERVER_SHUTDOWN_SECONDS 30

// Messenger settings
#define RESENDS(nastiness) (5 + 3 * (nastiness))

// Number of times the client manager will try to send a file before giving up
#define MAX_SOS 4

#define SEND_GROUP(nastiness) 500

#endif
