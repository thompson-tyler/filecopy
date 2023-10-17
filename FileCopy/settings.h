#ifndef CONFIG_H
#define CONFIG_H

#define MAX_FILENAME_LENGTH 80
#define MAX_DIRNAME_LENGTH 80

/* some of these settings may become easier with increased nastiness */
#define MAX_DISK_RETRIES 5

/* only do end to end check */
// #define JUST_END_TO_END

// Messenger settings
#define CLIENT_TIMEOUT 3000
#define MAX_RESEND_ATTEMPTS 10

// Number of times the client manager will try to send a file before giving up
#define MAX_SOS_COUNT 4

#define MAX_SEND_GROUP 500

#endif
