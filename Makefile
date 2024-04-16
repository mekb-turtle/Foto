include config.mk

CC ?= cc

ORIG_BUILD_DIR = build
BUILD_DIR := $(ORIG_BUILD_DIR)

BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
OBJ_BINARY_DIR = $(BUILD_DIR)/objbin

SRC_DIR = src
SRC_BINARY_DIR = binary

CPPFLAGS += -DTARGET='"$(TARGET)"'

ifdef VERSION
	CPPFLAGS += -DVERSION='"$(VERSION)"'
endif

ifeq ($(RELEASE),1)
	BUILD_DIR := $(BUILD_DIR)/release
	RELEASE := 1
	CFLAGS += -O2
	CPPFLAGS += -DRELEASE
else
	BUILD_DIR := $(BUILD_DIR)/debug
	RELEASE := 0
	CFLAGS += -Og
	CFLAGS += -g
	CPPFLAGS += -DDEBUG
endif

SRC_FILES := $(wildcard $(SRC_DIR)/*.c) $(EXTRA_SRC_FILES)
BINARY_FILES := $(wildcard $(SRC_BINARY_DIR)/*) $(EXTRA_BINARY_FILES)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))
OBJ_BINARY_FILES := $(patsubst $(SRC_BINARY_DIR)/%, $(OBJ_BINARY_DIR)/%.o, $(BINARY_FILES))

MAN_SRC_DIR=man
MAN_OUT_DIR=$(ORIG_BUILD_DIR)/man

MAN_SRC_FILES :=
MAN_OUT_FILES :=

ifeq ($(shell test -d '$(MAN_SRC_DIR)' && echo 1),1)
	MAN=1
	MAN_SRC_FILES := $(wildcard $(MAN_SRC_DIR)/*.[0-9].md)
	MAN_OUT_FILES := $(patsubst $(MAN_SRC_DIR)/%.md, $(MAN_OUT_DIR)/%, $(MAN_SRC_FILES))
else
	MAN=0
endif

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(OBJ_BINARY_FILES) $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(LDLIBS) $^ -o $@

$(OBJ_BINARY_DIR)/%.o: $(SRC_BINARY_DIR)/% | $(OBJ_BINARY_DIR)
	xxd -i $< | $(CC) $(CFLAGS) $(CPPFLAGS) -x c -c - -o $@
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p -- $(BIN_DIR)
$(OBJ_DIR):
	mkdir -p -- $(OBJ_DIR)
$(OBJ_BINARY_DIR):
	mkdir -p -- $(OBJ_BINARY_DIR)
$(MAN_OUT_DIR):
	mkdir -p -- $(MAN_OUT_DIR)

clean:
	rm -f -- $(BIN_DIR)/$(TARGET) $(OBJ_FILES) $(MAN_OUT_FILES) $(OBJ_BINARY_FILES) || true
	rmdir -- $(OBJ_BINARY_DIR) $(BIN_DIR) $(OBJ_DIR) $(BUILD_DIR) $(ORIG_BUILD_DIR) $(MAN_OUT_DIR) || true

man: $(MAN_OUT_FILES)

$(MAN_OUT_DIR)/%: $(MAN_SRC_DIR)/%.md | $(MAN_OUT_DIR)
	sed 's/INSERT_VERSION_HERE/$(VERSION)/g' < '$<' | pandoc -s -f markdown -t man - -o '$@'

.PHONY: all clean man
