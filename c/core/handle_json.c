/** 
 * Handle the json responses and get the key_id and the key.
 * **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "handle_json.h"

//Read the content of the file config.json to get the account id
static char *read_file(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return NULL;

    FILE *f = fopen(path, "rb");

    if (!f) return NULL;
    size_t size = st.st_size;
    char *buf = malloc(size + 1);

    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, size, f) != size) { free(buf); fclose(f); return NULL; }

    buf[size] = '\0';
    fclose(f);
    return buf;
}

//Get the values of the configuration file
static char *get_config_value(const char *path, const char *key) {
    char *data = read_file(path);
    if (!data) {
        fprintf(stderr, "Failed to read %s\n", path);
        return NULL;
    }
    cJSON *json = cJSON_Parse(data);
    free(data);
    if (!json) {
        fprintf(stderr, "Failed to parse %s\n", path);
        return NULL;
    }
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    char *result = NULL;
    if (cJSON_IsString(item) && item->valuestring) {
        size_t len = strlen(item->valuestring);
        result = malloc(len + 1);
        if (result) memcpy(result, item->valuestring, len + 1);
    }
    cJSON_Delete(json);
    return result;
}

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

char *get_ctx(const char *json_data) {
    return extract_first_key_field(json_data, "ctx");
}

char *get_sign(const char *json_data) {
    return extract_first_key_field(json_data, "sign");
}

char *get_ciphertext(const char *json_data) {
    return extract_first_key_field(json_data, "ciphertext");
}
char *get_pk(const char *json_data) {
    return extract_first_key_field(json_data, "pk");
}

//Get the account id
char *account_id(void) {
    return get_config_value("./core/config.json", "ACCOUNT_ID");
}