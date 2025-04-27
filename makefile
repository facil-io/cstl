#############################################################################
# This makefile was composed for facil.io
#
# This makefile can (and does) act as a "configure.sh" replacement for testing
# if libraries or language features are available... it should have everything,
# including the kitchen sink.
#
# This makefile SHOULD be easily portable and SHOULD work on any POSIX system
# for any project... under the following assumptions:
#
# * If your code has a `main` function, that code should be placed in the
#   `MAIN_ROOT` folder and/or `MAIN_SUBFOLDERS` (i.e., `./src`).
#
# * If your code provides a library style API, the published functions are
#   in the `LIB_ROOT` folder and/or `LIB_XXX_SUBFOLDERS` (i.e., `./lib`).
#
# * If you want to concat a number of header / source / markdown (docs) files
#   than place them in the `LIB_CONCAT_FOLDER`.
#
# * Test files are independent, each test files compiles and runs as is, and
#   placed in the `TEST_ROOT` folder (i.e., `./tests`).
#
#   Run tests (i.e., the test file `foo.c`) with:       `make tests/foo`
#   Run tests with DEBUG mode (no optimizations) with:  `make db/tests/foo`
#
# Copyright (c) 2016-2022 Boaz Segev
# License MIT / ISC (choose whichever you like)
#############################################################################
FIO_VERSION:= 0.8.x
#############################################################################
# Compilation Output Settings
#############################################################################

# binary name and location
NAME?=app

# a temporary folder that will be cleared out and deleted between fresh builds
# All object files will be placed in this folder
TMP_ROOT=tmp

# destination folder for the final compiled output
DEST?=$(TMP_ROOT)

# output folder for `make libdump` - dumps all library files (not source files)
# this creates three folders: `src`, `include` and `all` .
DUMP_LIB?=libdump

#############################################################################
# Source Code Folder Settings
#############################################################################

# the main source .c and .cpp source files root folder
MAIN_ROOT?=src
# subfolders under the main source root
MAIN_SUBFOLDERS?=

#############################################################################
# Library Folder Settings (library files aren't auto-compiled)
#############################################################################

# the library .c, .cpp and .so (or .dll) source files root folder
LIB_ROOT?=lib

# publicly used subfolders in the lib root
LIB_PUBLIC_SUBFOLDERS?=

# privately used subfolders in the lib root (this distinction is only relevant for CMake)
LIB_PRIVATE_SUBFOLDERS?=

# When exporting the library, what would it be called
LIB_NAME?=libfacil


#############################################################################
# Single Library Concatenation
#############################################################################

# a folder containing code that should be unified into a single file
#
# Note: files will be unified in the same order the system provides (usually, file name)
LIB_CONCAT_FOLDER?=fio-stl

# the path and file name to use when unifying *.c, *.h, and *.md files (without extension).
LIB_CONCAT_TARGET?=fio-stl

#############################################################################
# Test Source Code Folder
#############################################################################

# Testing folder
TEST_ROOT=tests

# The default test file to run when running: make test (without the C extension)
TEST_DEFAULT=stl

# Examples folder name
EXAMPLES_ROOT=examples

#############################################################################
# Makefile Runtime Tests (sets flags, such as HAVE_OPENSSL)
#############################################################################
# Tests are performed unless the value is empty / missing
TEST4ENDIAN:=     # __BIG_ENDIAN__=?
TEST4TM_ZONE:=    # HAVE_TM_TM_ZONE
TEST4SOCKET:=     # --- tests for socket library linker flags
TEST4SENDFILE:=   # HAVE_SENDFILE
TEST4POLL:=       # HAVE_KQUEUE / HAVE_EPOLL / HAVE_POLL
TEST4CRYPTO:=1    # HAVE_OPENSSL / HAVE_SODIUM
TEST4ZLIB:=1      # HAVE_ZLIB
TEST4PG:=         # HAVE_POSTGRESQL
TEST4SQLITE3:=    # HAVE_SQLITE3

#############################################################################
# Compiler / Linker Settings
#############################################################################

# any libraries required (only names, omit the "-l" at the beginning)
ifeq ($(OS),Windows_NT)
  # Windows libraries
  LINKER_LIBS=Ws2_32
  LIB_EXT=dll
else
  # POSIX libraries
  LINKER_LIBS=pthread m
  LIB_EXT=so
endif
# optimization level. (-march=native fails with clang on some ARM compilers)
# Consider: -O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize
OPTIMIZATION=-O3 -DNDEBUG -DNODEBUG
# optimization level in debug mode. i.e.: -fsanitize=thread -fsanitize=undefined -fsanitize=address
OPTIMIZATION_DEBUG=-O0 -g -coverage -fno-omit-frame-pointer -fno-builtin
# Warnings... i.e. -Wpedantic -Weverything -Wno-format-pedantic -Wshorten-64-to-32
WARNINGS=-Wshadow -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -Wformat-security
# any extra include folders, space separated list. (i.e. `pg_config --includedir`)
INCLUDE=.
# any preprocessor defined flags we want, space separated list (i.e. DEBUG )
FLAGS:=FIO_LEAK_COUNTER
# C specific compiler options
C_EXTRA_OPT:=
# C++ specific compiler options
CXX_EXTRA_OPT:=-Wno-keyword-macro -Wno-vla-extension -Wno-c99-extensions -Wno-zero-length-array -Wno-variadic-macros -Wno-missing-braces
# c standard   (if any, without the `-std=` prefix, i.e.: c11)
CSTD?=gnu11
# c++ standard (if any, without the `-std=` prefix, i.e.: c++11)
CXXSTD?=gnu++11
# pkg-config
PKG_CONFIG?=pkg-config

