#ifndef HANDLE_JSON_H
#define HANDLE_JSON_H

#include "cryptografy.h"

EncryptedMessage* parse_incoming_message(const char* json_str, const char* key_id);
char *account_id(void);
char *get_key_id(const char *json_data);
char *get_key(const char *json_data);
char *get_ctx(const char *json_data);
char *get_mlen(const char *json_data);
char *get_sign(const char *json_data);
char *get_ciphertext(const char *json_data);
char *get_pk(const char *json_data);

#endif