#ifndef SIGN_DILITHIUM
#define SIGN_DILITHIUM

typedef struct SignDilithium{
    unsigned char* msg;
    size_t mlen;
    unsigned char* ctx;
    size_t ctxlen;
    unsigned char* pk;
    unsigned char* sign;
    size_t siglen;
}SignDilithium;

SignDilithium* sign(unsigned char *msg, size_t mlen, char *ctx);
int verify(unsigned char *sign, unsigned char *msg, char *ctx, unsigned char *pk);

#endif
