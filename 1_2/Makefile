TARGETS := $(basename $(notdir $(wildcard *.c)))
BUILD_DIR := build

CFLAGS += -O0 -g -Wall -Wextra -pedantic -Wno-format -Wno-unused-parameter
LIBS   := -pthread
INCLUDE_DIR := .

all: $(TARGETS:%=$(BUILD_DIR)/%)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< $(LIBS) -o $@

clean:
	rm -rf $(BUILD_DIR)
