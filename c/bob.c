/**
 * Listens for incoming messages from Alice, retrieves the corresponding key from the QKD API, and decrypts the message using that key.
 */
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
#include "core/sign_dilithium.h"
#include "core/handle_msg.h"

#define URL_GETKEYBYID "https://kme-2.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-1/dec_keys?key_ID=%s"

//Bob receive the msg and key_id from Alice, then get the key from qkd and decrypt the msg.
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
        close(server_fd);
        return 1;
    }
    
    uint32_t payload_len = ntohl(net_len);

    char *buffer = malloc(payload_len + 1);
    if (!buffer) {
        close(new_socket);
        close(server_fd);
        return 1;
    }

    uint32_t total_read = 0;
    while (total_read < payload_len) {
        ssize_t bytes_read = read(new_socket, buffer + total_read, payload_len - total_read);
        if (bytes_read <= 0) break;
        total_read += (uint32_t)bytes_read;
    }
    buffer[payload_len] = '\0';

    char *k_id = get_key_id(buffer);
    char *acct = account_id();
    
    if (!acct) { 
        free(k_id);
        free(buffer); 
        close(new_socket);
        close(server_fd);
        return 1; 
    }
    
    if (k_id) {
        EncryptedMessage* msg = parse_incoming_message(buffer, k_id);

        char ca_path[512];
        char url[1024];
        snprintf(ca_path, sizeof(ca_path), "certs/account-%s-server-ca-qukaydee-com.crt", acct);
        snprintf(url, sizeof(url), URL_GETKEYBYID, acct, k_id);
        
        char *decp_response = connection_qkd(url, "certs/sae-2.crt", "certs/sae-2.key", ca_path);
            
        if (decp_response && msg) {
            char* key = get_key(decp_response);
            
            if (key) {
                char* textDecript = decrypt_message(msg, (unsigned char*)key, strlen(key));
                 
                if (textDecript) {
                    char *ctx = get_ctx(buffer); 
                    char *sign_hex = get_sign(buffer);
                    char *pk_hex = get_pk(buffer); 
                    size_t sig_bin_len, pk_bin_len;
                    unsigned char* sign_bin = hex_to_binary(sign_hex, &sig_bin_len);
                    unsigned char* pk_bin = hex_to_binary(pk_hex, &pk_bin_len);

                    if (sign_bin && pk_bin) {
                        int result = verify(sign_bin, (unsigned char*)textDecript, ctx, pk_bin);

                        if(result == 0) {
                            printf("Signature verified.\nText decrypted: %s\n", textDecript);
                        } else {
                            printf("Signature incorrect\n");
                        }
                    }
    
                    free(textDecript);
                    free(ctx);
                    free(sign_hex);
                    free(pk_hex);
                    free(sign_bin);
                    free(pk_bin);
                }
            }
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

int main(void) {
    return bob();
}