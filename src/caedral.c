#include "caedral_internal.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_last_error[1024];
static int g_last_status_code;
static char g_last_error_type[128];

static size_t caedral_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    caedral_buffer_t *buffer = (caedral_buffer_t *)userp;
    if (caedral_buffer_append(buffer, (const char *)contents, total) != CAEDRAL_OK) {
        return 0;
    }
    return total;
}

void caedral_set_last_error(int status_code, const char *type, const char *message) {
    g_last_status_code = status_code;
    if (type != NULL) {
        strncpy(g_last_error_type, type, sizeof(g_last_error_type) - 1);
        g_last_error_type[sizeof(g_last_error_type) - 1] = '\0';
    } else {
        g_last_error_type[0] = '\0';
    }
    if (message != NULL) {
        strncpy(g_last_error, message, sizeof(g_last_error) - 1);
        g_last_error[sizeof(g_last_error) - 1] = '\0';
    } else {
        g_last_error[0] = '\0';
    }
}

const char *caedral_get_last_error(void) {
    return g_last_error;
}

int caedral_get_last_status_code(void) {
    return g_last_status_code;
}

const char *caedral_get_last_error_type(void) {
    return g_last_error_type;
}

char *caedral_strdup(const char *value) {
    size_t len;
    char *copy;
    if (value == NULL) {
        return NULL;
    }
    len = strlen(value);
    copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, value, len + 1);
    return copy;
}

char *caedral_strndup(const char *value, size_t len) {
    char *copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, value, len);
    copy[len] = '\0';
    return copy;
}

void caedral_free(void *ptr) {
    free(ptr);
}

int caedral_buffer_append(caedral_buffer_t *buffer, const char *data, size_t len) {
    size_t new_size = buffer->size + len + 1;
    char *next = (char *)realloc(buffer->data, new_size);
    if (next == NULL) {
        return CAEDRAL_ERR_MEMORY;
    }
    buffer->data = next;
    memcpy(buffer->data + buffer->size, data, len);
    buffer->size += len;
    buffer->data[buffer->size] = '\0';
    return CAEDRAL_OK;
}

void caedral_buffer_free(caedral_buffer_t *buffer) {
    if (buffer == NULL) {
        return;
    }
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
}

static char *caedral_trim_trailing_slash(const char *base_url) {
    size_t len;
    char *copy;
    if (base_url == NULL || base_url[0] == '\0') {
        return caedral_strdup(CAEDRAL_DEFAULT_BASE_URL);
    }
    len = strlen(base_url);
    copy = caedral_strdup(base_url);
    if (copy == NULL) {
        return NULL;
    }
    while (len > 0 && copy[len - 1] == '/') {
        copy[len - 1] = '\0';
        len--;
    }
    return copy;
}

caedral_client_t *caedral_client_new(const char *api_key, const char *base_url) {
    caedral_client_t *client;
    if (api_key == NULL || api_key[0] == '\0') {
        caedral_set_last_error(0, "invalid_argument", "api_key is required");
        return NULL;
    }
    client = (caedral_client_t *)calloc(1, sizeof(caedral_client_t));
    if (client == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to allocate client");
        return NULL;
    }
    client->api_key = caedral_strdup(api_key);
    client->base_url = caedral_trim_trailing_slash(base_url);
    if (client->api_key == NULL || client->base_url == NULL) {
        caedral_client_free(client);
        caedral_set_last_error(0, "memory_error", "failed to allocate client fields");
        return NULL;
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);
    client->curl = curl_easy_init();
    if (client->curl == NULL) {
        caedral_client_free(client);
        caedral_set_last_error(0, "network_error", "failed to initialize curl");
        return NULL;
    }
    return client;
}

void caedral_client_free(caedral_client_t *client) {
    if (client == NULL) {
        return;
    }
    if (client->curl != NULL) {
        curl_easy_cleanup(client->curl);
    }
    free(client->api_key);
    free(client->base_url);
    free(client);
}

