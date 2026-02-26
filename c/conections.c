#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "handle_json.h"

#define URL_GETKEY "https://kme-1.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-2%s"
#define URL_GETKEYBYID "https://kme-2.acct-%s.etsi-qkd-api.qukaydee.com/api/v1/keys/sae-1/dec_keys?key_ID=%s"


struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static char *connection(const char *url, const char *cert_path, const char *key_path, const char *ca_path) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk = { .memory = malloc(1), .size = 0 };

    if (!chunk.memory) return NULL;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = curl_slist_append(NULL, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        curl_easy_setopt(curl, CURLOPT_SSLCERT, cert_path);
        curl_easy_setopt(curl, CURLOPT_SSLKEY, key_path);
        curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
        
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            chunk.memory = NULL;
        } 

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return chunk.memory;
}

int main(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    char *acct = account_id();
    if (!acct) return 1;

    char url[1024];
    char ca_path[512];
    snprintf(ca_path, sizeof(ca_path), "../certs/account-%s-server-ca-qukaydee-com.crt", acct);

    snprintf(url, sizeof(url), URL_GETKEY, acct, "/enc_keys?number=2&size=1024");
    char *response = connection(url, "../certs/sae-1.crt", "../certs/sae-1.key", ca_path);
    printf("Key: %s\n", get_key(response));

    if (response) {
        char *k_id = get_key_id(response); 
        
        if (k_id) {
            snprintf(url, sizeof(url), URL_GETKEYBYID, acct, k_id);
            char *dec_response = connection(url, "../certs/sae-2.crt", "../certs/sae-2.key", ca_path);
            
            if (dec_response) {
                printf("Get the key with ID: %s\n", dec_response);
                free(dec_response);
            }
        }
        free(response);
    }

    free(acct);
    curl_global_cleanup();
    return 0;
}