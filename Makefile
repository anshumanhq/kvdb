CC        := gcc
CFLAGS    := -Wall -Wextra -Werror -std=c11 -MMD -MP -D_POSIX_C_SOURCE=200809L
SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := build

VERSION    := $(shell git describe --tags --always --dirty 2>/dev/null || echo "dev")
BUILD_TIME := $(shell date -u +"%Y-%m-%dT%H:%M:%SZ")
VERSION_DEFS := -DVERSION=\"$(VERSION)\" -DBUILD_TIME=\"$(BUILD_TIME)\"

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

DEBUG_CFLAGS   := -g -fsanitize=address,undefined $(VERSION_DEFS)
RELEASE_CFLAGS := -O2 -DNDEBUG $(VERSION_DEFS)

debug: CFLAGS += $(DEBUG_CFLAGS)
debug: $(BUILD_DIR)/kvdb

release: CFLAGS += $(RELEASE_CFLAGS)
release: $(BUILD_DIR)/kvdb

$(BUILD_DIR)/kvdb: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $@

# Tests ASan+UBSan ke saath (bugs turant pakdo)
test: CFLAGS += $(DEBUG_CFLAGS) -I$(SRC_DIR)
test: $(BUILD_DIR)/test_hashtable
	./$(BUILD_DIR)/test_hashtable

$(BUILD_DIR)/test_hashtable: $(TEST_DIR)/test_hashtable.c $(SRC_DIR)/hashtable.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Release-mode test for Valgrind (no sanitizers)
test-release: CFLAGS += $(RELEASE_CFLAGS) -I$(SRC_DIR)
test-release: $(BUILD_DIR)/test_hashtable_release
	./$(BUILD_DIR)/test_hashtable_release

$(BUILD_DIR)/test_hashtable_release: $(TEST_DIR)/test_hashtable.c $(SRC_DIR)/hashtable.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Release optimization ke saath benchmark build aur run karega
bench: CFLAGS += $(RELEASE_CFLAGS) -I$(SRC_DIR)
bench: $(BUILD_DIR)/ht_bench
	./$(BUILD_DIR)/ht_bench

$(BUILD_DIR)/ht_bench: bench/ht_bench.c $(SRC_DIR)/hashtable.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)

.PHONY: debug release test test-release clean