caedral_chat_request_t *caedral_chat_request_new(const char *model) {
    caedral_chat_request_t *request;
    if (model == NULL || model[0] == '\0') {
        caedral_set_last_error(0, "invalid_argument", "model is required");
        return NULL;
    }
    request = (caedral_chat_request_t *)calloc(1, sizeof(caedral_chat_request_t));
    if (request == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to allocate chat request");
        return NULL;
    }
    request->model = caedral_strdup(model);
    if (request->model == NULL) {
        free(request);
        caedral_set_last_error(0, "memory_error", "failed to allocate model");
        return NULL;
    }
    return request;
}

void caedral_chat_request_add_message(caedral_chat_request_t *request, const char *role, const char *content) {
    caedral_chat_message_t *next;
    if (request == NULL || role == NULL) {
        return;
    }
    next = (caedral_chat_message_t *)realloc(request->messages, (request->message_count + 1) * sizeof(caedral_chat_message_t));
    if (next == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to grow message list");
        return;
    }
    request->messages = next;
    request->messages[request->message_count].role = caedral_strdup(role);
    request->messages[request->message_count].content = caedral_strdup(content != NULL ? content : "");
    request->message_count++;
}

void caedral_chat_request_free(caedral_chat_request_t *request) {
    size_t i;
    if (request == NULL) {
        return;
    }
    for (i = 0; i < request->message_count; i++) {
        free(request->messages[i].role);
        free(request->messages[i].content);
    }
    free(request->messages);
    free(request->model);
    free(request);
}

caedral_embedding_request_t *caedral_embedding_request_new(const char *model, const char *input) {
    caedral_embedding_request_t *request = (caedral_embedding_request_t *)calloc(1, sizeof(caedral_embedding_request_t));
    if (request == NULL) {
        return NULL;
    }
    request->model = caedral_strdup(model != NULL ? model : "caedral-embed");
    request->input = caedral_strdup(input);
    return request;
}

void caedral_embedding_request_free(caedral_embedding_request_t *request) {
    if (request == NULL) {
        return;
    }
    free(request->model);
    free(request->input);
    free(request);
}

caedral_image_request_t *caedral_image_request_new(const char *prompt) {
    caedral_image_request_t *request = (caedral_image_request_t *)calloc(1, sizeof(caedral_image_request_t));
    if (request == NULL) {
        return NULL;
    }
    request->prompt = caedral_strdup(prompt);
    return request;
}

void caedral_image_request_free(caedral_image_request_t *request) {
    if (request == NULL) {
        return;
    }
    free(request->model);
    free(request->prompt);
    free(request->size);
    free(request);
}

caedral_audio_request_t *caedral_audio_request_new(const char *input) {
    caedral_audio_request_t *request = (caedral_audio_request_t *)calloc(1, sizeof(caedral_audio_request_t));
    if (request == NULL) {
        return NULL;
    }
    request->input = caedral_strdup(input);
    return request;
}

void caedral_audio_request_free(caedral_audio_request_t *request) {
    if (request == NULL) {
        return;
    }
    free(request->model);
    free(request->input);
    free(request->voice);
    free(request);
}

caedral_rerank_request_t *caedral_rerank_request_new(const char *query, char **documents, size_t document_count) {
    caedral_rerank_request_t *request = (caedral_rerank_request_t *)calloc(1, sizeof(caedral_rerank_request_t));
    if (request == NULL) {
        return NULL;
    }
    request->query = caedral_strdup(query);
    request->documents = documents;
    request->document_count = document_count;
    return request;
}

void caedral_rerank_request_free(caedral_rerank_request_t *request) {
    if (request == NULL) {
        return;
    }
    free(request->model);
    free(request->query);
    free(request);
}

