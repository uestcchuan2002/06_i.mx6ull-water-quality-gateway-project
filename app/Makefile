TARGET := water_gateway

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
SQLITE_DIR := third_party/sqlite3

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

SQLITE_OBJ := $(BUILD_DIR)/sqlite3.o

CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

CPPFLAGS += -I$(INC_DIR) -I$(SQLITE_DIR)
CFLAGS += -std=gnu99 -Wall -Wextra -O2 -g -MMD -MP
LDFLAGS += -static

.PHONY: all clean run get_sqlite3

all: get_sqlite3 $(TARGET)

$(TARGET): $(OBJS) $(SQLITE_OBJ)
	$(CC) $(OBJS) $(SQLITE_OBJ) -o $@ $(LDFLAGS) -lpthread -ldl -lm

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/sqlite3.o: $(SQLITE_DIR)/sqlite3.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION -Wno-implicit-fallthrough -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

get_sqlite3:
	@sh get_sqlite3.sh

run: $(TARGET)
	./$(TARGET) -c ../config/gateway.conf

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)
