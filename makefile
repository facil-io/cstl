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
WARNINGS=-Wshadow -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -Wformat-security
# Compiler and linker flags
# Consider: -O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize
OPTIMIZATION=-O3 -DNDEBUG -DNODEBUG
# CFLAGS in debug mode. i.e.: -fsanitize=thread -fsanitize=undefined -fsanitize=address -coverage
DEBUG_CFLAGS:=$(CFLAGS) -O0 -DDEBUG=1 -fno-builtin $(WARNINGS) -I$(SRC_DIR) -I.
# CFLAGS in production mode.
CFLAGS+=$(OPTIMIZATION) $(WARNINGS) -I$(SRC_DIR) -I.
LDFLAGS+= -lm

# Main executable
PROJECT = $(BUILD_DIR)/$(NAME)

# combining the fio-stl headers to a single file: fio-stl
LIB_CONCAT_FOLDER=fio-stl
LIB_CONCAT_TARGET=fio-stl

# Source files
SOURCES = $(shell find $(SRC_DIR) -name "*.c" -type f 2>/dev/null | sed 's/ /\\ /g')
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Install paths
INSTALL_BIN = $(INSTALL_PREFIX)/bin
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include/$(NAME)

#############################################################################
# Basics
#############################################################################

# Targets
.PHONY: all clean test format lint install install-headers everything___ help set_debug_flags $(TEST_DIR) $(TEST_DIR)/%

# Default target
all: everything___
	@echo "Library headers installed to $(BUILD_DIR)/include/"

# Build main executable (only if source files exist)
ifneq ($(strip $(SOURCES)),)
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

#############################################################################
# Tests
#############################################################################

# Test files
TEST_SOURCES = $(shell find $(TEST_DIR) -name "*.c" -type f 2>/dev/null | sed 's/ /\\ /g')
TEST_TARGETS = $(TEST_SOURCES:$(TEST_DIR)/%=$(BUILD_DIR)/$(TEST_DIR)/%)
TEST_BINS = $(TEST_TARGETS:%.c=%)
AFTER_TEST_MESSAGE="\n$(DESCRIPTION) is brought to you by \x1B[1m$(AUTHOR)\x1B[0m.\n\x1B[1mValue deserves to be valued.\x1B[0m\n(please consider code contributions / donations)\n"

# Build test binaries (each test file becomes a separate executable)
$(BUILD_DIR)/$(TEST_DIR)/%: $(TEST_DIR)/%.c $(OBJECTS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "* Compiling Test: $*.c"
	@$(CC) $(CFLAGS) -o $@ $< $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS)) $(LDFLAGS)


# Build and run tests
test: | all $(TEST_BINS)
	@echo ""
	@if [ -n "$(TEST_BINS)" ]; then \
		echo "Running test suite..."; \
		test_count=0; \
		pass_count=0; \
		fail_count=0; \
		for test_bin in $(TEST_BINS); do \
			test_count=$$((test_count + 1)); \
			test_name=$$(basename $$test_bin); \
			echo "  [$$test_count] Running $$test_name..."; \
			if $$test_bin; then \
				echo "      ✓ PASS"; \
				pass_count=$$((pass_count + 1)); \
			else \
				echo "      ✗ FAIL"; \
				fail_count=$$((fail_count + 1)); \
			fi; \
			echo ""; \
		done; \
		echo ""; \
		echo "Test Results: $$pass_count passed, $$fail_count failed, $$test_count total"; \
		if [ $$fail_count -gt 0 ]; then exit 1; fi; \
	else \
		echo "No test files found in $(TEST_DIR)/"; \
	fi
	@echo "All tests complete!"
	@echo $(AFTER_TEST_MESSAGE)

$(TEST_DIR)/%: $(BUILD_DIR)/$(TEST_DIR)/%
	@echo "Running test $@"
	@echo "=================================="
	@if $(BUILD_DIR)/$@; then \
		echo " ✓ PASS";         \
	 else                     \
		echo " ✗ FAIL";         \
	 fi;
	@echo $(AFTER_TEST_MESSAGE) 

