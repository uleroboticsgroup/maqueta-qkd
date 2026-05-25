#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "api.h"
#include <sys/time.h>

typedef struct SignDilithium{
    unsigned char* msg;
    size_t msglen;
    unsigned char* ctx;
    size_t ctxlen;
    unsigned char* pk;
    unsigned char* sign;
    size_t siglen;
}SignDilithium;

/**
 * DILITHIUM DEPENDENCY: Secure Random Number Generator
 */
void randombytes(uint8_t *out, size_t outlen) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) {
        perror("Failed to open /dev/urandom");
        exit(1);
    }
    if (fread(out, 1, outlen, f) != outlen) {
        perror("Failed to read random bytes");
        exit(1);
    }
    fclose(f);
}

/**
 * Sign the msg recived with the ctx.
 * MSG: Message who want to be send
 * CTX: Identificator to who the msg is send
 */
SignDilithium* sign(unsigned char *msg, size_t msglen, char *ctx) {
    //Inicio timer
    struct timeval start, end;
    gettimeofday(&start, NULL);

    SignDilithium* signed_msg = malloc(sizeof(SignDilithium));
    if(!signed_msg) return NULL;

    signed_msg->pk = malloc(pqcrystals_dilithium5_PUBLICKEYBYTES);
    signed_msg->sign = malloc(pqcrystals_dilithium5_BYTES);

    unsigned char sk[pqcrystals_dilithium5_SECRETKEYBYTES];
    
    size_t ctxlen = strlen(ctx); 

    pqcrystals_dilithium5_ref_keypair(signed_msg->pk, sk);
    pqcrystals_dilithium5_ref_signature(signed_msg->sign, &signed_msg->siglen, msg, msglen, (const unsigned char*)ctx, ctxlen, sk);


    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed_ms = (seconds * 1000.0) + (microseconds / 1000.0);

    FILE *log_file = fopen("crypto_metricsSign.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "FIRMA:%.4f\n", elapsed_ms);
        fclose(log_file);
    } else {
        printf("Error al abrir el archivo de logs.\n");
    }

    signed_msg->msg = msg;
    signed_msg->msglen = msglen;
    signed_msg->ctx = (unsigned char*)ctx;
    signed_msg->ctxlen = ctxlen;

    return signed_msg;
}

/**
 * Verify the sign of the msg
 * SIGN: Signature recived
 * MSG: Message recived
 * PK: Public key
 * CTX: Context recived
 */
int verify(unsigned char *sign, unsigned char *msg,size_t mlen, char *ctx, unsigned char *pk) {
    //Inicio timer
    struct timeval start, end;
    gettimeofday(&start, NULL);

    size_t siglen = pqcrystals_dilithium5_BYTES; 
    size_t ctxlen = strlen(ctx);

    int result = pqcrystals_dilithium5_ref_verify((const uint8_t*)sign,siglen,(const uint8_t*)msg, mlen,(const uint8_t*)ctx,ctxlen,(const uint8_t*)pk);

    gettimeofday(&end, NULL);
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed_ms = (seconds * 1000.0) + (microseconds / 1000.0);

    FILE *log_file = fopen("crypto_metricsVerify.log", "a");
    if (log_file != NULL) {
        fprintf(log_file, "VERIFICACION:%.4f\n", elapsed_ms);
        fclose(log_file);
    } else {
        printf("Error al abrir el archivo de logs.\n");
    }
    return result;
} 

