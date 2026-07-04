CC ?= gcc
AR ?= ar
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude -Isrc -Ithird_party
LDFLAGS ?=
LIBS ?= -lcurl

LIB_SRCS = src/caedral.c third_party/cJSON.c
LIB_OBJS = $(LIB_SRCS:.c=.o)
STATIC_LIB = build/libcaedral.a

.PHONY: all clean test examples

all: $(STATIC_LIB) examples

build:
	@mkdir -p build

$(STATIC_LIB): build $(LIB_OBJS)
	$(AR) rcs $@ $(LIB_OBJS)

src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

third_party/%.o: third_party/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

examples: build/examples/quickstart

build/examples/quickstart: examples/quickstart.c $(STATIC_LIB)
	@mkdir -p build/examples
	$(CC) $(CFLAGS) $< -Lbuild -lcaedral $(LDFLAGS) $(LIBS) -o $@

build/tests/test_integration: tests/test_integration.c $(STATIC_LIB)
	@mkdir -p build/tests
	$(CC) $(CFLAGS) $< -Lbuild -lcaedral $(LDFLAGS) $(LIBS) -o $@

test: build/tests/test_integration
	@python tests/create_test_key.py > build/tests/.test_key 2>build/tests/.test_key_err || true
	@CAEDRAL_TEST_API_KEY=$$(cat build/tests/.test_key 2>/dev/null); \
	CAEDRAL_BASE_URL=$${CAEDRAL_BASE_URL:-http://localhost:5001}; \
	if [ -z "$$CAEDRAL_TEST_API_KEY" ]; then \
	  CAEDRAL_TEST_API_KEY=$$(python tests/create_test_key.py); \
	fi; \
	CAEDRAL_TEST_API_KEY="$$CAEDRAL_TEST_API_KEY" CAEDRAL_BASE_URL="$$CAEDRAL_BASE_URL" ./build/tests/test_integration

clean:
	rm -rf build src/*.o third_party/*.o
