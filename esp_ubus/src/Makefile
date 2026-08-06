BUILD_DIRS := ./build
BIN_DIRS := .
SRC_DIRS := .
INC_DIRS := $(CURDIR)

BINNAME := esp_ubus

TARGET_BIN := $(BIN_DIRS)/$(BINNAME)

SRCS := $(shell find $(SRC_DIRS) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIRS)/%.c,$(BUILD_DIRS)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CFLAGS := -g -Wall -Werror
CPPFLAGS := $(INC_FLAGS) -MMD -MP
LDFLAGS := -lblobmsg_json -lubox -lubus -lserialport

.PHONY: all clean

.DELETE_ON_ERROR:
all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILD_DIRS)/%.o: %.c
	@mkdir -p $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIRS)
	rm -f $(BINNAME)

-include $(DEPS)