void caedral_parse_api_error(const char *body, int status_code, char **error_type, char **error_message) {
    cJSON *root = cJSON_Parse(body != NULL ? body : "");
    cJSON *error = root != NULL ? cJSON_GetObjectItemCaseSensitive(root, "error") : NULL;
    if (error != NULL && cJSON_IsObject(error)) {
        cJSON *type = cJSON_GetObjectItemCaseSensitive(error, "type");
        cJSON *message = cJSON_GetObjectItemCaseSensitive(error, "message");
        cJSON *code = cJSON_GetObjectItemCaseSensitive(error, "code");
        if (cJSON_IsString(type) && type->valuestring != NULL) {
            *error_type = caedral_strdup(type->valuestring);
        }
        if (cJSON_IsString(message) && message->valuestring != NULL) {
            *error_message = caedral_strdup(message->valuestring);
        }
        if (cJSON_IsNumber(code) && code->valueint != 0) {
            status_code = code->valueint;
        }
    } else if (root != NULL) {
        cJSON *message = cJSON_GetObjectItemCaseSensitive(root, "message");
        if (cJSON_IsString(message) && message->valuestring != NULL) {
            *error_message = caedral_strdup(message->valuestring);
        }
    }
    if (*error_message == NULL) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Request failed with status %d", status_code);
        *error_message = caedral_strdup(buffer);
    }
    if (*error_type == NULL) {
        *error_type = caedral_strdup("unknown");
    }
    cJSON_Delete(root);
}

caedral_response_t *caedral_response_new(int status_code, const char *body) {
    caedral_response_t *response = (caedral_response_t *)calloc(1, sizeof(caedral_response_t));
    if (response == NULL) {
        return NULL;
    }
    response->status_code = status_code;
    response->body = caedral_strdup(body != NULL ? body : "");
    if (response->body == NULL) {
        free(response);
        return NULL;
    }
    if (status_code >= 400) {
        caedral_parse_api_error(response->body, status_code, &response->error_type, &response->error_message);
    }
    return response;
}

void caedral_response_free(caedral_response_t *response) {
    if (response == NULL) {
        return;
    }
    free(response->body);
    free(response->error_type);
    free(response->error_message);
    free(response);
}

int caedral_response_status_code(const caedral_response_t *response) {
    return response != NULL ? response->status_code : 0;
}

const char *caedral_response_body(const caedral_response_t *response) {
    return response != NULL ? response->body : NULL;
}

const char *caedral_response_error_type(const caedral_response_t *response) {
    return response != NULL ? response->error_type : NULL;
}

const char *caedral_response_error_message(const caedral_response_t *response) {
    return response != NULL ? response->error_message : NULL;
}

char *caedral_chat_response_get_content(const caedral_response_t *response) {
    cJSON *root;
    cJSON *choices;
    cJSON *first;
    cJSON *message;
    cJSON *content;
    char *result = NULL;
    if (response == NULL || response->body == NULL) {
        return NULL;
    }
    root = cJSON_Parse(response->body);
    if (root == NULL) {
        return NULL;
    }
    choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0) {
        cJSON_Delete(root);
        return NULL;
    }
    first = cJSON_GetArrayItem(choices, 0);
    message = cJSON_GetObjectItemCaseSensitive(first, "message");
    content = message != NULL ? cJSON_GetObjectItemCaseSensitive(message, "content") : NULL;
    if (cJSON_IsString(content) && content->valuestring != NULL) {
        result = caedral_strdup(content->valuestring);
    }
    cJSON_Delete(root);
    return result;
}

static char *caedral_build_url(caedral_client_t *client, const char *path) {
    size_t len = strlen(client->base_url) + strlen(path) + 1;
    char *url = (char *)malloc(len);
    if (url == NULL) {
        return NULL;
    }
    snprintf(url, len, "%s%s", client->base_url, path);
    return url;
}

static struct curl_slist *caedral_auth_headers(caedral_client_t *client, int json_body) {
    char auth_header[512];
    struct curl_slist *headers = NULL;
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", client->api_key);
    headers = curl_slist_append(headers, auth_header);
    if (json_body) {
        headers = curl_slist_append(headers, "Content-Type: application/json");
    }
    return headers;
}

