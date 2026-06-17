# Makefile for toy project
# C99, POSIX-compliant, fio-stl only

#############################################################################
# Settings
#############################################################################

DESCRIPTION="The facil.io C STL Library"
AUTHOR="Boaz Segev"

# Project configuration
NAME=fio-stl
CC ?= gcc
SRC_DIR = src
BUILD_DIR ?= tmp
TEST_DIR := tests
EXAMPLES_DIR := examples
INSTALL_PREFIX ?= /usr/local


# Compiler / Linker Warnings... i.e. -Wpedantic -Weverything -Wno-format-pedantic -Wshorten-64-to-32
WARNINGS=-Wshadow -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -Wformat-security -Wno-psabi
# Compiler and linker flags
# Consider: -O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize
OPTIMIZATION=-O3 -DNDEBUG -DNODEBUG
# CFLAGS in production mode.
CFLAGS+=$(OPTIMIZATION) $(WARNINGS) -I$(SRC_DIR) -I.
LDFLAGS+= -lm

# Main executable
PROJECT = $(BUILD_DIR)/$(NAME)

# combining headers, C files and docs to a single file? i.e., fio-stl
LIB_CONCAT_FOLDER=fio-stl
LIB_CONCAT_TARGET=fio-stl

# Source files
SOURCES = $(shell find $(SRC_DIR) -name "*.c" -type f 2>/dev/null | sed 's/ /\\ /g')
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Install paths
INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include/$(NAME)

# Library output paths
UNAME_S    := $(shell uname -s)
ifeq ($(OS),Windows_NT)
  SHARED_EXT   := dll
  SHARED_FLAGS := -shared -Wl,--out-implib,$(LIB_DIR)/libfio.dll.a
else ifeq ($(UNAME_S),Darwin)
  SHARED_EXT   := dylib
  SHARED_FLAGS := -dynamiclib
else
  SHARED_EXT   := so
  SHARED_FLAGS := -shared -fPIC
endif
LIB_DIR    := $(BUILD_DIR)/lib
LIB_STATIC := $(LIB_DIR)/libfio.a
LIB_SHARED := $(LIB_DIR)/libfio.$(SHARED_EXT)

#############################################################################
# Basics
#############################################################################

# Targets
.PHONY: all clean test format lint install install-headers everything___ help set_debug_flags FORCE lib

# Default target
all: everything___
	@echo "Library headers installed to $(BUILD_DIR)/include/"

# Build main executable (only if source files exist)
ifneq ($(strip $($(strip $(SOURCES)))),)
$(PROJECT): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
endif

