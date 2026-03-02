/** 
 * Encrypt and decrypt messages using a XOR cipher.
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
//#include "dilithium/ref/api.h" 
#include <stdbool.h>

typedef struct EncryptedMessage{
    char* key_id;
    unsigned char* ciphertext;
    size_t length;
} EncryptedMessage;

// Encrypt the plaintext using XOR with the key
EncryptedMessage* encrypt_message(const char* key_id, const unsigned char* key, size_t key_len, const unsigned char* plaintext, size_t plain_len) {
    if (key_len == 0) return NULL;

    EncryptedMessage* msg = malloc(sizeof(EncryptedMessage));
    if (!msg) return NULL;

    msg->key_id = strdup(key_id);
    msg->length = plain_len;
    msg->ciphertext = malloc(plain_len);

    if (!msg->ciphertext) {
        free(msg->key_id);
        free(msg);
        return NULL;
    }

    for (size_t i = 0; i < plain_len; i++) {
        msg->ciphertext[i] = plaintext[i] ^ key[i % key_len];
    }

    return msg;
}

// Decrypt the ciphertext using XOR with the key
char* decrypt_message(const EncryptedMessage* msg, const unsigned char* key, size_t key_len) {
    if (key_len == 0) return NULL;

    char* plaintext = malloc(msg->length + 1);
    if (!plaintext) return NULL;
    
    for (size_t i = 0; i < msg->length; i++) {
        plaintext[i] = (char)(msg->ciphertext[i] ^ key[i % key_len]);
    }
    plaintext[msg->length] = '\0';

    return plaintext;
}

// Parse the incoming JSON string to extract the ciphertext and key_id
EncryptedMessage* parse_incoming_message(const char* json_str, const char* key_id) {
    const char* array_start = strstr(json_str, "\"ciphertext\":[");
    if (!array_start) return NULL;
    
    array_start += 14;
    
    size_t count = 1;
    if (*array_start == ']') {
        count = 0;
    } else {
        for (const char* p = array_start; *p && *p != ']'; p++) {
            if (*p == ',') count++;
        }
    }
    
    EncryptedMessage* msg = malloc(sizeof(EncryptedMessage));
    if (!msg) return NULL;

    msg->key_id = strdup(key_id);
    msg->length = count;
    
    msg->ciphertext = malloc(count > 0 ? count : 1);
    if (!msg->ciphertext) {
        free(msg->key_id);
        free(msg);
        return NULL;
    }
    
    char* parser_ptr = (char*)array_start;
    for (size_t i = 0; i < count; i++) {
        msg->ciphertext[i] = (unsigned char)strtol(parser_ptr, &parser_ptr, 10);
        if (*parser_ptr == ',') parser_ptr++;
    }
    
    return msg;
}

// Free the memory allocated for the EncryptedMessage
void free_message(EncryptedMessage* msg) {
    if (msg) {
        free(msg->key_id);
        free(msg->ciphertext);
        free(msg);
    }
}