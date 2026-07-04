# Caedral C SDK

Official C client for the [Caedral API](https://caedral.com). Uses **libcurl** for HTTP and **cJSON** (vendored) for JSON parsing.

## Dependencies

- C11 compiler (`gcc`, `clang`, etc.)
- **libcurl** development headers and library
- Python 3 + `psycopg` + `bcrypt` (integration tests only, to create temporary API keys)

On Debian/Ubuntu:

```bash
sudo apt install build-essential libcurl4-openssl-dev
pip install psycopg[binary] bcrypt
```

On Windows, use **MSYS2/UCRT64**:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-curl mingw-w64-ucrt-x86_64-make
export PATH="/ucrt64/bin:$PATH"
make test
```

Or run `bash scripts/build_and_test.sh` on Linux/macOS.

## Build

```bash
cd sdk-c
make
```

This produces:

- `build/libcaedral.a` — static library
- `build/examples/quickstart` — example binary

### CMake (optional)

```bash
cmake -B build-cmake -S .
cmake --build build-cmake
```

## Quickstart

```c
#include "caedral.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    caedral_client_t *client = caedral_client_new("cd_live_...", "http://localhost:5001");
    if (client == NULL) {
        fprintf(stderr, "%s\n", caedral_get_last_error());
        return 1;
    }

    caedral_chat_request_t *req = caedral_chat_request_new("caedral-base");
    caedral_chat_request_add_message(req, "user", "Hello!");

    caedral_response_t *resp = caedral_chat_completions_create(client, req);
    if (resp == NULL) {
        fprintf(stderr, "%s\n", caedral_get_last_error());
        return 1;
    }

    char *content = caedral_chat_response_get_content(resp);
    printf("%s\n", content);
    caedral_free(content);

    caedral_response_free(resp);
    caedral_chat_request_free(req);
    caedral_client_free(client);
    return 0;
}
```

Run the included example:

```bash
export CAEDRAL_TEST_API_KEY=$(python tests/create_test_key.py)
export CAEDRAL_BASE_URL=http://localhost:5001
make examples
./build/examples/quickstart
```

## Memory management

| Object | Allocated by | Freed by |
|--------|--------------|----------|
| `caedral_client_t*` | `caedral_client_new()` | `caedral_client_free()` |
| `caedral_*_request_t*` | `caedral_*_request_new()` | matching `*_free()` |
| `caedral_response_t*` | API methods on success | `caedral_response_free()` |
| Strings from helpers (`caedral_chat_response_get_content`) | SDK | `caedral_free()` |

On failure, API methods return `NULL` and set global last-error state via `caedral_get_last_error()`.

## Error handling

```c
caedral_response_t *resp = caedral_chat_completions_create(client, req);
if (resp == NULL) {
    fprintf(stderr, "status=%d type=%s msg=%s\n",
        caedral_get_last_status_code(),
        caedral_get_last_error_type(),
        caedral_get_last_error());
}
```

Return codes from non-pointer APIs (e.g. streaming):

- `CAEDRAL_OK` (0)
- `CAEDRAL_ERR_INVALID_ARG`
- `CAEDRAL_ERR_MEMORY`
- `CAEDRAL_ERR_NETWORK`
- `CAEDRAL_ERR_API`
- `CAEDRAL_ERR_PARSE`

## Streaming

Callback-based SSE parsing (idiomatic C):

```c
int on_chunk(const caedral_stream_chunk_t *chunk, void *user_data) {
    printf("%s\n", chunk->json);
    return 0; /* return non-zero to abort */
}

caedral_chat_completions_create_stream(client, req, on_chunk, NULL);
```

Each callback receives one raw JSON chunk (`chat.completion.chunk`). The `json` pointer is valid only until the callback returns.

## API reference

### Client

```c
caedral_client_t *caedral_client_new(const char *api_key, const char *base_url);
void caedral_client_free(caedral_client_t *client);
```

### Chat

```c
caedral_response_t *caedral_chat_completions_create(caedral_client_t *client, const caedral_chat_request_t *request);
int caedral_chat_completions_create_stream(caedral_client_t *client, const caedral_chat_request_t *request,
    caedral_stream_callback_fn callback, void *user_data);
```

### Models & usage

```c
caedral_response_t *caedral_models_list(caedral_client_t *client);
caedral_response_t *caedral_usage_get(caedral_client_t *client);
```

### Specialized endpoints

```c
caedral_response_t *caedral_embeddings_create(caedral_client_t *client, const caedral_embedding_request_t *request);
caedral_response_t *caedral_images_generate(caedral_client_t *client, const caedral_image_request_t *request);
caedral_response_t *caedral_audio_generate(caedral_client_t *client, const caedral_audio_request_t *request);
caedral_response_t *caedral_rerank_create(caedral_client_t *client, const caedral_rerank_request_t *request);
```

Successful responses expose the raw JSON body via `caedral_response_body()`.

## Integration tests

Requires gateway on port **5001** and `DATABASE_URL` in repo root `.env`:

```bash
cd sdk-c
make test
```

Or manually:

```bash
export CAEDRAL_TEST_API_KEY=$(python tests/create_test_key.py)
make build/tests/test_integration
./build/tests/test_integration
```

## License

MIT
