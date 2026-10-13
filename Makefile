CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

SRC = src/gbc_memory.c src/gbc_video.c src/gbc_input.c src/gbc_hal.c
TEST_SRC = tests/test_runner.c

TARGET = test_runner.exe

all: $(TARGET)

$(TARGET): $(SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TARGET)
	./$(TARGET)

submodule-update:
	git submodule update --init --recursive

game:
	@python tools/pipeline.py $(ROM)

clean:
	rm -f $(TARGET) *.o src/*.o build/gbc_bridge_sample.wsc

.PHONY: all test clean game submodule-update
