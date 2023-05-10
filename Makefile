EXEC = foto

CC = cc
CFLAGS = -Wall
CPPFLAGS =
LDFLAGS =
LDLIBS = -lbsd -lcairo -lIL -lX11 -lXext -lm

TOP_BUILD_DIR := build
BUILD_DIR := $(TOP_BUILD_DIR)
BIN_BUILD_DIR = $(BUILD_DIR)/bin
OBJ_BUILD_DIR = $(BUILD_DIR)/obj
MAN_BUILD_DIR = $(BUILD_DIR)/man

SRC_DIR = src
SRCS := $(sort $(shell find '$(SRC_DIR)' -name '*.c'))

MAN_DIR = man
MAN_PAGES := $(sort $(shell find '$(MAN_DIR)' -name '*.[[:digit:]].md'))

COMPLETIONS_DIR = completions

ifeq ($(origin INSTALL_DIR),undefined)
	ifeq ($(shell id -u),0)
		INSTALL_DIR := '/usr/local'
	else
		INSTALL_DIR := '$(HOME)/.local'
	endif
endif

ifeq ($(origin BIN_INSTALL_DIR),undefined)
	BIN_INSTALL_DIR := '$(INSTALL_DIR)/bin'
endif

ifeq ($(origin MAN_INSTALL_DIR),undefined)
	MAN_INSTALL_DIR := '$(INSTALL_DIR)/share/man'
endif

ifeq ($(origin BASH_COMPLETIONS_INSTALL_DIR),undefined)
	BASH_COMPLETIONS_INSTALL_DIR := '$(INSTALL_DIR)/share/bash-completion/completions'
endif

ifeq ($(origin ZSH_COMPLETIONS_INSTALL_DIR),undefined)
	ZSH_COMPLETIONS_INSTALL_DIR := '$(INSTALL_DIR)/share/zsh/site-functions'
endif

ifeq ($(origin FISH_COMPLETIONS_INSTALL_DIR),undefined)
	FISH_COMPLETIONS_INSTALL_DIR := '$(INSTALL_DIR)/share/fish/completions'
endif

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

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_BUILD_DIR)/%.o)
MAN_BUILT_PAGES := $(MAN_PAGES:$(MAN_DIR)/%.md=$(MAN_BUILD_DIR)/%)

all: build

build: buildtext $(BIN_BUILD_DIR)/$(EXEC)

$(BIN_BUILD_DIR)/$(EXEC): $(OBJS)
	@printf "\e[93m==> \e[0;1mLinking executable %s…\e[0m\n" '$(EXEC)'
	@mkdir -p '$(dir $@)'
	@$(CC) $(LDFLAGS) $^ $(LDLIBS) -o '$@'
	@if [ "$(RELEASE)" = "1" ]; then printf "\e[95m==> \e[0;1mStripping executable %s…\e[0m\n" '$(EXEC)'; strip --strip-unneeded $(BIN_BUILD_DIR)/$(EXEC); fi
	@printf "\e[1;91m> \e[0;1mBuilt executable to %s…\e[0m\n" '$(BIN_BUILD_DIR)/$(EXEC)'

$(OBJ_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "\e[92m==> \e[0;1mCompiling %s…\e[0m\n" '$<'
	@mkdir -p '$(dir $@)'
	@$(CC) $(CPPFLAGS) $(CFLAGS) -c '$<' -o '$@'

buildtext:
	@printf "\e[1;91m> \e[0;1mBuilding %s…\e[0m\n" '$(EXEC)'

manbuildtext:
	@printf "\e[1;95m> \e[0;1mBuilding man pages…\e[0m\n"

run: build
	@printf "\e[1;94m> \e[0;1mRunning %s…\e[0m\n" '$(EXEC)'
	@$(BIN_BUILD_DIR)/$(EXEC) $(RUNARGS)

man: manbuildtext $(MAN_BUILT_PAGES)

$(MAN_BUILD_DIR)/%: $(MAN_DIR)/%.md
	@printf "\e[96m==> \e[0;1mCompiling man page %s…\e[0m\n" '$<'
	@mkdir -p '$(dir $@)'
	@pandoc -s -f markdown -t man '$<' -o '$@'

clean:
	@printf "\e[1;96m> \e[0;1mCleaning…\e[0m\n"
	@rm -rf '$(TOP_BUILD_DIR)'

install: build man
	@if [ "$(RELEASE)" != "1" ]; then printf "\e[1;93m> Installing requires you to be in release mode!\e[0m\n"; exit 1; fi
	@printf "\e[93m==> \e[0;1mInstalling %s to %s…\e[0m\n" '$(EXEC)' '$(BIN_INSTALL_DIR)'
	@install -Dpm755 -- '$(BIN_BUILD_DIR)/$(EXEC)' '$(BIN_INSTALL_DIR)/$(EXEC)'
	@printf "\e[93m==> \e[0;1mInstalling man pages to %s…\e[0m\n" '$(MAN_INSTALL_DIR)'
	$(foreach page,$(MAN_BUILT_PAGES),@install -Dpm644 -- '$(page)' '$(subst $(MAN_BUILD_DIR),$(MAN_INSTALL_DIR),$(page))')
	@printf "\e[93m==> \e[0;1mInstalling Completions…\e[0m\n"
	@install -Dpm755 -- '$(COMPLETIONS_DIR)/foto.bash' '$(BASH_COMPLETIONS_INSTALL_DIR)/foto'
	@install -Dpm644 -- '$(COMPLETIONS_DIR)/foto.zsh' '$(ZSH_COMPLETIONS_INSTALL_DIR)/_foto'
	@install -Dpm755 -- '$(COMPLETIONS_DIR)/foto.fish' '$(FISH_COMPLETIONS_INSTALL_DIR)/foto.fish'

uninstall:
	@if [ "$(RELEASE)" != "1" ]; then printf "\e[1;93m> Uninstalling requires you to be in release mode!\e[0m\n"; exit 1; fi
	@printf "\e[1;93m> \e[0;1mUninstalling %s…\e[0m\n" '$(EXEC)'
	@printf "\e[93m==> \e[0;1mUninstalling %s from %s…\e[0m\n" '$(EXEC)' '$(BIN_INSTALL_DIR)'
	@rm -f -- '$(BIN_INSTALL_DIR)/$(EXEC)'
	@printf "\e[93m==> \e[0;1mUninstalling man pages from %s…\e[0m\n" '$(MAN_INSTALL_DIR)'
	$(foreach page,$(MAN_BUILT_PAGES),@rm -f -- '$(subst $(MAN_BUILD_DIR),$(MAN_INSTALL_DIR),$(page))')
	@printf "\e[93m==> \e[0;1mUninstalling Completions…\e[0m\n"
	@rm -f '$(BASH_COMPLETIONS_INSTALL_DIR)/foto'
	@rm -f '$(ZSH_COMPLETIONS_INSTALL_DIR)/_foto'
	@rm -f '$(FISH_COMPLETIONS_INSTALL_DIR)/foto.fish'
