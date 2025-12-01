CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
SRC_DIR = src
BUILD_DIR = build
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

TARGET = $(BUILD_DIR)/gomoku
TARGET_O2 = $(BUILD_DIR)/gomoku-O2


all: $(TARGET)

o2: $(TARGET_O2)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_O2): $(OBJS)
	$(CC) $(CFLAGS) -O2 -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)/*.o $(TARGET)

.PHONY: all clean
