/** 
 * This class encrypt 
 * **/
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

//Encrypt the menssage with key get with QKD
EncryptedMessage* encrypt_message(const char* key_id, const unsigned char* key, size_t key_len, const unsigned char* plaintext, size_t plain_len) {
    if (key_len < plain_len) {
        printf("Error: The key is too short\n");
        return NULL;
    }

    EncryptedMessage* msg = malloc(sizeof(EncryptedMessage));
    msg->key_id = strdup(key_id);
    msg->length = plain_len;
    msg->ciphertext = malloc(plain_len);

    for (size_t i = 0; i < plain_len; i++) {
        msg->ciphertext[i] = plaintext[i] ^ key[i];
    }

    return msg;
}

//Decrypt the message with the same key of QKD
char* decrypt_message(const EncryptedMessage* msg, const unsigned char* key, size_t key_len) {
    if (key_len < msg->length) {
        printf("Error: The key is not enough\n");
        return NULL;
    }

    char* plaintext = malloc(msg->length + 1);
    
    for (size_t i = 0; i < msg->length; i++) {
        plaintext[i] = (char)(msg->ciphertext[i] ^ key[i]);
    }
    plaintext[msg->length] = '\0';

    return plaintext;
}

EncryptedMessage* parse_incoming_message(const char* json_str, const char* key_id) {
    EncryptedMessage* msg = malloc(sizeof(EncryptedMessage));
    msg->key_id = strdup(key_id);
    
    const char* array_start = strstr(json_str, "\"ciphertext\":[");
    if (!array_start) return NULL;
    
    array_start += 14;
    
    size_t count = 1;
    for (const char* p = array_start; *p && *p != ']'; p++) {
        if (*p == ',') count++;
    }
    
    msg->length = count;
    msg->ciphertext = malloc(count);
    
    char* parser_ptr = (char*)array_start;
    for (size_t i = 0; i < count; i++) {
        msg->ciphertext[i] = (unsigned char)strtol(parser_ptr, &parser_ptr, 10);
        if (*parser_ptr == ',') parser_ptr++;
    }
    
    return msg;
}

//Free the pointers
void free_message(EncryptedMessage* msg) {
    free(msg->key_id);
    free(msg->ciphertext);
    free(msg);
}

// int encrypt_dilithium() {
//     uint8_t pk[pqcrystals_dilithium2_PUBLICKEYBYTES];
//     uint8_t sk[pqcrystals_dilithium2_SECRETKEYBYTES];  
//     const char *msg = "Hello, world!";
//     size_t msglen = strlen(msg);
//     uint8_t *sm = malloc(pqcrystals_dilithium2_BYTES + msglen);
//     size_t smlen; 
//     uint8_t *recovered_msg = malloc(msglen + pqcrystals_dilithium2_BYTES);
//     size_t recovered_msglen;
//     if (!sm || !recovered_msg) {
//         fprintf(stderr, "Memory allocation failed\n");
//         return 1;
//     }
//     if (pqcrystals_dilithium2_ref_keypair(pk, sk) != 0) {
//         printf("Keypair generation failed\n");
//         free(sm); free(recovered_msg);
//         return 1;
//     }

//     pqcrystals_dilithium2_ref(sm, &smlen, (const uint8_t *)msg, msglen,NULL,0, sk);
//     printf("Message signed. Signed message length: %zu\n", smlen);
//     if (pqcrystals_dilithium2_ref_open(recovered_msg, &recovered_msglen, sm, smlen,NULL,0, pk) != 0) {
//         printf("Signature verification failed!\n");
//     } else {
//         recovered_msg[recovered_msglen] = '\0';
//         printf("Verification successful! Message: %s\n", recovered_msg);
//     }
//     free(sm);
//     free(recovered_msg);
//     return 0;
// }