#ifndef HANDLE_JSON_H
#define HANDLE_JSON_H

char *account_id(void);
char *get_key_id(const char *json_data);
char *get_key(const char *json_data);
char *get_ctx(const char *json_data);
char *get_sign(const char *json_data);
char *get_ciphertext(const char *json_data);
char *get_pk(const char *json_data);

#endif