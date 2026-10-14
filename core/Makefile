CC ?= gcc
AR ?= ar
CFLAGS = -Wall -Wextra -O2 -Iinclude

SRC = src/gbc_memory.c src/gbc_video.c src/gbc_input.c src/gbc_hal.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/test_runner.c

LIB_TARGET = libgbc_wsc.a
TEST_TARGET = test_runner.exe

all: lib test

lib: $(LIB_TARGET)

$(LIB_TARGET): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(OBJ) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJ) $(LIB_TARGET) $(TEST_TARGET)

.PHONY: all lib test clean