$(TEST_DIR): test;

#############################################################################
# Examples
#############################################################################
# Test files
EXAMPLES_SOURCES = $(shell find $(EXAMPLES_DIR) -name "*.c" -type f 2>/dev/null | sed 's/ /\\ /g')
EXAMPLES_BINS = $(EXAMPLES_SOURCES:$(EXAMPLES_DIR)/%.c=$(BUILD_DIR)/$(EXAMPLES_DIR)/%)

# Build test binaries (each test file becomes a separate executable)
$(BUILD_DIR)/$(EXAMPLES_DIR)/%: $(EXAMPLES_DIR)/%.c $(OBJECTS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "* Compiling Example: $*.c"
	@$(CC) $(CFLAGS) -o $@ $< $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS)) $(LDFLAGS)

$(EXAMPLES_DIR)/%: $(BUILD_DIR)/$(EXAMPLES_DIR)/%
	@echo "Running $@"
	@echo "=================================="
	@$(BUILD_DIR)/$@

$(EXAMPLES_DIR): $(EXAMPLES_BINS)
	@if [ -n "$(EXAMPLES_BINS)" ]; then \
		for e_bin in $(EXAMPLES_BINS); do \
		  echo "";                        \
			echo "=================================="; \
			echo "Running $$e_bin";         \
			echo "=================================="; \
			$$e_bin;                        \
		done;                             \
		echo "";                          \
		echo " ✓ Done";                   \
		echo "";                          \
	else                                \
		echo "No example files found in: $(EXAMPLES_DIR)/"; \
	fi


.NOTINTERMEDIATE: $(BUILD_DIR)/$(EXAMPLES_DIR)/% $(EXAMPLES_DIR)/%

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
TRY_COMPILE=$(shell printf $(1) | $(CC) $(INCLUDE_STR) $(CFLAGS) -xc -o /dev/null - $(LDFLAGS) $(2) >> /dev/null 2> /dev/null ; echo $$? 2> /dev/null)
TRY_COMPILE_AND_RUN=$(shell printf $(1) | $(CC) $(INCLUDE_STR) $(CFLAGS) -xc -o ./___$(NAME)_tmp_test_ - $(LDFLAGS) $(2) 2> /dev/null ; ./___fio_tmp_test_ >> /dev/null 2> /dev/null; echo $$?; rm ./___$(NAME)_tmp_test_ 2> /dev/null)
TRY_HEADER_AND_FUNC= $(shell printf "\#include <$(strip $(1))>\\nint main(void) {(void)($(strip $(2)));}" | $(CC) $(INCLUDE_STR) $(CFLAGS) -xc -o /dev/null - $(LDFLAGS) $(3) >> /dev/null 2> /dev/null; echo $$? 2> /dev/null)
EMPTY_STRING:=
# pkg-config
PKG_CONFIG?=pkg-config

#############################################################################
# Detecting SystemV socket libraries
# (no need to edit)
#############################################################################
ifdef TEST4SOCKET

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

ifdef TEST4POLL

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
ifdef TEST4CRYPTO

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
ifdef TEST4ZLIB

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
ifdef TEST4PG

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
ifdef TEST4SQLITE3

ifeq ($(call TRY_HEADER_AND_FUNC, sqlite3.h, sqlite3_open, -lsqlite3) , 0)
  $(info * Detected the SQLite3 library, setting HAVE_SQLITE3)
  CFLAGS+= -DHAVE_SQLITE3
  LDFLAGS+=-lsqlite3
endif

endif #TEST4SQLITE3
#############################################################################
# Help
#############################################################################

help:
	@echo "Available targets:"
	@echo "  all             - Build the project and copy headers (default)"
	@echo "  test            - Build and run tests"
	@echo "  format          - Format code with clang-format"
	@echo "  lint            - Lint code with clang-tidy"
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
