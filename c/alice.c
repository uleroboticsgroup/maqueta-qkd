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
#include <openssl/opensslv.h>
#include <openssl/crypto.h>
#include <sys/time.h>


#define URL_GETKEY "https://kme-1.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-2%s"

/** Alice send the msg to Bob. The msg have two parts msg and key_id to decrypt the msg.
 * @msg: The msg to be sent to Bob. It is in json format and contains the encrypted msg and the key_id.
 */
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

    close(sock);
    return 0;
}

/** Signs and encrypts a message before sending it to Bob. 
 *  @buffer: The message to be signed and encrypted.
 *  @len: The length of the message.
*/
static int sign_encrypt(unsigned char* buffer, size_t len){
    // OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG | OPENSSL_INIT_LOAD_CRYPTO_STRINGS, NULL);
    // size_t len = 0;
    // unsigned char *file = read_file("rosbag2.db3", &len);
    // if (!file) return 1;
    char *ctx = "To Bob";

    SignDilithium* signed_msg = sign(buffer, len, ctx);

    //Inicio timer
    struct timeval start, end;
    gettimeofday(&start, NULL);

    char *acct = account_id();
    char url[1024],ca_path[512];
    snprintf(ca_path, sizeof(ca_path), "certs/account-%s-server-ca-qukaydee-com.crt", acct);
    snprintf(url, sizeof(url), URL_GETKEY, acct, "/enc_keys?number=2&size=256");
    char *response = connection_qkd(url, "certs/sae-1.crt", "certs/sae-1.key", ca_path);
    
    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed_ms = (seconds * 1000.0) + (microseconds / 1000.0);

    FILE *log_file = fopen("crypto_metricsQKD.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "QKD:%.4f\n", elapsed_ms);
        fclose(log_file);
    } else {
        printf("Error al abrir el archivo de logs.\n");
    }

    if (response) {
        char *key = get_key(response);
        char *key_id = get_key_id(response);
        if (!key || !key_id) return 1;
        
        EncryptedMessage* msg = encrypt_message(key_id, (unsigned char*)key, signed_msg->msg, len);

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
        free(signed_msg);
        free(response);
    }
    return 0;
}

/** Handles the connection from the rosbag and processes the incoming messages.
 *  @client_socket: The socket connected to the rosbag.
 */
static int handle_rosbag_connection(int client_socket,int *counter) {
    while (1) {
        //Inicio timer
        struct timeval start, end;
        gettimeofday(&start, NULL);
        uint32_t net_len = 0;
        ssize_t n = read(client_socket, &net_len, sizeof(net_len));
        uint32_t payload_len = ntohl(net_len);
        unsigned char *buffer = malloc(payload_len + 1);
        size_t total_read = 0;

        if (n == 0) {
            break;
        }
        if (n < 0) {
            perror("Failed to read payload length");
            return 1;
        }

        while (total_read < payload_len) {
            ssize_t bytes_read = read(client_socket, buffer + total_read, payload_len - total_read);
            if (bytes_read <= 0) {
                free(buffer);
                return 1;
            }
            total_read += (size_t)bytes_read;
        }
        buffer[payload_len] = '\0';
        if (strcmp((char*)buffer, "{\"end\":true}") == 0) {
            sign_encrypt(buffer, payload_len);
            free(buffer);
            return 1;
        }
        
        gettimeofday(&end, NULL);
        long seconds = end.tv_sec - start.tv_sec;
        long microseconds = end.tv_usec - start.tv_usec;
        double elapsed_ms = (seconds * 1000.0) + (microseconds / 1000.0);
        
        FILE *log_file = fopen("crypto_metricsRosbagMsg.log", "a");
        if (log_file != NULL) {
            fprintf(log_file, "Rosbag:%.4f\n", elapsed_ms);
            fclose(log_file);
        } else {
            printf("Error al abrir el archivo de logs.\n");
        }
        (*counter)++;
        printf("Received message %d from rosbag\n", *counter);
        sign_encrypt(buffer, payload_len);
        free(buffer);
    }

    return 0;
}


/** Starts the rosbag server to receive messages and process them.
 *  @port: The port on which the server will listen.
 */
static int start_rosbag_server(uint16_t port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Alice TCP server listening on port %u\n", port);

    int counter = 0;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        if (handle_rosbag_connection(client_sock, &counter) == 1) {
            break;
        }
        close(client_sock);
    }

    close(server_fd);
    return 0;
}

//Start the server to receive the msg from rosbag, then encrypt and sign the msg and send it to Bob.
int main(void){
    return start_rosbag_server(8081);
}