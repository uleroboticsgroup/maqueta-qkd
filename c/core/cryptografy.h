#ifndef CRYPTOGRAFY_H
#define CRYPTOGRAFY_H

#include <stdlib.h>

//int encrypt_dilithium();
typedef struct {
    char *key_id;
    unsigned char *ciphertext;
    size_t len;
} EncryptedMessage;
EncryptedMessage* encrypt_message(const char* key_id, const unsigned char* key, size_t key_len, const unsigned char* plaintext, size_t plain_len);
char* decrypt_message(const EncryptedMessage* msg, const unsigned char* key, size_t key_len);
EncryptedMessage* parse_incoming_message(const char* json_str, const char* key_id);
void free_message(EncryptedMessage*  msg);

#endif