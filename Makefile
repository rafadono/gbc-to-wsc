CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude

SRC = src/gbc_memory.c src/gbc_video.c src/gbc_input.c src/gbc_hal.c
TEST_SRC = tests/test_runner.c

all: test_runner

test_runner: $(SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: test_runner
	./test_runner

clean:
	rm -f test_runner test_runner.exe *.o src/*.o

.PHONY: all test clean