# Build object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "* Compiling $*.c"
	@$(CC) $(CFLAGS) -c -o $@ $<

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Format code with clang-format (style set in .clang-format)
format:
	@echo "Formatting code..."
	@clang-format -i $(SRC_DIR)/**/*.c $(SRC_DIR)/**/*.h $(SRC_DIR)/*.c $(SRC_DIR)/*.h 2>/dev/null || true
	@echo "Formatting complete!"

# Lint code with clang-tidy
lint:
	@echo "Linting code..."
	@clang-tidy $(SRC_DIR)/main.c --extra-arg=-std=c99 -- $(CFLAGS)
	@echo "Linting complete!"

#############################################################################
# Debug Flags (db/target or target/db)
#############################################################################

set_debug_flags:
	$(eval CFLAGS=$(DEBUG_CFLAGS))
	@echo "(!) Debug mode detected."

# Force-rebuild helper: any target that depends on this is always out of date
FORCE:

db/%: | clean set_debug_flags % ;
%/db: | clean set_debug_flags % ;

#############################################################################
# Install
#############################################################################

# Install binary and headers to system
install: $(PROJECT) install-headers install-docs
	@echo "Installing $(NAME) to $(INSTALL_PREFIX)..."
	@mkdir -p $(INSTALL_BIN)
	@mkdir -p $(INSTALL_INCLUDE)
	@install -m 755 $(PROJECT) $(INSTALL_BIN)/
	@cp -r $(BUILD_DIR)/include/* $(INSTALL_INCLUDE)/
	@echo "Installation complete!"
	@echo "  Binary: $(INSTALL_BIN)/$(NAME)"
	@echo "  Headers: $(INSTALL_INCLUDE)/"


# Copy headers to build/include preserving directory structure
install-headers: | $(BUILD_DIR)
	@echo "Copying headers to $(BUILD_DIR)/include..."
	@find $(SRC_DIR) -name "*.h" -type f | while read -r header; do \
		rel_path=$${header#$(SRC_DIR)/}; \
		target_dir=$(BUILD_DIR)/include/$$(dirname "$$rel_path"); \
		mkdir -p "$$target_dir"; \
		cp "$$header" "$$target_dir/"; \
	done
	@echo "Headers copied successfully!"

# Copy documentation to build/docs preserving directory structure
install-docs: | $(BUILD_DIR)
	@echo "Copying documentation to $(BUILD_DIR)/docs..."
	@find $(SRC_DIR) -name "*.md" -type f | while read -r file_name; do \
		rel_path=$${file_name#$(SRC_DIR)/}; \
		target_dir=$(BUILD_DIR)/docs/$$(dirname "$$rel_path"); \
		mkdir -p "$$target_dir"; \
		cp "$$file_name" "$$target_dir/"; \
	done
	@echo "Headers copied successfully!"

everything___: install-docs install-headers

############################################################################-
# Library Build (static + shared from lib/fio.c)
############################################################################-

lib: $(LIB_STATIC) $(LIB_SHARED)
	@echo "Libraries ready in $(LIB_DIR)/"
	@echo "  Static:  $(LIB_STATIC)"
	@echo "  Shared:  $(LIB_SHARED)"

$(LIB_DIR):
	@mkdir -p $@

# Single PIC object — valid for both static archive and shared library
$(LIB_DIR)/fio.pic.o: lib/fio.c lib/fio.h FORCE | $(LIB_DIR)
	@echo "* Compiling lib/fio.c"
	@$(CC) $(CFLAGS) -fPIC -c -o $@ lib/fio.c

$(LIB_STATIC): $(LIB_DIR)/fio.pic.o | $(LIB_DIR)
	@echo "* Archiving $(LIB_STATIC)"
	@ar rcs $@ $<

$(LIB_SHARED): $(LIB_DIR)/fio.pic.o | $(LIB_DIR)
	@echo "* Linking $(LIB_SHARED)"
	@$(CC) $(SHARED_FLAGS) -o $@ $< $(LDFLAGS)

############################################################################-
# Generic folder rules: build & run every standalone C program
############################################################################-

AFTER_RUN_MESSAGE="\\n$(DESCRIPTION) is brought to you by \\x1B[1m$(AUTHOR)\x1B[0m.\\n\\x1B[1mValue deserves to be valued.\x1B[0m\\n(please consider code contributions / donations)\\n"


# $(call DEFINE_UNIT_RULE, path) - explicit rule to run one compiled unit
define DEFINE_UNIT_RULE
$(1): $(BUILD_DIR)/$(1)
	@echo "Running $$@"
	@echo "=================================="
	@$(BUILD_DIR)/$$@
endef

# $(call DEFINE_FOLDER, folder) generates:
#   <folder>              - build and run all .c units in <folder>
#   <folder>/<path/name>  - build and run a specific unit
#   tmp/<folder>/<path/name> - the actual binary target
define DEFINE_FOLDER
$(1)_SOURCES := $(shell find $(1) -name "*.c" -type f 2>/dev/null | sort | sed 's/ /\\ /g')
$(1)_BINS    := $$($(1)_SOURCES:%.c=$(BUILD_DIR)/%)
$(1)_RUNS    := $$($(1)_SOURCES:$(1)/%.c=$(1)/%)

.PHONY: $(1) $$($(1)_RUNS)

# Build: each .c becomes its own executable under $(BUILD_DIR).
# FORCE is a normal prerequisite so the binary is always recompiled.
$$(BUILD_DIR)/$(1)/%: $(1)/%.c $(OBJECTS) FORCE | $(BUILD_DIR)
	@mkdir -p $$(dir $$@)
	@echo "* Compiling $(1)/$$*.c"
	@$$(CC) $$(CFLAGS) -o $$@ $$< $$(filter-out $$(BUILD_DIR)/main.o,$$(OBJECTS)) $$(LDFLAGS)

# Run one unit (explicit rules so prerequisites are built even for phony targets)
$$(foreach run,$$($(1)_RUNS),$$(eval $$(call DEFINE_UNIT_RULE,$$(run))))

# Run every unit in the folder, with pass/fail counter
$(1): $$($(1)_BINS)
	@echo ""
	@echo "Running all units in $(1)/ ..."
	@unit_count=0; pass_count=0; fail_count=0; failed_units=""; \
	for unit_bin in $$($(1)_BINS); do \
		unit_count=$$$$((unit_count + 1)); \
		unit_name=$$$$(basename $$$$unit_bin); \
		echo "  [$$$$unit_count] Running $$$$unit_name ..."; \
		if $$$$unit_bin; then \
			echo "      ✓ UNIT PASSED: $$$$unit_bin"; \
			pass_count=$$$$((pass_count + 1)); \
		else \
			echo "      ✗ UNIT FAILED: $$$$unit_bin"; \
			fail_count=$$$$((fail_count + 1)); \
			failed_units="$$$$failed_units$$$$unit_name "; \
		fi; \
		echo ""; \
	done; \
	echo "Unit Results: $$$$pass_count passed, $$$$fail_count failed, $$$$unit_count total"; \
	if [ $$$$fail_count -gt 0 ]; then \
		echo ""; \
		echo "Failed units:"; \
		for failed_unit in $$$$failed_units; do \
			echo "  ✗ $$$$failed_unit"; \
		done; \
		exit 1; \
	fi
	@echo "All units in $(1)/ complete!"
	@echo $(AFTER_RUN_MESSAGE)

endef

$(foreach folder,$(TEST_DIR) $(EXAMPLES_DIR) extras benchmarks stress tests-old,$(eval $(call DEFINE_FOLDER,$(folder))))

# `make test` is a convenience alias for `make tests`
test: tests;
test/%: tests/% ;
benchmark: benchmarks;
bench: benchmarks;
benchmark/%: benchmarks/% ;
bench/%: benchmarks/% ;
test-all: tests benchmarks stress;
tests-all: test-all;
all-tests: test-all;

#############################################################################
# Combining single-file library
#############################################################################

ifdef LIB_CONCAT_FOLDER
ifdef LIB_CONCAT_TARGET
ifneq ($(OS),Windows_NT)
# POSIX implementation

ifneq ($(wildcard $(LIB_CONCAT_FOLDER)/*.h), $(EMPTY))
  $(info * Merging to single-file header: $(LIB_CONCAT_TARGET).h)
  $(shell rm $(LIB_CONCAT_TARGET).h 2> /dev/null)
  $(shell cat $(LIB_CONCAT_FOLDER)/*.h >> $(LIB_CONCAT_TARGET).h)
endif
ifneq ($(wildcard $(LIB_CONCAT_FOLDER)/*.c), $(EMPTY))
  $(info * Merging to single-file C source: $(LIB_CONCAT_TARGET).c)
  $(shell rm $(LIB_CONCAT_TARGET).c 2> /dev/null)
  $(shell cat $(LIB_CONCAT_FOLDER)/*.c >> $(LIB_CONCAT_TARGET).c)
endif
ifneq ($(wildcard $(LIB_CONCAT_FOLDER)/*.md), $(EMPTY))
  $(info * Merging documentation: $(LIB_CONCAT_TARGET).md)
  $(shell rm $(LIB_CONCAT_TARGET).md 2> /dev/null)
  $(shell cat $(LIB_CONCAT_FOLDER)/*.md >> $(LIB_CONCAT_TARGET).md)
endif

else
# Windows implementation
$(warning *** Single-file library concatination skipped: requires a POSIX system.)
LDFLAGS += -lcrypt32
endif #Windows_NT

endif # LIB_CONCAT_TARGET
endif # LIB_CONCAT_FOLDER

#############################################################################
# Compile Time Tests / Flags
#############################################################################

#############################################################################
# Makefile Runtime Tests (sets flags, such as HAVE_OPENSSL)
#############################################################################
# Tests are performed unless the value is empty / missing
TEST4SOCKET:=     # --- tests for socket library linker flags
TEST4POLL:=       # HAVE_KQUEUE / HAVE_EPOLL / HAVE_POLL
TEST4CRYPTO:=1    # HAVE_OPENSSL / HAVE_SODIUM
TEST4ZLIB:=1      # HAVE_ZLIB
TEST4PG:=         # HAVE_POSTGRESQL
TEST4SQLITE3:=    # HAVE_SQLITE3

#############################################################################
# TRY_RUN, TRY_COMPILE and TRY_COMPILE_AND_RUN functions
#
# Call using $(call TRY_COMPILE, code, compiler_flags)
#
# Returns shell code as string: "0" (success) or non-0 (failure)
#
# TRY_COMPILE_AND_RUN returns the program's shell code as string.
#############################################################################
TRY_RUN=$(shell $(1) >> /dev/null 2> /dev/null; echo $$?;)
TRY_COMPILE=$(shell printf $(1) | $(CC) $(CFLAGS) -xc -o /dev/null - $(LDFLAGS) $(2) >> /dev/null 2> /dev/null ; echo $$? 2> /dev/null)
TRY_COMPILE_AND_RUN=$(shell printf $(1) | $(CC) $(CFLAGS) -xc -o ./___$(NAME)_tmp_test_ - $(LDFLAGS) $(2) 2> /dev/null ; ./___fio_tmp_test_ >> /dev/null 2> /dev/null; echo $$?; rm ./___$(NAME)_tmp_test_ 2> /dev/null)
TRY_HEADER_AND_FUNC= $(shell printf "\#include <$(strip $(1))>\\nint main(void) {(void)($(strip $(2)));}" | $(CC) $(CFLAGS) -xc -o /dev/null - $(LDFLAGS) $(3) >> /dev/null 2> /dev/null; echo $$? 2> /dev/null)
EMPTY_STRING:=
# pkg-config
PKG_CONFIG?=pkg-config

#############################################################################
# Detecting SystemV socket libraries
# (no need to edit)
#############################################################################
ifneq ($(strip $(TEST4SOCKET)),)

FIO_TEST_SOCKET_AND_NETWORK_SERVICE:="\\n\
\#include <sys/types.h>\\n\
\#include <sys/socket.h>\\n\
\#include <netinet/in.h>\\n\
\#include <arpa/inet.h>\\n\
int main(void) {\\n\
  struct sockaddr_in addr = { .sin_port = 0 };\\n\
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);\\n\
  if(fd == -1) return 1;\\n\
  if(inet_pton(AF_INET, \"127.0.0.1\", &addr.sin_addr) < 1) return 1;\\n\
  return connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0 ? 1 : 0;\\n\
}\\n\
"

ifeq ($(call TRY_COMPILE, $(FIO_TEST_SOCKET_AND_NETWORK_SERVICE), $(EMPTY)), 0)
  $(info * Detected native socket API, without additional libraries)
else ifeq ($(call TRY_COMPILE, $(FIO_TEST_SOCKET_AND_NETWORK_SERVICE), "-lsocket" "-lnsl"), 0)
  $(info * Detected socket API from libsocket and libnsl)
  LINKER_LIBS_EXT:=$(LINKER_LIBS_EXT) socket nsl
else
  $(warning No socket API detected - won't be able to compile facil.io)
endif

endif # TEST4SOCKET

ifneq ($(strip $(TEST4POLL)),)

FIO_POLL_TEST_KQUEUE:="\\n\
\#define _GNU_SOURCE\\n\
\#include <stdlib.h>\\n\
\#include <sys/event.h>\\n\
int main(void) {\\n\
  int fd = kqueue();\\n\
}\\n\
"

FIO_POLL_TEST_EPOLL:="\\n\
\#define _GNU_SOURCE\\n\
\#include <stdlib.h>\\n\
\#include <stdio.h>\\n\
\#include <sys/types.h>\\n\
\#include <sys/stat.h>\\n\
\#include <fcntl.h>\\n\
\#include <sys/epoll.h>\\n\
int main(void) {\\n\
  int fd = epoll_create1(EPOLL_CLOEXEC);\\n\
}\\n\
"

FIO_POLL_TEST_POLL:="\\n\
\#define _GNU_SOURCE\\n\
\#include <stdlib.h>\\n\
\#include <poll.h>\\n\
int main(void) {\\n\
  struct pollfd plist[18];\\n\
  memset(plist, 0, sizeof(plist[0]) * 18);\\n\
  poll(plist, 1, 1);\\n\
}\\n\
"

# Test for manual selection and then TRY_COMPILE with each polling engine
ifdef FIO_POLL
  $(info * Skipping polling tests, enforcing manual selection of: poll)
  FLAGS+=HAVE_POLL
else ifdef FIO_FORCE_POLL
  $(info * Skipping polling tests, enforcing manual selection of: poll)
  FLAGS+=HAVE_POLL
else ifdef FIO_FORCE_EPOLL
  $(info * Skipping polling tests, enforcing manual selection of: epoll)
  FLAGS+=HAVE_EPOLL
else ifdef FIO_FORCE_KQUEUE
  $(info * Skipping polling tests, enforcing manual selection of: kqueue)
  FLAGS+=HAVE_KQUEUE
else ifeq ($(call TRY_COMPILE, $(FIO_POLL_TEST_EPOLL), $(EMPTY)), 0)
  $(info * Detected `epoll`)
  FLAGS+=HAVE_EPOLL
else ifeq ($(call TRY_COMPILE, $(FIO_POLL_TEST_KQUEUE), $(EMPTY)), 0)
  $(info * Detected `kqueue`)
  FLAGS+=HAVE_KQUEUE
else ifeq ($(call TRY_COMPILE, $(FIO_POLL_TEST_POLL), $(EMPTY)), 0)
  $(info * Detected `poll` - this is suboptimal fallback!)
  FLAGS+=HAVE_POLL
else
$(warning No supported polling engine! won't be able to compile facil.io)
endif

endif # TEST4POLL
#############################################################################
# SSL/ TLS Library Detection
# (no need to edit)
#############################################################################
ifneq ($(strip $(TEST4CRYPTO)),)

# OpenSSL requirement C application code
FIO_TLS_TEST_OPENSSL:="\\n\
\#define _GNU_SOURCE\\n\
\#include <stdlib.h>\\n\
\#include <stdio.h>\\n\
\#include <openssl/bio.h> \\n\
\#include <openssl/err.h> \\n\
\#include <openssl/ssl.h> \\n\
\#if OPENSSL_VERSION_MAJOR < 3L \\n\
\#error \"OpenSSL version too small\" \\n\
\#endif \\n\
int main(void) { \\n\
  SSL_library_init(); \\n\
  SSL_CTX *ctx = SSL_CTX_new(TLS_method()); \\n\
  SSL *ssl = SSL_new(ctx); \\n\
  BIO *bio = BIO_new_socket(3, 0); \\n\
  BIO_up_ref(bio); \\n\
  SSL_set0_rbio(ssl, bio); \\n\
  SSL_set0_wbio(ssl, bio); \\n\
}\\n\
"

OPENSSL_CFLAGS:=
OPENSSL_LDFLAGS:="-lssl" "-lcrypto"
LIBSODIUM_CFLAGS:=
LIBSODIUM_LDFLAGS:=-lsodium
# detect OpenSSL flags using pkg-config, if available
ifeq ($(shell $(PKG_CONFIG) -- openssl >/dev/null 2>&1; echo $$?), 0)
  OPENSSL_CFLAGS:=$(shell $(PKG_CONFIG) --cflags openssl)
  OPENSSL_LDFLAGS:=$(shell $(PKG_CONFIG) --libs openssl)
endif
ifeq ($(shell $(PKG_CONFIG) -- libsodium >/dev/null 2>&1; echo $$?), 0)
  LIBSODIUM_CFLAGS:=$(shell $(PKG_CONFIG) --cflags libsodium)
  LIBSODIUM_LDFLAGS:=$(shell $(PKG_CONFIG) --libs libsodium)
endif

# add TLS library flags (TODO? non-exclusive?)
ifdef FIO_NO_TLS
  $(info * Skipping crypto library detection.)
else ifeq ($(call TRY_COMPILE, $(FIO_TLS_TEST_OPENSSL), $(OPENSSL_CFLAGS) $(OPENSSL_LDFLAGS)), 0)
  $(info * Detected the OpenSSL library, setting HAVE_OPENSSL)
  CFLAGS+=-DHAVE_OPENSSL $(OPENSSL_CFLAGS)
  LDFLAGS+=$(OPENSSL_LDFLAGS)
else ifeq ($(call TRY_COMPILE, "\#include <sodium.h.h>\\n int main(void) {}", $(LIBSODIUM_CFLAGS) $(LIBSODIUM_LDFLAGS)) , 0)
  # Sodium Crypto Library: https://doc.libsodium.org/usage
  $(info * Detected the Sodium library, setting HAVE_SODIUM)
  CFLAGS+=-DHAVE_SODIUM $(LIBSODIUM_CFLAGS)
  LDFLAGS+=$(LIBSODIUM_LDFLAGS)
else
  $(info * No compatible SSL/TLS library detected.)
endif # FIO_NO_TLS

endif # TEST4CRYPTO
#############################################################################
# ZLib Library Detection
# (no need to edit)
#############################################################################
ifneq ($(strip $(TEST4ZLIB)),)

ifeq ($(call TRY_HEADER_AND_FUNC, zlib.h, 0, -lz) , 0)
  $(info * Detected the zlib library, setting HAVE_ZLIB)
  CFLAGS+= -DHAVE_ZLIB
  LDFLAGS+= -lz
endif

endif #TEST4ZLIB
#############################################################################
# PostgreSQL Library Detection
# (no need to edit)
#############################################################################
ifneq ($(strip $(TEST4PG)),)

ifeq ($(call TRY_HEADER_AND_FUNC, libpq-fe.h, 0, -lpg) , 0)
  $(info * Detected the PostgreSQL library, setting HAVE_POSTGRESQL)
  CFLAGS+=-DHAVE_POSTGRESQL
  LDFLAGS+=-lpg
else ifeq ($(call TRY_HEADER_AND_FUNC, "/usr/include/postgresql/libpq-fe.h", 0, "-lpg") , 0)
  $(info * Detected the PostgreSQL library, setting HAVE_POSTGRESQL)
  CFLAGS+=-DHAVE_POSTGRESQL -I/usr/include/postgresql
  LDFLAGS+=-lpg
endif

endif # TEST4PG
# #############################################################################
# SQLite3 Library Detection
# (no need to edit)
#############################################################################
ifneq ($(strip $(TEST4SQLITE3)),)

ifeq ($(call TRY_HEADER_AND_FUNC, sqlite3.h, sqlite3_open, -lsqlite3) , 0)
  $(info * Detected the SQLite3 library, setting HAVE_SQLITE3)
  CFLAGS+= -DHAVE_SQLITE3
  LDFLAGS+=-lsqlite3
endif

endif #TEST4SQLITE3

#############################################################################
# Debug build flags (defined after all detection so -DHAVE_OPENSSL and
# similar flags are preserved when switching to debug mode)
#############################################################################

DEBUG_CFLAGS:=$(CFLAGS) -O0 -DDEBUG=1 -fno-builtin $(WARNINGS) -I$(SRC_DIR) -I.

#############################################################################
# Help
#############################################################################

help:
	@echo "Available targets:"
	@echo "  all             - Build the project and copy headers (default)"
	@echo "  test / tests    - Build and run tests"
	@echo "  examples        - Build and run examples"
	@echo "  extras          - Build and run extras"
	@echo "  format          - Format code with clang-format"
	@echo "  lint            - Lint code with clang-tidy"
	@echo "  lib             - Build static (libfio.a) and shared (libfio.dylib/.so) libraries"
	@echo "  install         - Install binary and headers to $(INSTALL_PREFIX)"
	@echo "  clean           - Remove build artifacts"
	@echo "  help            - Show this help message"

#############################################################################
# Cleanup
#############################################################################
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(PROJECT)
	@echo "Clean complete!"

