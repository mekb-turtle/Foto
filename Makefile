EXEC = foto

BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

SRC_DIR = src
SRCS := $(sort $(shell find $(SRC_DIR) -name '*.c'))

CC = cc
CFLAGS = -Wall
CPPFLAGS = 
LDFLAGS =

LDLIBS = -lbsd -lcairo -lIL -lX11 -lXext -lm

ifeq ($(RELEASE),1)
	BUILD_DIR := $(BUILD_DIR)/release
	RELEASE := 1
	CFLAGS += -O2
else
	BUILD_DIR := $(BUILD_DIR)/debug
	RELEASE := 0
	CFLAGS += -Og
	CFLAGS += -g
endif

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: build

build: buildtext $(BIN_DIR)/$(EXEC) 
	@if [ "$(RELEASE)" = "1" ]; then printf "\e[95m==> \e[0;1mStripping executable %s…\e[0m\n" "$(EXEC)"; strip --strip-unneeded $(BIN_DIR)/$(EXEC); fi

$(BIN_DIR)/$(EXEC): $(OBJS)
	@printf "\e[93m==> \e[0;1mLinking executable %s…\e[0m\n" "$(EXEC)"
	@mkdir -p $(BIN_DIR)
	@$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "\e[92m==> \e[0;1mCompiling %s…\e[0m\n" "$<"
	@mkdir -p $(dir $@)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

buildtext:
	@printf "\e[1;91m> \e[0;1mBuilding %s…\e[0m\n" "$(EXEC)"

run: build
	@printf "\e[1;94m> \e[0;1mRunning %s…\e[0m\n" "$(EXEC)"
	@$(BIN_DIR)/$(EXEC) $(RUNARGS)

clean:
	@printf "\e[1;96m> \e[0;1mCleaning…\e[0m\n"
	@rm -rf $(BUILD_DIR)

install: build
	@if [ "$(RELEASE)" != "1" ]; then printf "\e[1;93m> Installing requires you to be in release mode!\e[0m\n"; exit 1; fi
	@printf "\e[1;93m> \e[0;1mInstalling %s…\e[0m\n" "$(EXEC)"
	@sudo install -sDpg0 -o0 -m755 $(BIN_DIR)/$(EXEC) /usr/local/bin/

uninstall:
	@printf "\e[1;93m> \e[0;1mUninstalling %s…\e[0m\n" "$(EXEC)"
	@sudo rm /usr/local/bin/$(EXEC)
