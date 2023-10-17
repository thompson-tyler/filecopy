#ifndef CONFIG_H
#define CONFIG_H

#define FILENAME_LENGTH 80
#define DIRNAME_LENGTH 80

/* some of these settings may become easier with increased nastiness */
#define DISK_RETRIES(nastiness) (5 + 2 * (nastiness))

// Application settings
#define CLIENT_TIMEOUT 3000
#define SERVER_SHUTDOWN_SECONDS 30

// Messenger settings
#define RESENDS(nastiness) (5 + 3 * (nastiness))
#define MATCHING_READS(nastiness) (nastiness)

// Number of times the client manager will try to send a file before giving up
#define MAX_SOS 4

#define SEND_GROUP(nastiness) 500

#endif
