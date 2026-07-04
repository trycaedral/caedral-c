#include "caedral.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_failed = 0;

static void expect_true(int condition, const char *name) {
    tests_run++;
    if (!condition) {
        tests_failed++;
        fprintf(stderr, "FAIL: %s\n", name);
    } else {
        printf("PASS: %s\n", name);
    }
}

static int stream_callback(const caedral_stream_chunk_t *chunk, void *user_data) {
    size_t *count = (size_t *)user_data;
    (void)chunk;
    (*count)++;
    return 0;
}

static int retry_chat(caedral_client_t *client, caedral_chat_request_t *request, caedral_response_t **out) {
    int attempt;
    for (attempt = 0; attempt < 3; attempt++) {
        *out = caedral_chat_completions_create(client, request);
        if (*out != NULL) {
            return CAEDRAL_OK;
        }
        const char *err = caedral_get_last_error();
        if (err == NULL || strstr(err, "Upstream error") == NULL) {
            return CAEDRAL_ERR_API;
        }
    }
    return CAEDRAL_ERR_API;
}

int main(void) {
    const char *api_key = getenv("CAEDRAL_TEST_API_KEY");
    const char *base_url = getenv("CAEDRAL_BASE_URL");
    caedral_client_t *client = NULL;
    caedral_response_t *response = NULL;
    caedral_chat_request_t *request = NULL;
    char *content = NULL;
    size_t stream_chunks = 0;

    if (base_url == NULL || base_url[0] == '\0') {
        base_url = "http://localhost:5001";
    }
    if (api_key == NULL || api_key[0] == '\0') {
        fprintf(stderr, "CAEDRAL_TEST_API_KEY is required (run: python tests/create_test_key.py)\n");
        return 1;
    }

    client = caedral_client_new(api_key, base_url);
    expect_true(client != NULL, "client_new");

    response = caedral_models_list(client);
    expect_true(response != NULL, "models_list");
    if (response != NULL) {
        expect_true(strstr(caedral_response_body(response), "caedral-base") != NULL, "models_contains_base");
        caedral_response_free(response);
        response = NULL;
    }

    request = caedral_chat_request_new("caedral-base");
    caedral_chat_request_add_message(request, "user", "Reply with exactly: C SDK OK");
    expect_true(retry_chat(client, request, &response) == CAEDRAL_OK, "chat_completion");
    if (response != NULL) {
        content = caedral_chat_response_get_content(response);
        expect_true(content != NULL && content[0] != '\0', "chat_has_content");
        caedral_free(content);
        caedral_response_free(response);
        response = NULL;
    }

    stream_chunks = 0;
    expect_true(
        caedral_chat_completions_create_stream(client, request, stream_callback, &stream_chunks) == CAEDRAL_OK,
        "chat_stream");
    expect_true(stream_chunks > 0, "chat_stream_chunks");

    response = caedral_usage_get(client);
    expect_true(response != NULL, "usage_get");
    if (response != NULL) {
        expect_true(strstr(caedral_response_body(response), "weeklyPool") != NULL, "usage_has_weekly_pool");
        caedral_response_free(response);
        response = NULL;
    }

    caedral_client_free(client);
    client = caedral_client_new("cd_live_invalid_integration_test_key", base_url);
    request = caedral_chat_request_new("caedral-base");
    caedral_chat_request_add_message(request, "user", "Hello");
    response = caedral_chat_completions_create(client, request);
    expect_true(response == NULL, "invalid_api_key");
    expect_true(strcmp(caedral_get_last_error_type(), "invalid_api_key") == 0, "invalid_api_key_type");

    caedral_chat_request_free(request);
    caedral_response_free(response);
    caedral_client_free(client);

    printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return tests_failed == 0 ? 0 : 1;
}
