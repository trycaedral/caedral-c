#include "caedral.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const char *api_key = getenv("CAEDRAL_API_KEY");
    const char *base_url = getenv("CAEDRAL_BASE_URL");
    caedral_client_t *client;
    caedral_chat_request_t *request;
    caedral_response_t *models;
    caedral_response_t *completion;
    char *content;

    if (api_key == NULL || api_key[0] == '\0') {
        api_key = getenv("CAEDRAL_TEST_API_KEY");
    }
    if (api_key == NULL || api_key[0] == '\0') {
        fprintf(stderr, "Set CAEDRAL_API_KEY or CAEDRAL_TEST_API_KEY\n");
        return 1;
    }
    if (base_url == NULL || base_url[0] == '\0') {
        base_url = "http://localhost:5001";
    }

    client = caedral_client_new(api_key, base_url);
    if (client == NULL) {
        fprintf(stderr, "client: %s\n", caedral_get_last_error());
        return 1;
    }

    models = caedral_models_list(client);
    if (models == NULL) {
        fprintf(stderr, "models: %s\n", caedral_get_last_error());
        caedral_client_free(client);
        return 1;
    }
    printf("Models response (%d bytes)\n", (int)strlen(caedral_response_body(models)));
    caedral_response_free(models);

    request = caedral_chat_request_new("caedral-base");
    caedral_chat_request_add_message(request, "user", "Say hello in one short sentence.");
    completion = caedral_chat_completions_create(client, request);
    if (completion == NULL) {
        fprintf(stderr, "chat: %s\n", caedral_get_last_error());
        caedral_chat_request_free(request);
        caedral_client_free(client);
        return 1;
    }

    content = caedral_chat_response_get_content(completion);
    printf("Assistant: %s\n", content != NULL ? content : "(empty)");
    caedral_free(content);
    caedral_response_free(completion);
    caedral_chat_request_free(request);
    caedral_client_free(client);
    return 0;
}
