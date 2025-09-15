CC      := gcc
CFLAGS  := -Wall -Wextra -O2
TARGET  := unhex
SRC     := unhex.c
TESTS   := $(wildcard test/*.in)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

test: all
	@for test in $(TESTS); do \
		echo "Running $$test..."; \
		bash test/test.sh $$test || { echo "Test $$test failed."; exit 1; }; \
	done
	@echo "All tests passed."

clean:
	rm -f $(TARGET)