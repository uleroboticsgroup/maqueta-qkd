#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include "core/handle_json.h"
#include "core/cryptografy.h"

#define URL_GETKEYBYID "https://kme-2.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-1/dec_keys?key_ID=%s"

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
    
    msg->len = count;
    msg->ciphertext = malloc(count);
    
    char* parser_ptr = (char*)array_start;
    for (size_t i = 0; i < count; i++) {
        msg->ciphertext[i] = (unsigned char)strtol(parser_ptr, &parser_ptr, 10);
        if (*parser_ptr == ',') parser_ptr++;
    }
    
    return msg;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    printf("Server (Bob) listening on port 8080...\n");

    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    
    uint32_t net_len = 0;
    if (read(new_socket, &net_len, 4) != 4) {
        printf("Failed to read header\n");
        close(new_socket);
        return 1;
    }
    
    uint32_t payload_len = ntohl(net_len);
    printf("Expecting payload of %u bytes\n", payload_len);

    char *buffer = malloc(payload_len + 1);
    int total_read = 0;
    while (total_read < payload_len) {
        int bytes_read = read(new_socket, buffer + total_read, payload_len - total_read);
        if (bytes_read <= 0) break;
        total_read += bytes_read;
    }
    buffer[payload_len] = '\0';

    printf("Received JSON from Alice: %s\n", buffer);

    char *k_id = get_key_id(buffer); 
    char url[1024];
    char ca_path[512];

    char *acct = account_id();
    if (!acct) { free(buffer); return 1; }
    
    if (k_id) {
        EncryptedMessage* msg = parse_incoming_message(buffer, k_id);

        snprintf(url, sizeof(url), URL_GETKEYBYID, acct, k_id);
        char *decp_response = connection_qkd(url, "../certs/sae-2.crt", "../certs/sae-2.key", ca_path);
            
        if (decp_response && msg) {
            char* key = get_key(decp_response);

            char* textDecript = decrypt_message(msg, (unsigned char*)key, strlen(key));
             
            printf("Mensaje Desencriptado: %s\n", textDecript);
            
            free(textDecript);
            free(key);
        }

        if (msg) {
            free(msg->key_id);
            free(msg->ciphertext);
            free(msg);
        }
        free(decp_response);
        free(k_id);
    }

    free(buffer);
    free(acct);
    close(new_socket);
    close(server_fd);
    return 0;
}