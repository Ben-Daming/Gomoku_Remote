CC := gcc
BASIC_CFLAGS := -Wall -Wextra -Iinclude
DEBUG_FLAGS := -Og -g 
OPT_FLAGS := -O3 -g -march=native
MAD_OPT_FLAGS := -march=native -mtune=native -flto -fwhole-program
SRC_DIR := src
BUILD_DIR := build
CFLAGS := $(BASIC_CFLAGS) $(OPT_FLAGS) -fopenmp
MADFLAGS := $(BASIC_CFLAGS) $(OPT_FLAGS) $(MAD_OPT_FLAGS) -fopenmp
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET := $(BUILD_DIR)/gomoku
MAD_TARGET := $(BUILD_DIR)/gomoku-release
MAD_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/release/%.o, $(SRCS))

all: $(TARGET)
release: $(MAD_TARGET)

$(MAD_TARGET): $(MAD_OBJS)
	$(CC) $(MADFLAGS) -o $@ $^
$(BUILD_DIR)/release/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)/release
	$(CC) $(MADFLAGS) -c -o $@ $<

o2: $(TARGET_O2)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<



clean:
	rm -rf $(BUILD_DIR)/*.o $(TARGET) $(MAD_TARGET) $(MAD_OBJS)

.PHONY: all clean release