#############################################################################
# CMake Support
#############################################################################

# The library details for CMake incorporation. Can be safely removed.
CMAKE_FILENAME?=
# Project name to be stated in the CMakeFile
CMAKE_PROJECT=facil.io
# Space delimited list of required packages
CMAKE_REQUIRE_PACKAGE=Threads

#############################################################################
# Debug Settings
#############################################################################

# add DEBUG flag if requested
ifeq ($(DEBUG), 1)
  $(info * Detected DEBUG environment flag, enforcing debug mode compilation)
  FLAGS:=$(FLAGS) DEBUG
  # # comment the following line if you want to use a different address sanitizer or a profiling tool.
  OPTIMIZATION:=$(OPTIMIZATION_DEBUG)
  # possibly useful:  -Wconversion -Wcomma -fsanitize=undefined -Wshadow
  # go crazy with clang: -Weverything -Wno-cast-qual -Wno-used-but-marked-unused -Wno-reserved-id-macro -Wno-padded -Wno-disabled-macro-expansion -Wno-documentation-unknown-command -Wno-bad-function-cast -Wno-missing-prototypes
endif

#############################################################################
# OS Specific Settings (debugger, disassembler, etc')
#############################################################################

# Set default C and C++ compilers
ifneq ($(OS),Windows_NT)
  OS:=$(shell uname)
  CC?=gcc
  CXX?=g++
else ifneq (,$(findstring version,$(shell gcc -v 2>&1)))
  CC?=gcc
  CXX?=g++
else
  $(warning *** Windows systems might not work with this makefile / library.)
  CC?=cl
  CXX?=cl
endif

ifeq ($(OS),Darwin) # Run MacOS commands
  # debugger
  DB=lldb
  # disassemble tool. Use stub to disable.
  DISAMS=otool -dtVGX
  # documentation commands
  # DOCUMENTATION=cldoc generate $(INCLUDE_STR) -- --output ./html $(foreach dir, $(LIB_PUBLIC_SUBFOLDERS), $(wildcard $(addsuffix /, $(basename $(dir)))*.h*))
  # rule modifier (can't be indented)
$(DEST)/$(LIB_NAME).$(LIB_EXT): LDFLAGS+=-dynamiclib -install_name $(realpath $(DEST))/$(LIB_NAME).$(LIB_EXT)
else
  # debugger
  DB=gdb
  # disassemble tool, leave undefined.
  # DISAMS=otool -tVX
  # documentation commands
  DOCUMENTATION=
endif

#############################################################################
# Automatic Setting Expansion
# (don't edit)
#############################################################################

BIN=$(DEST)/$(NAME)

LIBDIR_PUB=$(LIB_ROOT) $(foreach dir, $(LIB_PUBLIC_SUBFOLDERS), $(addsuffix /,$(basename $(LIB_ROOT)))$(dir))
LIBDIR_PRIV=$(foreach dir, $(LIB_PRIVATE_SUBFOLDERS), $(addsuffix /,$(basename $(LIB_ROOT)))$(dir))

LIBDIR=$(LIBDIR_PUB) $(LIBDIR_PRIV)
LIBSRC=$(foreach dir, $(LIBDIR), $(wildcard $(addsuffix /, $(basename $(dir)))*.c*))

MAINDIR=$(MAIN_ROOT) $(foreach main_root, $(MAIN_ROOT) , $(foreach dir, $(MAIN_SUBFOLDERS), $(addsuffix /,$(basename $(main_root)))$(dir)))
MAINSRC=$(foreach dir, $(MAINDIR), $(wildcard $(addsuffix /, $(basename $(dir)))*.c*))

FOLDERS=$(LIBDIR) $(MAINDIR) $(TEST_ROOT)
SOURCES=$(LIBSRC) $(MAINSRC)

BUILDTREE=$(TMP_ROOT) $(TMP_ROOT)/$(TEST_ROOT) $(TMP_ROOT)/$(EXAMPLES_ROOT) $(foreach dir, $(FOLDERS), $(addsuffix /, $(basename $(TMP_ROOT)))$(basename $(dir)))

ifeq ($(OS),Windows_NT)
# Windows libraries
# BUILDTREE=$(subst /,\,$(BUILDTREE))
endif

CCL=$(CC)

INCLUDE_STR=$(foreach dir,$(INCLUDE),$(addprefix -I, $(dir))) $(foreach dir,$(FOLDERS),$(addprefix -I, $(dir)))

