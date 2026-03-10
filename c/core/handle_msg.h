#ifndef HANDLE_MSG_H
#define HANDLE_MSG_H

#include <stddef.h>
#include "cryptografy.h"
#include "sign_dilithium.h"

typedef struct { 
    EncryptedMessage *encrypt_msg;
    SignDilithium *signed_msg;
}PayloadSend;

unsigned char* read_file(const char *filename, size_t *out_len);
char* build_json_payload(const PayloadSend *payload);
unsigned char* hex_to_binary(const char* hexstr, size_t* out_len);

#endif