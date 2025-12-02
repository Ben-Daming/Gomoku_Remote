CC := gcc
BASIC_CFLAGS := -Wall -Wextra -Iinclude
DEBUG_FLAGS := -g -pg
OPT_FLAGS := -O3
SRC_DIR := src
BUILD_DIR := build
CFLAGS := $(BASIC_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS)
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET := $(BUILD_DIR)/gomoku

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)/*.o $(TARGET)

.PHONY: all clean
