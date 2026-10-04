/* Notre contract V3 shape fields — unit test against the golden fixture. */
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
    FILE *f = fopen("tests/fixtures/notre-contract/notre-response-telemetry-v3.json", "rb");
    expect_true(f != NULL, "fixture_open");
    if (f == NULL) return 1;

    char body[4096];
    size_t n = fread(body, 1, sizeof(body) - 1, f);
    body[n] = '\0';
    fclose(f);

    caedral_response_t *response = caedral_response_new(200, body);
    expect_true(response != NULL, "response_new");

    caedral_notre_metadata_t meta;
    memset(&meta, 0, sizeof(meta));
    int ok = caedral_chat_response_get_notre(response, &meta);
    expect_true(ok == 1, "get_notre_ok");
    expect_true(meta.present == 1, "present");
    expect_true(meta.input_before == 900, "input_before");
    expect_true(meta.input_sent == 240, "input_sent");
    expect_true(meta.input_saved == 660, "input_saved_math");
    expect_true(strcmp(meta.shape, "chat") == 0, "shape_chat");
    expect_true(meta.contract_version == 3, "contract_version_3");
    expect_true(meta.saved_breakdown.present == 1, "saved_breakdown_present");
    expect_true(meta.saved_breakdown.cache_hit_tokens == 640, "cache_hit_tokens");
    expect_true(meta.saved_breakdown.dedup_tokens == 20, "dedup_tokens");
    expect_true(meta.saved_breakdown.prefilter_tokens == 0, "prefilter_tokens");

    caedral_response_free(response);

    printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return tests_failed == 0 ? 0 : 1;
}
