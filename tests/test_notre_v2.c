/* Notre contract V2 economy fields — unit test against the golden fixture. */
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

int main(void) {
    FILE *f = fopen("tests/fixtures/notre-contract/notre-response-telemetry-v2.json", "rb");
    expect_true(f != NULL, "fixture_open");
    if (f == NULL) return 1;

    char body[4096];
    size_t n = fread(body, 1, sizeof(body) - 1, f);
    body[n] = '\0';
    fclose(f);

    caedral_response_t *response = caedral_response_new(200, body);
    expect_true(response != NULL, "response_new");

    caedral_notre_metadata_t meta;
    memset(&meta, 0xFF, sizeof(meta)); /* poison: parser must fill defaults */
    memset(&meta.mode, 0, sizeof(meta.mode));
    memset(&meta.result, 0, sizeof(meta.result));
    int ok = caedral_chat_response_get_notre(response, &meta);
    expect_true(ok == 1, "get_notre_ok");
    expect_true(meta.present == 1, "present");
    expect_true(meta.enabled == 1, "enabled");
    expect_true(meta.intervened == 1, "intervened");
    expect_true(meta.fallback_used == 0, "fallback_used");
    expect_true(strcmp(meta.mode, "auto") == 0, "mode_auto");
    expect_true(meta.input_before == 1200, "input_before");
    expect_true(meta.input_sent == 310, "input_sent");
    expect_true(meta.input_saved == 890, "input_saved_saved_math");
    expect_true(meta.value_usd > 0.0026 && meta.value_usd < 0.0028, "value_usd");
    expect_true(strcmp(meta.result, "optimized") == 0, "result_optimized");

    caedral_response_free(response);

    printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return tests_failed == 0 ? 0 : 1;
}