int caedral_http_request(
    caedral_client_t *client,
    const char *method,
    const char *path,
    const char *json_body,
    caedral_buffer_t *out_body,
    long *out_status) {
    char *url = NULL;
    struct curl_slist *headers = NULL;
    CURLcode code;
    int json = json_body != NULL;

    if (client == NULL || path == NULL || out_body == NULL || out_status == NULL) {
        caedral_set_last_error(0, "invalid_argument", "invalid HTTP request arguments");
        return CAEDRAL_ERR_INVALID_ARG;
    }

    url = caedral_build_url(client, path);
    if (url == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to build URL");
        return CAEDRAL_ERR_MEMORY;
    }

    out_body->data = NULL;
    out_body->size = 0;
    headers = caedral_auth_headers(client, json);

    curl_easy_reset(client->curl);
    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_CUSTOMREQUEST, method);
    curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, caedral_write_callback);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, out_body);
    curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, CAEDRAL_DEFAULT_TIMEOUT_SECONDS);
    curl_easy_setopt(client->curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (json) {
        curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, json_body);
    }

    code = curl_easy_perform(client->curl);
    curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, out_status);
    curl_slist_free_all(headers);
    free(url);

    if (code != CURLE_OK) {
        caedral_set_last_error(0, "network_error", curl_easy_strerror(code));
        caedral_buffer_free(out_body);
        return CAEDRAL_ERR_NETWORK;
    }
    return CAEDRAL_OK;
}

typedef struct {
    caedral_stream_callback_fn callback;
    void *user_data;
    char line_buffer[8192];
    size_t line_len;
    caedral_buffer_t raw_body;
} caedral_stream_ctx_t;

static int caedral_stream_write(const char *data, size_t len, caedral_stream_ctx_t *ctx) {
    size_t i;
    for (i = 0; i < len; i++) {
        char ch = data[i];
        if (ch == '\n') {
            ctx->line_buffer[ctx->line_len] = '\0';
            if (strncmp(ctx->line_buffer, "data:", 5) == 0) {
                const char *payload = ctx->line_buffer + 5;
                while (*payload == ' ') {
                    payload++;
                }
                if (payload[0] != '\0' && strcmp(payload, "[DONE]") != 0) {
                    caedral_stream_chunk_t chunk;
                    chunk.json = payload;
                    chunk.user_data = ctx->user_data;
                    if (ctx->callback(&chunk, ctx->user_data) != 0) {
                        return 1;
                    }
                }
            }
            ctx->line_len = 0;
        } else if (ctx->line_len + 1 < sizeof(ctx->line_buffer)) {
            ctx->line_buffer[ctx->line_len++] = ch;
        }
    }
    return 0;
}

static size_t caedral_stream_callback_fn(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    caedral_stream_ctx_t *ctx = (caedral_stream_ctx_t *)userp;
    if (caedral_buffer_append(&ctx->raw_body, (const char *)contents, total) != CAEDRAL_OK) {
        return 0;
    }
    if (caedral_stream_write((const char *)contents, total, ctx) != 0) {
        return 0;
    }
    return total;
}

int caedral_http_stream(
    caedral_client_t *client,
    const char *path,
    const char *json_body,
    caedral_stream_callback_fn callback,
    void *user_data) {
    char *url = NULL;
    struct curl_slist *headers = NULL;
    CURLcode code;
    long status = 0;
    caedral_stream_ctx_t ctx;

    if (client == NULL || path == NULL || json_body == NULL || callback == NULL) {
        caedral_set_last_error(0, "invalid_argument", "invalid stream arguments");
        return CAEDRAL_ERR_INVALID_ARG;
    }

    url = caedral_build_url(client, path);
    headers = caedral_auth_headers(client, 1);
    memset(&ctx, 0, sizeof(ctx));
    ctx.callback = callback;
    ctx.user_data = user_data;

    curl_easy_reset(client->curl);
    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, caedral_stream_callback_fn);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(client->curl, CURLOPT_TIMEOUT, CAEDRAL_DEFAULT_TIMEOUT_SECONDS);

    code = curl_easy_perform(client->curl);
    curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers);
    free(url);

    if (code != CURLE_OK) {
        caedral_buffer_free(&ctx.raw_body);
        caedral_set_last_error(0, "network_error", curl_easy_strerror(code));
        return CAEDRAL_ERR_NETWORK;
    }
    if (status >= 400) {
        char *error_type = NULL;
        char *error_message = NULL;
        caedral_parse_api_error(ctx.raw_body.data, (int)status, &error_type, &error_message);
        caedral_set_last_error((int)status, error_type, error_message);
        free(error_type);
        free(error_message);
        caedral_buffer_free(&ctx.raw_body);
        return CAEDRAL_ERR_API;
    }
    caedral_buffer_free(&ctx.raw_body);
    return CAEDRAL_OK;
}

