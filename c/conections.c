#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define ACCOUNT_ID "3009"

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("Not enough memory\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int main(void) {

    // unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    // unsigned char sk[CRYPTO_SECRETKEYBYTES];

    // if (crypto_sign_keypair(pk, sk) != 0) {
    //     printf("Dilithium keypair generation failed\n");
    //     return -1;
    // }

    // printf("Dilithium keypair generated\n");

    CURL *curl;
    CURLcode res;

    char url[256];
    snprintf(url, sizeof(url),"https://kme-1.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-2/status",ACCOUNT_ID);

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_SSLCERT, "../certs/sae-1.crt");
        curl_easy_setopt(curl, CURLOPT_SSLKEY, "../certs/sae-1.key");

        char ca_cert_path[256];
        snprintf(ca_cert_path, sizeof(ca_cert_path),"../certs/account-%s-server-ca-qukaydee-com.crt",ACCOUNT_ID);

        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_cert_path);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "Error en la conexión: %s\n",curl_easy_strerror(res));
        } else {
            long http_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            if(http_code == 200) {
                printf("%s\n", chunk.memory);
            } else {
                printf("Error: %ld\n", http_code);
                printf("%s\n", chunk.memory);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    curl_global_cleanup();

    return 0;
}