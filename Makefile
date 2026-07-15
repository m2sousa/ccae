CC = gcc
CFLAGS = -std=c23 -Iinclude \
		 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes \
		 -g -fsanitize=address,undefined -fno-omit-frame-pointer

SRC := $(shell find src -name '*.c')
OBJ := $(patsubst src/%.c, build/%.o, $(SRC))

TEST_SRC := $(shell find tests -name '*.c')
TEST_OBJ := $(patsubst tests/%.c, build/tests/%.o, $(TEST_SRC))

build/ccae: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Filter-out build/main.o to avoid collision with the runner entry point while linking.
build/ccae-tests: $(filter-out build/main.o, $(OBJ)) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/tests/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Itests -MMD -MP -c $< -o $@

-include $(OBJ:.o=.d)
-include $(TEST_OBJ:.o=.d)

.PHONY: all clean lsp tests
all: build/ccae build/ccae-tests
clean:
	rm -rf ./build/*

lsp:
	bear -- make -B all

tests: build/ccae-tests
	./build/ccae-tests
