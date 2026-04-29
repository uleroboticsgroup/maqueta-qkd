#ifndef CRYPTOGRAFY_H
#define CRYPTOGRAFY_H

#include <stdlib.h>

typedef struct {
    char *key_id;
    unsigned char *ciphertext;
    size_t length;
} EncryptedMessage;

EncryptedMessage* encrypt_message(const char* key_id, const unsigned char* key, const unsigned char* plaintext, size_t plain_len);
char* decrypt_message(const EncryptedMessage* msg, const unsigned char* key);

#endif