static char *caedral_chat_request_to_json(const caedral_chat_request_t *request, int stream) {
    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_CreateArray();
    size_t i;
    char *json;
    if (root == NULL || messages == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(messages);
        return NULL;
    }
    cJSON_AddStringToObject(root, "model", request->model);
    cJSON_AddBoolToObject(root, "stream", stream ? 1 : 0);
    for (i = 0; i < request->message_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "role", request->messages[i].role);
        cJSON_AddStringToObject(item, "content", request->messages[i].content);
        cJSON_AddItemToArray(messages, item);
    }
    cJSON_AddItemToObject(root, "messages", messages);
    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

static caedral_response_t *caedral_do_json_post(caedral_client_t *client, const char *path, char *json_body) {
    caedral_buffer_t body = {0};
    long status = 0;
    caedral_response_t *response;
    int rc = caedral_http_request(client, "POST", path, json_body, &body, &status);
    free(json_body);
    if (rc != CAEDRAL_OK) {
        return NULL;
    }
    response = caedral_response_new((int)status, body.data);
    caedral_buffer_free(&body);
    if (response == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to allocate response");
        return NULL;
    }
    if (status >= 400) {
        caedral_set_last_error(response->status_code, response->error_type, response->error_message);
        caedral_response_free(response);
        return NULL;
    }
    return response;
}

caedral_response_t *caedral_chat_completions_create(caedral_client_t *client, const caedral_chat_request_t *request) {
    char *json;
    if (client == NULL || request == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client and request are required");
        return NULL;
    }
    json = caedral_chat_request_to_json(request, 0);
    if (json == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to encode chat request");
        return NULL;
    }
    return caedral_do_json_post(client, "/v1/chat/completions", json);
}

int caedral_chat_completions_create_stream(
    caedral_client_t *client,
    const caedral_chat_request_t *request,
    caedral_stream_callback_fn callback,
    void *user_data) {
    char *json;
    int rc;
    if (client == NULL || request == NULL || callback == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client, request, and callback are required");
        return CAEDRAL_ERR_INVALID_ARG;
    }
    json = caedral_chat_request_to_json(request, 1);
    if (json == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to encode chat request");
        return CAEDRAL_ERR_MEMORY;
    }
    rc = caedral_http_stream(client, "/v1/chat/completions", json, callback, user_data);
    free(json);
    return rc;
}

caedral_response_t *caedral_models_list(caedral_client_t *client) {
    caedral_buffer_t body = {0};
    long status = 0;
    caedral_response_t *response;
    if (client == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client is required");
        return NULL;
    }
    if (caedral_http_request(client, "GET", "/v1/models", NULL, &body, &status) != CAEDRAL_OK) {
        return NULL;
    }
    response = caedral_response_new((int)status, body.data);
    caedral_buffer_free(&body);
    if (response == NULL) {
        return NULL;
    }
    if (status >= 400) {
        caedral_set_last_error(response->status_code, response->error_type, response->error_message);
        caedral_response_free(response);
        return NULL;
    }
    return response;
}

caedral_response_t *caedral_usage_get(caedral_client_t *client) {
    caedral_buffer_t body = {0};
    long status = 0;
    caedral_response_t *response;
    if (client == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client is required");
        return NULL;
    }
    if (caedral_http_request(client, "GET", "/v1/usage", NULL, &body, &status) != CAEDRAL_OK) {
        return NULL;
    }
    response = caedral_response_new((int)status, body.data);
    caedral_buffer_free(&body);
    if (response == NULL) {
        return NULL;
    }
    if (status >= 400) {
        caedral_set_last_error(response->status_code, response->error_type, response->error_message);
        caedral_response_free(response);
        return NULL;
    }
    return response;
}

