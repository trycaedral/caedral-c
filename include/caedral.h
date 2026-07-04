#ifndef CAEDRAL_H
#define CAEDRAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Return codes */
#define CAEDRAL_OK 0
#define CAEDRAL_ERR_INVALID_ARG -1
#define CAEDRAL_ERR_MEMORY -2
#define CAEDRAL_ERR_NETWORK -3
#define CAEDRAL_ERR_API -4
#define CAEDRAL_ERR_PARSE -5

typedef struct caedral_client caedral_client_t;
typedef struct caedral_response caedral_response_t;

typedef struct caedral_chat_message {
    char *role;
    char *content;
} caedral_chat_message_t;

typedef struct caedral_chat_request {
    char *model;
    caedral_chat_message_t *messages;
    size_t message_count;
    int stream; /* ignored by create(); set automatically for stream API */
} caedral_chat_request_t;

typedef struct caedral_embedding_request {
    char *model;
    char *input;
} caedral_embedding_request_t;

typedef struct caedral_image_request {
    char *model;
    char *prompt;
    int n;
    char *size;
} caedral_image_request_t;

typedef struct caedral_audio_request {
    char *model;
    char *input;
    char *voice;
} caedral_audio_request_t;

typedef struct caedral_rerank_request {
    char *model;
    char *query;
    char **documents;
    size_t document_count;
    int top_n;
} caedral_rerank_request_t;

typedef struct caedral_stream_chunk {
    const char *json; /* valid until callback returns */
    void *user_data;
} caedral_stream_chunk_t;

typedef int (*caedral_stream_callback_fn)(const caedral_stream_chunk_t *chunk, void *user_data);

/* Global last-error state (set by most recent API call on any thread — not thread-safe) */
const char *caedral_get_last_error(void);
int caedral_get_last_status_code(void);
const char *caedral_get_last_error_type(void);

/* Client lifecycle — caller owns returned client; free with caedral_client_free() */
caedral_client_t *caedral_client_new(const char *api_key, const char *base_url);
void caedral_client_free(caedral_client_t *client);

/* Request builders — caller owns; free with corresponding _free() */
caedral_chat_request_t *caedral_chat_request_new(const char *model);
void caedral_chat_request_add_message(caedral_chat_request_t *request, const char *role, const char *content);
void caedral_chat_request_free(caedral_chat_request_t *request);

caedral_embedding_request_t *caedral_embedding_request_new(const char *model, const char *input);
void caedral_embedding_request_free(caedral_embedding_request_t *request);

caedral_image_request_t *caedral_image_request_new(const char *prompt);
void caedral_image_request_free(caedral_image_request_t *request);

caedral_audio_request_t *caedral_audio_request_new(const char *input);
void caedral_audio_request_free(caedral_audio_request_t *request);

caedral_rerank_request_t *caedral_rerank_request_new(const char *query, char **documents, size_t document_count);
void caedral_rerank_request_free(caedral_rerank_request_t *request);

/* API methods — return owned response on success (HTTP 2xx), NULL on failure */
caedral_response_t *caedral_chat_completions_create(caedral_client_t *client, const caedral_chat_request_t *request);
int caedral_chat_completions_create_stream(
    caedral_client_t *client,
    const caedral_chat_request_t *request,
    caedral_stream_callback_fn callback,
    void *user_data);

caedral_response_t *caedral_models_list(caedral_client_t *client);
caedral_response_t *caedral_usage_get(caedral_client_t *client);
caedral_response_t *caedral_embeddings_create(caedral_client_t *client, const caedral_embedding_request_t *request);
caedral_response_t *caedral_images_generate(caedral_client_t *client, const caedral_image_request_t *request);
caedral_response_t *caedral_audio_generate(caedral_client_t *client, const caedral_audio_request_t *request);
caedral_response_t *caedral_rerank_create(caedral_client_t *client, const caedral_rerank_request_t *request);

/* Response accessors — pointers valid until caedral_response_free() */
void caedral_response_free(caedral_response_t *response);
int caedral_response_status_code(const caedral_response_t *response);
const char *caedral_response_body(const caedral_response_t *response);
const char *caedral_response_error_type(const caedral_response_t *response);
const char *caedral_response_error_message(const caedral_response_t *response);

/* Convenience parsers (allocate new strings — caller must free with caedral_free()) */
char *caedral_chat_response_get_content(const caedral_response_t *response);
void caedral_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* CAEDRAL_H */
