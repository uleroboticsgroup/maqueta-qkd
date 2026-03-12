/**
 * Alice is the sender of the message. She will get the key and key_id from qkd, encrypt and sign the msg and send it to Bob.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "core/handle_json.h"
#include "core/cryptografy.h"
#include "core/connection_qkd.h"
#include "core/handle_msg.h"
#include "core/sign_dilithium.h"
#include <stdint.h>


#define URL_GETKEY "https://kme-1.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-2%s"

//Alice send the msg to Bob. The msg have two parts msg and key_id to decrypt the msg.
int alice(char *msg) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    inet_pton(AF_INET, "192.168.8.2", &serv_addr.sin_addr);//Same network in the docker_compose

    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) {
            break;
        }
        printf("Bob not ready, retrying in 1s...\n");
        close(sock);
        sleep(1);
    }

    uint32_t payload_len = strlen(msg);
    uint32_t net_len = htonl(payload_len);

    send(sock, &net_len, 4, 0);

    send(sock, msg , payload_len, 0);
    
    printf("Encrypted message sent to Bob!\n");

    close(sock);
    return 0;
}

//Get key and key_id from qkd and encrypt the msg
int main(void){
    size_t len = 0;
    unsigned char *file = read_file("lorem.txt", &len);
    if (!file) return 1;

    char *ctx = "To Bob";

    SignDilithium* signed_msg = sign(file, len, ctx);
    if(!signed_msg) {
        free(file);
        return 1;
    }

    char *acct = account_id();
    if (!acct) {
        free(file);
        return 1;
    }
    char url[1024],ca_path[512];
    snprintf(ca_path, sizeof(ca_path), "certs/account-%s-server-ca-qukaydee-com.crt", acct);
    snprintf(url, sizeof(url), URL_GETKEY, acct, "/enc_keys?number=2&size=1024");
    char *response = connection_qkd(url, "certs/sae-1.crt", "certs/sae-1.key", ca_path);
    
    if (response) {
        char *key = get_key(response);
        char *key_id = get_key_id(response);
        if (!key || !key_id) return 1;
        
        EncryptedMessage* msg = encrypt_message(key_id, (unsigned char*)key, strlen(key), signed_msg->msg, len);

        PayloadSend* payload = malloc(sizeof(PayloadSend));

        payload->encrypt_msg = msg;
        payload->signed_msg = signed_msg;
        

        if (msg) {
            char *json_msg = build_json_payload(payload);
            if (json_msg) {
                alice(json_msg);
                free(json_msg);
            }
            
        }
        free(payload);
        free(key);
        free(key_id);
    }

    free(acct);
    free(response);
    free(signed_msg);
}