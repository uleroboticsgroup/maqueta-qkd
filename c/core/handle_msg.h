#ifndef HANDLE_JSON_H
#define HANDLE_JSON_H

#include <stddef.h>

unsigned char* read_file(const char *filename, size_t *out_len);
char* build_json_payload(const char *key_id, const unsigned char *ciphertext, size_t cipher_len);

#endif