static char *caedral_embedding_request_to_json(const caedral_embedding_request_t *request) {
    cJSON *root = cJSON_CreateObject();
    char *json;
    if (root == NULL) {
        return NULL;
    }
    cJSON_AddStringToObject(root, "model", request->model != NULL ? request->model : "caedral-embed");
    cJSON_AddStringToObject(root, "input", request->input);
    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

caedral_response_t *caedral_embeddings_create(caedral_client_t *client, const caedral_embedding_request_t *request) {
    char *json;
    if (client == NULL || request == NULL || request->input == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client and embedding request are required");
        return NULL;
    }
    json = caedral_embedding_request_to_json(request);
    if (json == NULL) {
        caedral_set_last_error(0, "memory_error", "failed to encode embedding request");
        return NULL;
    }
    return caedral_do_json_post(client, "/v1/embeddings", json);
}

static char *caedral_image_request_to_json(const caedral_image_request_t *request) {
    cJSON *root = cJSON_CreateObject();
    char *json;
    if (root == NULL) {
        return NULL;
    }
    if (request->model != NULL) {
        cJSON_AddStringToObject(root, "model", request->model);
    }
    cJSON_AddStringToObject(root, "prompt", request->prompt);
    if (request->n > 0) {
        cJSON_AddNumberToObject(root, "n", request->n);
    }
    if (request->size != NULL) {
        cJSON_AddStringToObject(root, "size", request->size);
    }
    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

caedral_response_t *caedral_images_generate(caedral_client_t *client, const caedral_image_request_t *request) {
    char *json;
    if (client == NULL || request == NULL || request->prompt == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client and image request are required");
        return NULL;
    }
    json = caedral_image_request_to_json(request);
    if (json == NULL) {
        return NULL;
    }
    return caedral_do_json_post(client, "/v1/images/generations", json);
}

static char *caedral_audio_request_to_json(const caedral_audio_request_t *request) {
    cJSON *root = cJSON_CreateObject();
    char *json;
    if (root == NULL) {
        return NULL;
    }
    if (request->model != NULL) {
        cJSON_AddStringToObject(root, "model", request->model);
    }
    cJSON_AddStringToObject(root, "input", request->input);
    if (request->voice != NULL) {
        cJSON_AddStringToObject(root, "voice", request->voice);
    }
    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

caedral_response_t *caedral_audio_generate(caedral_client_t *client, const caedral_audio_request_t *request) {
    char *json;
    if (client == NULL || request == NULL || request->input == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client and audio request are required");
        return NULL;
    }
    json = caedral_audio_request_to_json(request);
    if (json == NULL) {
        return NULL;
    }
    return caedral_do_json_post(client, "/v1/audio/speech", json);
}

static char *caedral_rerank_request_to_json(const caedral_rerank_request_t *request) {
    cJSON *root = cJSON_CreateObject();
    cJSON *documents = cJSON_CreateArray();
    size_t i;
    char *json;
    if (root == NULL || documents == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(documents);
        return NULL;
    }
    if (request->model != NULL) {
        cJSON_AddStringToObject(root, "model", request->model);
    }
    cJSON_AddStringToObject(root, "query", request->query);
    for (i = 0; i < request->document_count; i++) {
        cJSON_AddItemToArray(documents, cJSON_CreateString(request->documents[i]));
    }
    cJSON_AddItemToObject(root, "documents", documents);
    if (request->top_n > 0) {
        cJSON_AddNumberToObject(root, "top_n", request->top_n);
    }
    json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

caedral_response_t *caedral_rerank_create(caedral_client_t *client, const caedral_rerank_request_t *request) {
    char *json;
    if (client == NULL || request == NULL || request->query == NULL) {
        caedral_set_last_error(0, "invalid_argument", "client and rerank request are required");
        return NULL;
    }
    json = caedral_rerank_request_to_json(request);
    if (json == NULL) {
        return NULL;
    }
    return caedral_do_json_post(client, "/v1/rerank", json);
}
