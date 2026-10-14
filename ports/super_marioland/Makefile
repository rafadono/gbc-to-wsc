CC ?= gcc
CFLAGS = -Wall -Wextra -O2 -I../../core/include -Ihooks

CORE_SRC = ../../core/src/gbc_memory.c ../../core/src/gbc_video.c ../../core/src/gbc_input.c ../../core/src/gbc_hal.c
HOOKS_SRC = hooks/sml_hooks.c
TEST_SRC = test_sml_hooks.c
TEST_TARGET = test_sml_hooks.exe

all: test

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(CORE_SRC) $(HOOKS_SRC) $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^

patch:
	@python apply_widescreen_patch.py

revert:
	@python apply_widescreen_patch.py --revert

clean:
	rm -f $(TEST_TARGET) *.o hooks/*.o

.PHONY: all test patch revert clean
