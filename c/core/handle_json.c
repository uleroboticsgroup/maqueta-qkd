/** 
 * Handle the json responses and get the key_id and the key.
 * **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "handle_json.h"
#include <stdio.h>

//Extract the field from the json response
static char *extract_first_key_field(const char *json_data, const char *field) {
    if (!json_data) return NULL;
    cJSON *root = cJSON_Parse(json_data);
    if (!root) return NULL;

    char *result = NULL;
    cJSON *keys = cJSON_GetObjectItemCaseSensitive(root, "keys");
    if (cJSON_IsArray(keys) && cJSON_GetArraySize(keys) > 0) {
        cJSON *first_key = cJSON_GetArrayItem(keys, 0);
        cJSON *item = cJSON_GetObjectItemCaseSensitive(first_key, field);
        if (cJSON_IsString(item) && item->valuestring) {
            result = strdup(item->valuestring);
        }
    }

    cJSON_Delete(root);
    return result;
}

//Get the key_id from the json response
char *get_key_id(const char *json_data) {
    return extract_first_key_field(json_data, "key_ID");
}

//Get the key from the json response
char *get_key(const char *json_data) {
    return extract_first_key_field(json_data, "key");
}

//Get the context value
char *get_ctx(const char *json_data) {
    return extract_first_key_field(json_data, "ctx");
}

//Get the signatura value
char *get_sign(const char *json_data) {
    return extract_first_key_field(json_data, "sign");
}

//Get the ciphertext
char *get_ciphertext(const char *json_data) {
    return extract_first_key_field(json_data, "ciphertext");
}

//Get the public key
char *get_pk(const char *json_data) {
    return extract_first_key_field(json_data, "pk");
}

//Get the ACCOUNT_ID enviroment value
char *account_id(void) {
    char *env_val = getenv("ACCOUNT_ID");
    if (env_val != NULL) {
        return strdup(env_val);
    }
    return NULL; 
}