MAIN_OBJS=$(foreach source, $(MAINSRC), $(addprefix $(TMP_ROOT)/, $(addsuffix .o, $(basename $(source)))))
LIB_OBJS=$(foreach source, $(LIBSRC), $(addprefix $(TMP_ROOT)/, $(addsuffix .o, $(basename $(source)))))

OBJS_DEPENDENCY=$(LIB_OBJS:.o=.d) $(MAIN_OBJS:.o=.d) 

LINKER_LIBS_EXT=

#############################################################################
# Combining single-file library
#############################################################################

ifdef LIB_CONCAT_FOLDER
ifdef LIB_CONCAT_TARGET
ifneq ($(OS),Windows_NT)
# POSIX implementation

LIB_CONCAT_HEADERS=$(wildcard $(LIB_CONCAT_FOLDER)/*.h)
LIB_CONCAT_SOURCES=$(wildcard $(LIB_CONCAT_FOLDER)/*.c)
LIB_CONCAT_DOCS=$(wildcard $(LIB_CONCAT_FOLDER)/*.md)
ifneq ($(LIB_CONCAT_HEADERS), $(EMPTY))
  $(info * Building single-file header: $(LIB_CONCAT_TARGET).h)
  $(shell rm $(LIB_CONCAT_TARGET).h 2> /dev/null)
  $(shell cat $(LIB_CONCAT_FOLDER)/*.h >> $(LIB_CONCAT_TARGET).h)
endif
ifneq ($(LIB_CONCAT_SOURCES), $(EMPTY))
  $(info * Building single-file C source: $(LIB_CONCAT_TARGET).c)
  $(shell rm $(LIB_CONCAT_TARGET).c 2> /dev/null)
  $(shell cat $(LIB_CONCAT_FOLDER)/*.c >> $(LIB_CONCAT_TARGET).c)
endif
ifneq ($(LIB_CONCAT_DOCS), $(EMPTY))
  $(info * Building documentation: $(LIB_CONCAT_TARGET).md)
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
TRY_COMPILE_AND_RUN=$(shell printf $(1) | $(CC) $(INCLUDE_STR) $(CFLAGS) -xc -o ./___fio_tmp_test_ - $(LDFLAGS) $(2) 2> /dev/null ; ./___fio_tmp_test_ >> /dev/null 2> /dev/null; echo $$?; rm ./___fio_tmp_test_ 2> /dev/null)
TRY_HEADER_AND_FUNC= $(shell printf "\#include <$(strip $(1))>\\nint main(void) {(void)($(strip $(2)));}" | $(CC) $(INCLUDE_STR) $(CFLAGS) -xc -o /dev/null - $(LDFLAGS) $(3) >> /dev/null 2> /dev/null; echo $$? 2> /dev/null)

#############################################################################
# GCC bug handling.
#
# GCC might trigger a bug when -fipa-icf is enabled and (de)constructor
# functions are used (as in our case with -O2 or above).
#
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70306
#############################################################################


ifneq ($(strip $(CC)),gcc)
ifeq ($(shell $(CC) -v 2>&1 | grep -o "^gcc version [0-7]\." | grep -o "^gcc version"),gcc version)
  OPTIMIZATION+=-fno-ipa-icf
  $(info * Disabled `-fipa-icf` optimization, might be buggy with this gcc version.)
endif
endif

#############################################################################
# Endian  Detection
# (no need to edit)
#############################################################################
ifdef TEST4ENDIAN

ifeq ($(call TRY_COMPILE_AND_RUN, "int main(void) {int i = 1; return (int)(i & ((unsigned char *)&i)[sizeof(i)-1]);}\n",$(EMPTY)), 1)
  $(info * Detected Big Endian byte order.)
  FLAGS:=$(FLAGS) __BIG_ENDIAN__
else ifeq ($(call TRY_COMPILE_AND_RUN, "int main(void) {int i = 1; return (int)(i & ((unsigned char *)&i)[0]);}\n",$(EMPTY)), 1)
  $(info * Detected Little Endian byte order.)
  FLAGS:=$(FLAGS) __BIG_ENDIAN__=0
else
  $(info * Byte ordering (endianness) detection failed)
endif

endif # TEST4ENDIAN
#############################################################################
# Detecting 'struct tm' fields
# (no need to edit)
#############################################################################
ifdef TEST4TM_ZONE

FIO_TEST_STRUCT_TM_TM_ZONE:="\\n\
\#define _GNU_SOURCE\\n\
\#include <time.h>\\n\
int main(void) {\\n\
  struct tm tm;\\n\
  tm.tm_zone = \"UTC\";\\n\
  return 0;\\n\
}\\n\
"

ifeq ($(call TRY_COMPILE, $(FIO_TEST_STRUCT_TM_TM_ZONE), $(EMPTY)), 0)
  $(info * Detected 'tm_zone' field in 'struct tm')
  FLAGS:=$(FLAGS) HAVE_TM_TM_ZONE=1
endif

endif # TEST4TM_ZONE
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
#############################################################################
# Detecting The `sendfile` System Call
# (no need to edit)
#############################################################################
ifdef TEST4SENDFILE

# Linux variation
FIO_SENDFILE_TEST_LINUX:="\\n\
\#define _GNU_SOURCE\\n\
\#include <stdlib.h>\\n\
\#include <stdio.h>\\n\
\#include <sys/sendfile.h>\\n\
int main(void) {\\n\
  off_t offset = 0;\\n\
  ssize_t result = sendfile(2, 1, (off_t *)&offset, 300);\\n\
}\\n\
"

# BSD variation
FIO_SENDFILE_TEST_BSD:="\\n\
\#define _GNU_SOURCE\\n\
\#include <stdlib.h>\\n\
\#include <stdio.h>\\n\
\#include <sys/types.h>\\n\
\#include <sys/socket.h>\\n\
\#include <sys/uio.h>\\n\
int main(void) {\\n\
  off_t sent = 0;\\n\
  off_t offset = 0;\\n\
  ssize_t result = sendfile(2, 1, offset, (size_t)sent, NULL, &sent, 0);\\n\
}\\n\
"

# Apple variation
FIO_SENDFILE_TEST_APPLE:="\\n\
\#define _GNU_SOURCE\\n\
\#include <stdlib.h>\\n\
\#include <stdio.h>\\n\
\#include <sys/types.h>\\n\
\#include <sys/socket.h>\\n\
\#include <sys/uio.h>\\n\
int main(void) {\\n\
  off_t sent = 0;\\n\
  off_t offset = 0;\\n\
  ssize_t result = sendfile(2, 1, offset, &sent, NULL, 0);\\n\
}\\n\
"

ifeq ($(call TRY_COMPILE, $(FIO_SENDFILE_TEST_LINUX), $(EMPTY)), 0)
  $(info * Detected `sendfile` (Linux))
  FLAGS+=USE_SENDFILE_LINUX HAVE_SENDFILE
else ifeq ($(call TRY_COMPILE, $(FIO_SENDFILE_TEST_BSD), $(EMPTY)), 0)
  $(info * Detected `sendfile` (BSD))
  FLAGS+=USE_SENDFILE_BSD HAVE_SENDFILE
else ifeq ($(call TRY_COMPILE, $(FIO_SENDFILE_TEST_APPLE), $(EMPTY)), 0)
  $(info * Detected `sendfile` (Apple))
  FLAGS+=USE_SENDFILE_APPLE HAVE_SENDFILE
else
  $(info * No `sendfile` support detected.)
  FLAGS:=$(FLAGS) USE_SENDFILE=0
endif

endif # TEST4SENDFILE
#############################################################################
# kqueue / epoll / poll Selection / Detection
# (no need to edit)
#############################################################################
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
  FLAGS+=FIO_ENGINE_POLL HAVE_POLL
else ifdef FIO_FORCE_POLL
  $(info * Skipping polling tests, enforcing manual selection of: poll)
  FLAGS+=FIO_ENGINE_POLL HAVE_POLL
else ifdef FIO_FORCE_EPOLL
  $(info * Skipping polling tests, enforcing manual selection of: epoll)
  FLAGS+=FIO_ENGINE_EPOLL HAVE_EPOLL
else ifdef FIO_FORCE_KQUEUE
  $(info * Skipping polling tests, enforcing manual selection of: kqueue)
  FLAGS+=FIO_ENGINE_KQUEUE HAVE_KQUEUE
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

# default OpenSSL flags
OPENSSL_CFLAGS:=
OPENSSL_LIBS:=
OPENSSL_LDFLAGS:="-lssl" "-lcrypto"
# detect OpenSSL flags using pkg-config, if available
ifeq ($(shell $(PKG_CONFIG) -- openssl >/dev/null 2>&1; echo $$?), 0)
  OPENSSL_CFLAGS:=$(shell $(PKG_CONFIG) --cflags openssl)
  OPENSSL_LDFLAGS:=$(shell $(PKG_CONFIG) --libs openssl)
endif
ifeq ($(shell $(PKG_CONFIG) -- libsodium >/dev/null 2>&1; echo $$?), 0)
  LIBSODIUM_CFLAGS:=$(shell $(PKG_CONFIG) --cflags libsodium)
  LIBSODIUM_LDFLAGS:=$(shell $(PKG_CONFIG) --libs libsodium)
else
  LIBSODIUM_CFLAGS:=
  LIBSODIUM_LDFLAGS:=-lsodium
endif


# add TLS library flags (TODO? non-exclusive?)
ifdef FIO_NO_TLS
  $(info * Skipping crypto library detection.)
else ifeq ($(call TRY_COMPILE, $(FIO_TLS_TEST_OPENSSL), $(OPENSSL_CFLAGS) $(OPENSSL_LDFLAGS)), 0)
  $(info * Detected the OpenSSL library, setting HAVE_OPENSSL)
  FLAGS+=HAVE_OPENSSL FIO_TLS_FOUND
  LINKER_LIBS_EXT:=$(LINKER_LIBS_EXT) $(OPENSSL_LIBS)
  LDFLAGS+=$(OPENSSL_LDFLAGS)
  CFLAGS+=$(OPENSSL_CFLAGS)
  CXXFLAGS+=$(OPENSSL_CFLAGS)
  PKGC_REQ_OPENSSL=openssl >= 3.0, openssl < 4.0
  PKGC_REQ+=$$(PKGC_REQ_OPENSSL)
else ifeq ($(call TRY_COMPILE, "\#include <sodium.h.h>\\n int main(void) {}", $(LIBSODIUM_CFLAGS) $(LIBSODIUM_LDFLAGS)) , 0)
  # Sodium Crypto Library: https://doc.libsodium.org/usage
  $(info * Detected the Sodium library, setting HAVE_SODIUM)
  FLAGS:=$(FLAGS) HAVE_SODIUM
  LDFLAGS+=$(LIBSODIUM_LDFLAGS)
  CFLAGS+=$(LIBSODIUM_CFLAGS)
  CXXFLAGS+=$(LIBSODIUM_CFLAGS)
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
  FLAGS:=$(FLAGS) HAVE_ZLIB
  LINKER_LIBS_EXT:=$(LINKER_LIBS_EXT) z
  PKGC_REQ_ZLIB=zlib
  PKGC_REQ+=$$(PKGC_REQ_ZLIB)
endif

endif #TEST4ZLIB
#############################################################################
# PostgreSQL Library Detection
# (no need to edit)
#############################################################################
ifdef TEST4PG

ifeq ($(call TRY_HEADER_AND_FUNC, libpq-fe.h, 0, -lpg) , 0)
  $(info * Detected the PostgreSQL library, setting HAVE_POSTGRESQL)
  FLAGS:=$(FLAGS) HAVE_POSTGRESQL
  LINKER_LIBS_EXT:=$(LINKER_LIBS_EXT) pg
else ifeq ($(call TRY_HEADER_AND_FUNC, "/usr/include/postgresql/libpq-fe.h", 0, "-lpg") , 0)
  $(info * Detected the PostgreSQL library, setting HAVE_POSTGRESQL)
  FLAGS:=$(FLAGS) HAVE_POSTGRESQL
  INCLUDE_STR:=$(INCLUDE_STR) -I/usr/include/postgresql
  LINKER_LIBS_EXT:=$(LINKER_LIBS_EXT) pg
endif

endif # TEST4PG
# #############################################################################
# SQLite3 Library Detection
# (no need to edit)
#############################################################################
ifdef TEST4SQLITE3

ifeq ($(call TRY_HEADER_AND_FUNC, sqlite3.h, sqlite3_open, -lsqlite3) , 0)
  $(info * Detected the SQLite3 library, setting HAVE_SQLITE3)
  FLAGS:=$(FLAGS) HAVE_SQLITE3
  LINKER_LIBS_EXT:=$(LINKER_LIBS_EXT) sqlite3
  PKGC_REQ_ZLIB=sqlite3
  PKGC_REQ+=$$(PKGC_REQ_ZLIB)
endif

endif #TEST4SQLITE3
#############################################################################
# Updated flags and final values
# (don't edit)
#############################################################################
FLAGS_STR=$(foreach flag,$(FLAGS),$(addprefix -D, $(flag)))

ifeq ($(OS),Windows_NT)
# Windows
INCLUDE_STR:=$(subst /,\,$(INCLUDE_STR))
CFLAGS:=$(CFLAGS) -g -std=$(CSTD) $(FLAGS_STR) $(WARNINGS) $(INCLUDE_STR) $(C_EXTRA_OPT)
CXXFLAGS:=$(CXXFLAGS) -std=$(CXXSTD)  $(FLAGS_STR) $(WARNINGS) $(INCLUDE_STR) $(CXX_EXTRA_OPT)
else
#POSIX
CFLAGS:=$(CFLAGS) -g -std=$(CSTD) -fpic $(FLAGS_STR) $(WARNINGS) $(INCLUDE_STR) $(C_EXTRA_OPT)
CXXFLAGS:=$(CXXFLAGS) -std=$(CXXSTD) -fpic  $(FLAGS_STR) $(WARNINGS) $(INCLUDE_STR) $(CXX_EXTRA_OPT)
endif

LINKER_FLAGS=$(LDFLAGS) $(foreach lib,$(LINKER_LIBS),$(addprefix -l,$(lib))) $(foreach lib,$(LINKER_LIBS_EXT),$(addprefix -l,$(lib)))
CFLAGS_DEPENDENCY=-MT $@ -MMD -MP

# Build a "Requires:" string for the pkgconfig/facil.pc file
# unfortunately, leading or trailing commas are interpreted as
# "empty package name" by pkg-config, therefore we work around this by using
# $(strip ..).
# The following 2 lines are from the manual of GNU make
nullstring :=
space := $(nullstring) # end of line
comma := ,
$(eval PKGC_REQ_EVAL:=$(subst $(space),$(comma) ,$(strip $(PKGC_REQ))))

#############################################################################
# Task Settings - Compile Time Measurement
#############################################################################
TIME_TEST_CMD:=which time
ifeq ($(call TRY_RUN, $(TIME_TEST_CMD), $(EMPTY)), 0)
  CC:=time $(CC)
  CXX:=time $(CXX)
endif

#############################################################################
# Tasks - default task
#############################################################################
$(NAME): build

#############################################################################
# Tasks - task modulators
#############################################################################

.PHONY : db/%
db/%: clean set_debug_flags % ;

.PHONY : clean/%
clean/%: clean % ;

#############################################################################
# Tasks - cleanup and tree construction
#############################################################################

ifneq ($(OS),Windows_NT)
# POSIX libraries
.PHONY : create_tree
create_tree:
	-@mkdir -p $(BUILDTREE) 2> /dev/null

.PHONY : clean
clean:
	-@rm -f $(BIN) 2> /dev/null || echo "" >> /dev/null
	-@rm -R -f $(TMP_ROOT) 2> /dev/null || echo "" >> /dev/null
	-@rm -R -f $(DEST) 2> /dev/null || echo "" >> /dev/null
	-@mkdir -p $(BUILDTREE) 2> /dev/null


else
# Windows libraries
.PHONY : create_tree
create_tree:
	-@mkdir $(BUILDTREE)

.PHONY : clean
clean:
	-@del /f /q $(subst /,\,$(BIN))
	-@del /s /f /q $(subst /,\,$(TMP_ROOT))
	-@del /s /f /q $(subst /,\,$(DEST))
	-@mkdir $(BUILDTREE)
endif

#############################################################################
# Tasks - (TODO!) documentation builder
#############################################################################

.PHONY : documentation.%
documentation.%: ;
	@$(DOCUMENTATION)

#############################################################################
# Tasks - compiling
#############################################################################

.PHONY : set_debug_flags
set_debug_flags:
	$(eval OPTIMIZATION=$(OPTIMIZATION_DEBUG))
	$(eval CFLAGS+= -DDEBUG=1 -fno-builtin)
	$(eval CXXFLAGS+= -DDEBUG=1 -fno-builtin)
	$(eval LINKER_FLAGS= -g -DDEBUG=1 $(LINKER_FLAGS))
	@echo "* Set debug flags."

$(TMP_ROOT)/%.d: ;

ifeq ($(DISAMS),)
DISAMS=echo
AFTER_DISAMS___= >> /dev/null 
else
AFTER_DISAMS___= >> $(TMP_ROOT)/$*.s
endif

$(TMP_ROOT)/%.o: %.c $(TMP_ROOT)/%.d | create_tree
	@echo "* Compiling $*.c"
	@$(CC) -c $*.c -o $@ $(CFLAGS_DEPENDENCY) $(CFLAGS) $(OPTIMIZATION)
	@$(DISAMS) $(TMP_ROOT)/$*.o $(AFTER_DISAMS___)

$(TMP_ROOT)/%.o: %.cpp $(TMP_ROOT)/%.d | create_tree
	@echo "* Compiling $*.cpp (C++ source file)"
	@$(CC) -c $*.cpp -o $@ $(CFLAGS_DEPENDENCY) $(CXXFLAGS) $(OPTIMIZATION)
	$(eval CCL=$(CXX))

$(TMP_ROOT)/%.o: %.cxx $(TMP_ROOT)/%.d | create_tree
	@echo "* Compiling $*.cxx (C++ source file)"
	@$(CC) -c $*.cxx -o $@ $(CFLAGS_DEPENDENCY) $(CXXFLAGS) $(OPTIMIZATION)
	$(eval CCL=$(CXX))

$(TMP_ROOT)/%.o: %.c++ $(TMP_ROOT)/%.d | create_tree
	@echo "* Compiling $*.c++ (C++ source file)"
	@$(CC) -c $*.c++ -o $@ $(CFLAGS_DEPENDENCY) $(CXXFLAGS) $(OPTIMIZATION)
	$(eval CCL=$(CXX))

-include $(OBJS_DEPENDENCY)

#############################################################################
# Tasks - Building Source (LIB_OBJS + MAIN_OBJS)
#############################################################################

link.%:
	@echo "* Linking... ($(DEST)/$*)"
	@$(CCL) -o $(DEST)/$* $(MAIN_OBJS) $(OPTIMIZATION) $(LINKER_FLAGS)

build_start.%: create_tree ;

build_finish.%: link.% documentation.%
	@echo "* Finished build ($(DEST)/$*)"

run.%: link.%
	@$(DEST)/$*

build: build_start.$(NAME) $(MAIN_OBJS) build_finish.$(NAME);

run: build run.$(NAME) ;


#############################################################################
# Tasks - Library Build (LIB_OBJS) (TODO!)
#############################################################################

build_lib_objects: $(LIB_OBJS) ;

link_lib:
	@$(CCL) -shared -o $(DEST)/$(LIB_NAME).$(LIB_EXT) $(LIB_OBJS) $(OPTIMIZATION) $(LINKER_FLAGS)
	@$(DOCUMENTATION)


lib: create_tree build_lib_objects link_lib documentation.all;


# $(DEST)/pkgconfig/$(LIB_NAME).pc: makefile | libdump
# 	@mkdir -p $(DEST)/pkgconfig && \
# 	printf "\
# Name: facil.io\\n\
# Description: facil.io\\n\
# Cflags: -I%s\\n\
# Libs: -L%s -lfacil\\n\
# Version: %s\\n\
# Requires.private: %s\\n\
# " $(realpath $(DEST)/../libdump/include) $(realpath $(DEST)) $(FIO_VERSION) "$(PKGC_REQ_EVAL)" > $@

# $(DEST)/$(LIB_NAME).$(LIB_EXT): build_lib_objects | $(DEST)/pkgconfig/$(LIB_NAME).pc


#############################################################################
# Tasks - Testing
#
# Tasks (order of appearance in code is different, due to matching rules):
#
# - test             compile and runs the default test
# - tests/cpp        tests CPP compatibility
# - tests/XXX        compile and runs XXX.c
#
#############################################################################

.SECONDARY: ;

tests_set_env.%: create_tree
	$(eval CFLAGS+=-DTEST=1 -DFIO_WEAK_TLS)
	$(eval CXXFLAGS+=-DTEST=1 -DFIO_WEAK_TLS)
	$(eval MAIN_OBJS=$(TMP_ROOT)/$(TEST_ROOT)/$*.o)
	@echo "* Set testing flags ($*)"

# tests_build.XXX will compile and link tests/XXX.c
tests_build.%: tests_set_env.% build_start.% $(TMP_ROOT)/$(TEST_ROOT)/%.o build_finish.% ;

tests/cpp: tests_set_env.cpp build_start.cpp $(TMP_ROOT)/$(TEST_ROOT)/cpp.o build_finish.cpp
	@echo "* Compilation of C++ variation successful."

# tests/build/XXX will compile and run tests/XXX.c
tests/%: tests_build.% run.% ;

ifneq ($(TEST_DEFAULT),)

test: tests/$(TEST_DEFAULT) ;

endif

#############################################################################
# Tasks - Examples
#############################################################################

examples_set_env.%: create_tree
	$(eval MAIN_OBJS=$(TMP_ROOT)/$(EXAMPLES_ROOT)/$*.o)
	@echo "* Set example flags ($*)"

# examples_build.XXX will compile and link examples/XXX.c
examples_build.%: examples_set_env.% build_start.% $(TMP_ROOT)/$(EXAMPLES_ROOT)/%.o build_finish.% ;

# examples/build/XXX will compile and run examples/XXX.c
examples/%: examples_build.% run.% ;

#############################################################################
# Tasks - library code dumping
#############################################################################

ifndef DUMP_LIB
.PHONY : libdump
libdump: cmake

else

ifeq ($(LIBDIR_PRIV),)

.PHONY : libdump
libdump: cmake
	-@rm -R $(DUMP_LIB) 2> /dev/null
	-@mkdir $(DUMP_LIB)
	-@mkdir $(DUMP_LIB)/src
	-@mkdir $(DUMP_LIB)/include
	-@mkdir $(DUMP_LIB)/all  # except README.md files
	-@cp -n $(foreach dir,$(LIBDIR_PUB), $(wildcard $(addsuffix /, $(basename $(dir)))*.[^m]*)) $(DUMP_LIB)/all 2> /dev/null
	-@cp -n $(foreach dir,$(LIBDIR_PUB), $(wildcard $(addsuffix /, $(basename $(dir)))*.h*)) $(DUMP_LIB)/include 2> /dev/null
	-@cp -n $(foreach dir,$(LIBDIR_PUB), $(wildcard $(addsuffix /, $(basename $(dir)))*.[^hm]*)) $(DUMP_LIB)/src 2> /dev/null

else

.PHONY : libdump
libdump: cmake
	-@rm -R $(DUMP_LIB) 2> /dev/null
	-@mkdir $(DUMP_LIB)
	-@mkdir $(DUMP_LIB)/src
	-@mkdir $(DUMP_LIB)/include
	-@mkdir $(DUMP_LIB)/all  # except README.md files
	-@cp -n $(foreach dir,$(LIBDIR_PUB), $(wildcard $(addsuffix /, $(basename $(dir)))*.[^m]*)) $(DUMP_LIB)/all 2> /dev/null
	-@cp -n $(foreach dir,$(LIBDIR_PUB), $(wildcard $(addsuffix /, $(basename $(dir)))*.h*)) $(DUMP_LIB)/include 2> /dev/null
	-@cp -n $(foreach dir,$(LIBDIR_PUB), $(wildcard $(addsuffix /, $(basename $(dir)))*.[^hm]*)) $(DUMP_LIB)/src 2> /dev/null
	-@cp -n $(foreach dir,$(LIBDIR_PRIV), $(wildcard $(addsuffix /, $(basename $(dir)))*.[^m]*)) $(DUMP_LIB)/all 2> /dev/null
	-@cp -n $(foreach dir,$(LIBDIR_PRIV), $(wildcard $(addsuffix /, $(basename $(dir)))*.h*)) $(DUMP_LIB)/include 2> /dev/null
	-@cp -n $(foreach dir,$(LIBDIR_PRIV), $(wildcard $(addsuffix /, $(basename $(dir)))*.[^hm]*)) $(DUMP_LIB)/src 2> /dev/null

endif
endif

#############################################################################
# Tasks - CMake
#############################################################################
ifndef CMAKE_FILENAME
.PHONY : cmake
cmake:
	@echo 'Missing CMake variables'

else

.PHONY : cmake
cmake:
	-@rm $(CMAKE_FILENAME) 2> /dev/null
	@touch $(CMAKE_FILENAME)
	@echo 'project($(CMAKE_PROJECT))' >> $(CMAKE_FILENAME)  
	@echo '' >> $(CMAKE_FILENAME)
	@echo 'cmake_minimum_required(VERSION 2.4)' >> $(CMAKE_FILENAME)
	@echo '' >> $(CMAKE_FILENAME)
	@$(foreach pkg,$(CMAKE_REQUIRE_PACKAGE),echo 'find_package($(pkg) REQUIRED)' >> $(CMAKE_FILENAME);)
	@echo '' >> $(CMAKE_FILENAME)
	@echo 'set($(CMAKE_PROJECT)_SOURCES' >> $(CMAKE_FILENAME)
	@$(foreach src,$(LIBSRC),echo '  $(src)' >> $(CMAKE_FILENAME);)
	@echo ')' >> $(CMAKE_FILENAME)
	@echo '' >> $(CMAKE_FILENAME)
	@echo 'add_library($(CMAKE_PROJECT) $${$(CMAKE_PROJECT)_SOURCES})' >> $(CMAKE_FILENAME)
	@echo 'target_link_libraries($(CMAKE_PROJECT)' >> $(CMAKE_FILENAME)
	@$(foreach src,$(LINKER_LIBS),echo '  PUBLIC $(src)' >> $(CMAKE_FILENAME);)
	@echo '  )' >> $(CMAKE_FILENAME)
	@echo 'target_include_directories($(CMAKE_PROJECT)' >> $(CMAKE_FILENAME)
	@$(foreach src,$(LIBDIR_PUB),echo '  PUBLIC  $(src)' >> $(CMAKE_FILENAME);)
	@$(foreach src,$(LIBDIR_PRIV),echo '  PRIVATE $(src)' >> $(CMAKE_FILENAME);)
	@echo ')' >> $(CMAKE_FILENAME)
	@echo '' >> $(CMAKE_FILENAME)

endif

#############################################################################
# Tasks - make variable printout (test)
#############################################################################

# Prints the make variables, used for debugging the makefile
.PHONY : vars.%
vars.%:
	@echo "NAME: $(NAME)"
	@echo "CC: $(CC)"
	@echo "CXX: $(CXX)"
	@echo "BIN: $(BIN)"
	@echo "MAIN_ROOT: $(MAIN_ROOT)"
	@echo ""
	@echo "LIBDIR_PUB: $(LIBDIR_PUB)"
	@echo ""
	@echo "LIBDIR_PRIV: $(LIBDIR_PRIV)"
	@echo ""
	@echo "MAINDIR: $(MAINDIR)"
	@echo ""
	@echo "FOLDERS: $(FOLDERS)"
	@echo ""
	@echo "BUILDTREE: $(BUILDTREE)"
	@echo ""
	@echo "LIBSRC: $(LIBSRC)"
	@echo ""
	@echo "MAINSRC: $(MAINSRC)"
	@echo ""
	@echo "LIB_OBJS: $(LIB_OBJS)"
	@echo ""
	@echo "MAIN_OBJS: $(MAIN_OBJS)"
	@echo ""
	@echo "TEST_OBJS: $(TEST_OBJS)"
	@echo ""
	@echo "OBJS_DEPENDENCY: $(OBJS_DEPENDENCY)"
	@echo ""
	@echo "CFLAGS: $(CFLAGS)"
	@echo ""
	@echo "OPTIMIZATION: $(OPTIMIZATION)"
	@echo ""
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo ""
	@echo "LINKER_LIBS: $(LINKER_LIBS)"
	@echo ""
	@echo "LINKER_LIBS_EXT: $(LINKER_LIBS_EXT)"
	@echo ""
	@echo "LINKER_FLAGS: $(LINKER_FLAGS)"

.PHONY : vars
vars: vars.all ;

#############################################################################
# Tasks - facil.io specific: running examples
#############################################################################

.PHONY : ex/%
ex/%:
	@NAME=$* MAIN_ROOT="extras extras/http extras/parsers examples/$*/src" make run


# .PHONY : ex_prep.%
# ex_prep.%:
# 	$(eval MAIN_ROOT=extras extras/http extras/parsers examples/$*/src)

# .PHONY : ex/%
# ex/%: ex_prep.% build_start.% $(MAIN_OBJS) $(LIB_OBJS) build_finish.% run.% ;
