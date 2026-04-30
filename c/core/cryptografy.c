
/**
 * Encrypt and decrypt messages using AES cipher with OpenSSL.
 * Follow this tutorial: https://eclipsesource.com/blogs/2017/01/17/tutorial-aes-encryption-and-decryption-with-openssl/
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <b64/cdecode.h>
#include <b64/cencode.h>

typedef struct EncryptedMessage{
    char* key_id;
    unsigned char* ciphertext;
    size_t length;
} EncryptedMessage;

/**
 * Decode a base64-encoded key to raw bytes
 * @key: The base64-encoded key to decode
 * Returns: The decoded raw key or NULL on failure
 */
static unsigned char* decode_base64_key(const char* key) {
    size_t b64_len = strlen(key);
    if (b64_len == 0) return NULL;

    unsigned char* raw_key = malloc(b64_len);
    if (!raw_key){
        free(raw_key);
        return NULL;
    }

    base64_decodestate state;
    base64_init_decodestate(&state);

    int decoded_len = base64_decode_block(key, b64_len, (char*)raw_key, &state);

    if (decoded_len <= 0) {
        free(raw_key);
        return NULL;
    }

    return raw_key;
}

/**
 * Encrypt the plaintext using AES-256-CBC and return an EncryptedMessage struct
 * @key_id: The key ID to associate with the encrypted message
 * @key: The base64-encoded key to use for encryption
 * @plaintext: The plaintext to encrypt
 * @plain_len: The length of the plaintext
 * Returns: The encrypted message or NULL on failure
 */
EncryptedMessage* encrypt_message(const char* key_id, const unsigned char* key, const unsigned char* plaintext, size_t plain_len) {

    unsigned char* raw_key = decode_base64_key((const char*)key);
    if (!raw_key) {
        printf("Failed to decode key for decryption\n");
        return NULL;
    }

    EncryptedMessage* msg = malloc(sizeof(EncryptedMessage));
    if (!msg) {
        free(raw_key);
        return NULL;
    }

    msg->key_id = strdup(key_id);
    msg->length = plain_len + 16;
    msg->ciphertext = calloc(1, msg->length);
    if (!msg->ciphertext) {
        free(msg->key_id);
        free(msg->ciphertext);
        free(msg);
        free(raw_key);
        return NULL;
    }

    /* Create and initialise the context */
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(msg->ciphertext);
        free(msg->key_id);
        free(msg);
        return NULL;
    }

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    * and IV size appropriate for your cipher
    * We use 256 bit AES, key size is 32 bytes(decode of base64) and IV size is 16 bytes.
    */
    unsigned char iv[16] = {0};
    int ret = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, raw_key, iv);
    free(raw_key);
    if (ret != 1) {
        printf("EVP_EncryptInit_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        free(msg->ciphertext);
        free(msg->key_id);
        free(msg);
        return NULL;
    }

    /* Provide the message to be decrypted, and obtain the plaintext output.
    * EVP_EncryptUpdate can be called multiple times if necessary
    */
    int len = 0;
    ret = EVP_EncryptUpdate(ctx, msg->ciphertext, &len, plaintext, plain_len);
    if (ret != 1) {
        printf("EVP_EncryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        free(msg->ciphertext);
        free(msg->key_id);
        free(msg);
        return NULL;
    }

    /* Finalize the decryption. Further plaintext bytes may be written at
    * this stage.
    */
    int final_len = 0;
    ret = EVP_EncryptFinal_ex(ctx, msg->ciphertext + len, &final_len);
    if (ret != 1) {
        printf("EVP_EncryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        free(msg->ciphertext);
        free(msg->key_id);
        free(msg);
        return NULL;
    }

    msg->length = len + final_len;
    EVP_CIPHER_CTX_free(ctx);
    return msg;
}

/**
 * Decrypt the ciphertext using AES-256-CBC and return the plaintext
 * @msg: The encrypted message to decrypt
 * @key: The base64-encoded key to use for decryption
 * Returns: The decrypted plaintext or NULL on failure
 */
char* decrypt_message(const EncryptedMessage* msg, const unsigned char* key) {

    unsigned char* raw_key = decode_base64_key((const char*)key);
    if (!raw_key) {
        printf("Failed to decode key for decryption\n");
        return NULL;
    }
    char* plaintext = malloc(msg->length + 1);
    if (!plaintext) {
        free(raw_key);
        return NULL;
    }

    memcpy(plaintext, msg->ciphertext, msg->length);
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        free(plaintext);
        free(raw_key);
        return NULL;
    }

    unsigned char iv[16] = {0};
    int ret = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, raw_key, iv);
    free(raw_key);
    if (ret != 1) {
        printf("EVP_DecryptInit_ex failed for decryption\n");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }

    int len = 0;
    ret = EVP_DecryptUpdate(ctx, (unsigned char*)plaintext, &len, (unsigned char*)plaintext, msg->length);
    if (ret != 1) {
        printf("EVP_DecryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }

    int final_len = 0;
    ret = EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext + len, &final_len);
    if (ret != 1) {
        printf("EVP_DecryptFinal_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }

    EVP_CIPHER_CTX_free(ctx);
    plaintext[len + final_len] = '\0';
    return plaintext;
}
