TARGET := water_gateway

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

CPPFLAGS += -I$(INC_DIR)
CFLAGS += -std=gnu99 -Wall -Wextra -O2 -g -MMD -MP
LDFLAGS += -static

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET) -c ../config/gateway.conf

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)
