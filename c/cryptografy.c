#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "dilithium/ref/api.h" 



int encrypt_dilithium() {
    uint8_t pk[pqcrystals_dilithium2_PUBLICKEYBYTES];
    uint8_t sk[pqcrystals_dilithium2_SECRETKEYBYTES];
    
    const char *msg = "Hello, world!";
    size_t msglen = strlen(msg);
    
    uint8_t *sm = malloc(pqcrystals_dilithium2_BYTES + msglen);
    size_t smlen;
    
    uint8_t *recovered_msg = malloc(msglen + pqcrystals_dilithium2_BYTES);
    size_t recovered_msglen;

    if (!sm || !recovered_msg) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    if (pqcrystals_dilithium2_ref_keypair(pk, sk) != 0) {
        printf("Keypair generation failed\n");
        free(sm); free(recovered_msg);
        return 1;
    }
    printf("Keypair generated successfully.\n");

    pqcrystals_dilithium2_ref(sm, &smlen, (const uint8_t *)msg, msglen,NULL,0, sk);
    printf("Message signed. Signed message length: %zu\n", smlen);

    if (pqcrystals_dilithium2_ref_open(recovered_msg, &recovered_msglen, sm, smlen,NULL,0, pk) != 0) {
        printf("Signature verification failed!\n");
    } else {
        recovered_msg[recovered_msglen] = '\0';
        printf("Verification successful! Message: %s\n", recovered_msg);
    }

    free(sm);
    free(recovered_msg);
    return 0;
}