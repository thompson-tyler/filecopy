#ifndef CONFIG_H
#define CONFIG_H

#define MAX_DISK_RETRIES 50
#define MAX_FILENAME_LENGTH 80
#define HASH_MATCHES 10
#define HASH_SAMPLES 200

/* only do end to end check */
// #define JUST_END_TO_END

// Messenger settings
#define MESSENGER_TIMEOUT 1000
#define MAX_RESEND_ATTEMPTS 10

// Number of times the client manager will try to send a file before giving up
#define MAX_SOS_COUNT 4

#define MAX_SEND_GROUP 200

#endif
