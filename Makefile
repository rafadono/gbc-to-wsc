CC ?= gcc
AR ?= ar
CFLAGS = -Wall -Wextra -O2 -Icore/include

CORE_SRC = core/src/gbc_memory.c core/src/gbc_video.c core/src/gbc_input.c core/src/gbc_audio.c core/src/gbc_hal.c
CORE_OBJ = $(CORE_SRC:.c=.o)
TEST_SRC = core/tests/test_runner.c
TEST_TARGET = test_runner.exe
CORE_LIB = core/libgbc_wsc.a

all: $(TEST_TARGET) $(CORE_LIB)

$(CORE_LIB): $(CORE_OBJ)
	$(AR) rcs $@ $^

core/src/%.o: core/src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(CORE_SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_TARGET)
	./$(TEST_TARGET)

lib: $(CORE_LIB)

submodule-update:
	git submodule update --init --recursive

game:
	@python tools/pipeline.py $(ROM)

run:
	@python tools/test_wsc.py $(ROM)

validate:
	@python tools/test_wsc.py $(ROM) --validate 4.0

runners:
	@python tools/test_wsc.py --list

clean:
	rm -f $(TEST_TARGET) $(CORE_LIB) core/src/*.o *.o build/gbc_bridge_sample.wsc

.PHONY: all test lib clean game submodule-update run validate runners
