#ifndef CRYPTOGRAFY_H
#define CRYPTOGRAFY_H

#include <stdlib.h>

//int encrypt_dilithium();
typedef struct EncryptedMessage EncryptedMessage;
EncryptedMessage* encrypt_message(const char* key_id, const unsigned char* key, size_t key_len, const unsigned char* plaintext, size_t plain_len);
char* decrypt_message(const EncryptedMessage* msg, const unsigned char* key, size_t key_len);
void free_message(EncryptedMessage*  msg);

#endif