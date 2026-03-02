/** 
 * Handle the connection with the https://qukaydee.com and get the response.
 * The response is in json format and contains the key_id and the key.
 * The key is used to encrypt and decrypt messages.
 * **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "handle_json.h"
#include "cryptografy.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

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

// Connects to the QKD API and retrieves the key information.
char *connection_qkd(const char *url, const char *cert_path, const char *key_path, const char *ca_path) {
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
