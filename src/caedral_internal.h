#ifndef CAEDRAL_INTERNAL_H
#define CAEDRAL_INTERNAL_H

#include "caedral.h"
#include "cJSON.h"

#include <curl/curl.h>

#define CAEDRAL_DEFAULT_BASE_URL "https://api.caedral.com"
#define CAEDRAL_DEFAULT_TIMEOUT_SECONDS 120L

struct caedral_client {
    char *api_key;
    char *base_url;
    CURL *curl;
};

struct caedral_response {
    int status_code;
    char *body;
    char *error_type;
    char *error_message;
};

typedef struct {
    char *data;
    size_t size;
} caedral_buffer_t;

void caedral_set_last_error(int status_code, const char *type, const char *message);
caedral_response_t *caedral_response_new(int status_code, const char *body);
int caedral_http_request(
    caedral_client_t *client,
    const char *method,
    const char *path,
    const char *json_body,
    caedral_buffer_t *out_body,
    long *out_status);
int caedral_http_stream(
    caedral_client_t *client,
    const char *path,
    const char *json_body,
    caedral_stream_callback_fn callback,
    void *user_data);
char *caedral_strdup(const char *value);
char *caedral_strndup(const char *value, size_t len);
void caedral_buffer_free(caedral_buffer_t *buffer);
int caedral_buffer_append(caedral_buffer_t *buffer, const char *data, size_t len);
void caedral_parse_api_error(const char *body, int status_code, char **error_type, char **error_message);

#endif /* CAEDRAL_INTERNAL_H */
