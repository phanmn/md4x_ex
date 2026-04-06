# Makefile for md4x_ex NIF
# Based on erlang.mk and brotli Makefile patterns

# System type and platform-specific flags (use gnu99 for POSIX extensions like strdup)
UNAME_SYS := $(shell uname -s)
ifeq ($(UNAME_SYS), Darwin)
	CC ?= cc
	CFLAGS += -O3 -std=gnu99 -finline-functions
	LDFLAGS ?= -flat_namespace -undefined suppress
else ifeq ($(UNAME_SYS), FreeBSD)
	CC = /usr/bin/clang
	CFLAGS += -O3 -std=gnu99 -finline-functions
else ifeq ($(UNAME_SYS), Linux)
	CC ?= gcc
	CFLAGS += -O3 -std=gnu99 -finline-functions
endif

# Global flags
CFLAGS += -Wall -Wextra -DHAVE_CONFIG_H -fPIC
LDFLAGS ?= -shared -rdynamic

# Find Erlang NIF include directory (ERTS include dir contains erl_nif.h)
ERTS_INCLUDE_DIR ?= $(shell erl -noshell -eval "io:format(\"~s/erts-~s/include/\", [code:root_dir(), erlang:system_info(version)])." -s init stop 2>/dev/null)
ifneq ($(ERTS_INCLUDE_DIR),)
  CFLAGS += -I$(ERTS_INCLUDE_DIR)
endif

# Add project include paths
CFLAGS += -I./c_src -I./c_src/renderers -I./c_src/libyaml/include -I./c_src/libyaml/include/yaml -I./c_src/libyaml/src

# Source files
C_SOURCES = \
	c_src/md4x.c \
	c_src/entity.c \
	c_src/renderers/md4x-html.c \
	c_src/renderers/md4x-ast.c \
	c_src/renderers/md4x-meta.c \
	c_src/renderers/md4x-heal.c \
	c_src/renderers/md4x-ansi.c \
	c_src/renderers/md4x-text.c \
	c_src/renderers/md4x-markdown.c \
	c_src/md4x_nif.c

# Libyaml sources
YAML_SOURCES = $(wildcard c_src/libyaml/src/*.c)

# All sources
ALL_SOURCES = $(C_SOURCES) $(YAML_SOURCES)

# Build directory for object files
BUILD_DIR = build
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(ALL_SOURCES))

# Output shared object
TARGET = priv/md4x_nif.so

# Verbosity control (V=1 for verbose output)
ifeq ($(V),1)
c_verbose =
link_verbose =
else
c_verbose = @echo " C     " $(notdir $<);
link_verbose = @echo " LD    " $(notdir $@);
endif

# Default target
all: $(TARGET)

# Create priv directory if it doesn't exist
$(shell mkdir -p priv)

# Link the shared object
$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(link_verbose) $(CC) $(LDFLAGS) $(LDLIBS) -o $@ $(OBJECTS)

# Create build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

# Compile C sources to build directory
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(c_verbose) $(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR) $(TARGET)

# Phony targets
.PHONY: all clean
