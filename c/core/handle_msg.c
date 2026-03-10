/** 
 * Reads the file how contains the message.
 * **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handle_msg.h"
#include "cryptografy.h"


//Read the file and return the content and the len.
unsigned char* read_file(const char *filename, size_t *out_len) {
    FILE *f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        fclose(f);
        return NULL;
    }

    unsigned char *buffer = malloc(fsize + 1);
    if (!buffer) {
        perror("Failed to allocate memory for file");
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, fsize, f);
    buffer[read_bytes] = '\0';
    *out_len = read_bytes;

    fclose(f);
    return buffer;
}

//Change raw binary to hexadecimal
char* binary_to_hex(const unsigned char* bin, size_t len) {
    if (!bin || len == 0) return NULL;
    char* hex = malloc(len * 2 + 1);
    if (!hex) return NULL;
    
    for(size_t i = 0; i < len; i++) {
        sprintf(hex + (i * 2), "%02x", bin[i]);
    }
    hex[len * 2] = '\0';
    return hex;
}

unsigned char* hex_to_binary(const char* hexstr, size_t* out_len) {
    if (!hexstr) return NULL;
    size_t len = strlen(hexstr);
    if (len % 2 != 0) return NULL;

    size_t bin_len = len / 2;
    unsigned char* bin = malloc(bin_len);
    if (!bin) return NULL;

    for (size_t i = 0; i < bin_len; i++) {
        sscanf(hexstr + 2 * i, "%2hhx", &bin[i]);
    }

    *out_len = bin_len;
    return bin;
}

//Build the json msg to send to Bob. The json have the key_id and the ciphertext.
char* build_json_payload(const PayloadSend *payload) {
    size_t ciphertext_max_len = payload->encrypt_msg->len * 4; 
    
    char *hex_sign = binary_to_hex(payload->signed_msg->sign, payload->signed_msg->siglen);
    char *hex_pk = binary_to_hex(payload->signed_msg->pk, payload->signed_msg->siglen);

    if (!hex_sign || !hex_pk) {
        free(hex_sign);
        free(hex_pk);
        return NULL;
    }
    
    size_t ctx_len = payload->signed_msg->ctx ? strlen((char*)payload->signed_msg->ctx) : 0;
    size_t sign_len = strlen(hex_sign);
    size_t pk_len = strlen(hex_pk);
    size_t keyid_len = payload->encrypt_msg->key_id ? strlen(payload->encrypt_msg->key_id) : 0;
    
    size_t max_size = 250 + keyid_len + ciphertext_max_len + ctx_len + sign_len + pk_len;
    char *json_msg = malloc(max_size);
    if (!json_msg) {
        free(hex_sign);
        free(hex_pk);
        return NULL;
    }

    int offset = snprintf(json_msg, max_size, 
        "{\"keys\":[{"
        "\"key_ID\":\"%s\","
        "\"ctx\":\"%s\","
        "\"sign\":\"%s\","
        "\"pk\":\"%s\","
        "\"ciphertext\":[",
        payload->encrypt_msg->key_id,
        payload->signed_msg->ctx,
        hex_sign,
        hex_pk
    );

    for(size_t i = 0; i < payload->encrypt_msg->len; i++) {
        offset += snprintf(json_msg + offset, max_size - offset, "%d%s", 
                           payload->encrypt_msg->ciphertext[i], 
                           (i == payload->encrypt_msg->len - 1) ? "" : ",");
    }

    snprintf(json_msg + offset, max_size - offset, "]}]}");
    
    free(hex_sign);
    free(hex_pk);
    
    return json_msg;
}