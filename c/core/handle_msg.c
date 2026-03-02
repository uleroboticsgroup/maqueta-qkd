/** 
 * Reads the file how contains the message.
 * **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handle_msg.h"

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

//Build the json msg to send to Bob. The json have the key_id and the ciphertext.
char* build_json_payload(const char *key_id, const unsigned char *ciphertext, size_t cipher_len) {
    size_t max_size = 100 + strlen(key_id) + (cipher_len * 4);
    
    char *json_msg = malloc(max_size);

    int offset = snprintf(json_msg, max_size, "{\"keys\":[{\"key_ID\":\"%s\",\"ciphertext\":[", key_id);

    for(size_t i = 0; i < cipher_len; i++) {
        offset += snprintf(json_msg + offset, max_size - offset, "%d%s", 
                           ciphertext[i], 
                           (i == cipher_len - 1) ? "" : ",");
    }

    snprintf(json_msg + offset, max_size - offset, "]}]}");
    
    return json_msg;
}