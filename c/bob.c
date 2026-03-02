#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include "core/handle_json.h"
#include "core/cryptografy.h"
#include "core/connection_qkd.h"

#define URL_GETKEYBYID "https://kme-2.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-1/dec_keys?key_ID=%s"

int bob(){
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

    char *buffer = malloc(payload_len + 1);
    uint32_t total_read = 0;
    
    while (total_read < payload_len) {
        ssize_t bytes_read = read(new_socket, buffer + total_read, payload_len - total_read);
        if (bytes_read <= 0) break;
        total_read += (uint32_t)bytes_read;
    }
    buffer[payload_len] = '\0';

    char *k_id = get_key_id(buffer); 
    char url[1024];
    char ca_path[512];

    char *acct = account_id();
    if (!acct) { free(buffer); return 1; }
    
    if (k_id) {
        EncryptedMessage* msg = parse_incoming_message(buffer, k_id);

        snprintf(ca_path, sizeof(ca_path), "certs/account-%s-server-ca-qukaydee-com.crt", acct);
        
        snprintf(url, sizeof(url), URL_GETKEYBYID, acct, k_id);
        char *decp_response = connection_qkd(url, "certs/sae-2.crt", "certs/sae-2.key", ca_path);
            
        if (decp_response && msg) {
            char* key = get_key(decp_response);

            char* textDecript = decrypt_message(msg, (unsigned char*)key, strlen(key));
             
            printf("Text decrypted: %s\n", textDecript);
            
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

int main() {
    bob();
}