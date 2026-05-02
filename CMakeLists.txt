cmake_minimum_required(VERSION 3.12...3.27)

# Fallback for using newer policies on CMake <3.17.
if(${CMAKE_VERSION} VERSION_LESS 3.17)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(facil-io VERSION 0.8.0
    DESCRIPTION "The facil-io library is much more than a Web Application Framework and includes core tools and type templates that any C (and C++) project will find useful."
    HOMEPAGE_URL "https://github.com/facil-io/cstl")

# Set the source directories
set(LIB_ROOT lib)
set(LIB_CONCAT_FOLDER fio-stl)
include_directories(".")

include(CheckLibraryExists)
include(CheckStructHasMember)
include(CheckTypeSize)
include(FeatureSummary)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)

find_library(MATH_LIBRARY m)

# default value of ${CMAKE_BUILD_TYPE} is Debug 
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose default type of build (Debug)" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CONFIGURATION_TYPES "Release") # for Windows
endif()

option(FACIL_ENABLE_OPENSSL "Build facil.io with OpenSSL" OFF)
option(FACIL_ENABLE_SODIUM "Build facil.io with Sodium" OFF)
option(FACIL_SHARED_BUILD "Build facil.io as shared library" ON)
option(FACIL_STATIC_BUILD "Build facil.io as static library" ON)
option(FACIL_EXAMPLES_BUILD "Build Examples" OFF)
option(FACIL_TESTS_BUILD "Build Tests" OFF)

# Detect endianess
check_type_size("void*" SIZE_OF_POINTER)
if(${SIZE_OF_POINTER} EQUAL 8)
    set(FLAGS "__BIG_ENDIAN__")
else()
    set(FLAGS "__BIG_ENDIAN__=0")
endif()

find_package(Threads REQUIRED)

if(FACIL_ENABLE_OPENSSL)
    find_package(OpenSSL REQUIRED)
endif()
if(FACIL_ENABLE_SODIUM)
    find_package(Sodium REQUIRED)
endif()

# Detect 'struct tm' fields
check_struct_has_member("struct tm" tm_zone time.h HAVE_TM_TM_ZONE)

# Detect SystemV socket libraries
check_library_exists(socket connect "" HAVE_SOCKET)
check_library_exists(nsl gethostname "" HAVE_NSL)
if(HAVE_SOCKET AND HAVE_NSL)
    link_libraries(socket nsl)
endif()

# Variable to track whether any of the checks have succeeded
set(HAVE_SENDFILE FALSE)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Detect the `sendfile` system call for Linux
    if(NOT HAVE_SENDFILE)
        check_c_source_compiles("
        #define _GNU_SOURCE
        #include <stdlib.h>
        #include <stdio.h>
        #include <sys/sendfile.h>
        int main(void) {
            off_t offset = 0;
            ssize_t result = sendfile(2, 1, (off_t *)&offset, 300);
        }
    "            HAVE_SENDFILE_LINUX)

        if(HAVE_SENDFILE_LINUX)
            set(FLAGS "${FLAGS} USE_SENDFILE_LINUX HAVE_SENDFILE")
            set(HAVE_SENDFILE TRUE)
        endif()
    endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # Detect the `sendfile` system call for Apple
    if(NOT HAVE_SENDFILE)
        check_c_source_compiles("
        #define _GNU_SOURCE
        #include <stdlib.h>
        #include <stdio.h>
        #include <sys/types.h>
        #include <sys/socket.h>
        #include <sys/uio.h>
        int main(void) {
            off_t sent = 0;
            off_t offset = 0;
            ssize_t result = sendfile(2, 1, offset, &sent, NULL, 0);
        }
    "            HAVE_SENDFILE_APPLE)

        if(HAVE_SENDFILE_APPLE)
            set(FLAGS "${FLAGS} USE_SENDFILE_APPLE HAVE_SENDFILE")
            set(HAVE_SENDFILE TRUE)
        endif()
    endif()
else()
    # Detect the `sendfile` system call for BSD
    if(NOT HAVE_SENDFILE)
        check_c_source_compiles("
        #define _GNU_SOURCE
        #include <stdlib.h>
        #include <stdio.h>
        #include <sys/types.h>
        #include <sys/socket.h>
        #include <sys/uio.h>
        int main(void) {
            off_t sent = 0;
            off_t offset = 0;
            ssize_t result = sendfile(2, 1, offset, (size_t)sent, NULL, &sent, 0);
        }
    "            HAVE_SENDFILE_BSD)

        if(HAVE_SENDFILE_BSD)
            set(FLAGS "${FLAGS} USE_SENDFILE_BSD HAVE_SENDFILE")
            set(HAVE_SENDFILE TRUE)
        endif()
    endif()
endif()

# Detect IOCP supp for Windows
if(WIN32)
    if(NOT HAVE_IOCP)
        check_c_source_compiles("
        #include <winsock2.h>
        #include <windows.h>
        int main(void) {
            HANDLE iocp = CreateIoCompletionP(INVALID_HANDLE_VALUE, NULL, 0, 0);
            if (iocp == NULL) {
                return 1;
            }
            CloseHandle(iocp);
            return 0;
        }
    "            HAVE_IOCP)

        if(HAVE_IOCP)
            set(FLAGS "${FLAGS} FIO_ENGINE_IOCP HAVE_IOCP")
        else()
            message(WARNING "No supped polling engine detected. Unable to compile facil.io.")
        endif()
    endif()
else()
    # Detect polling mechanisms (kqueue / epoll / poll)
    if(NOT HAVE_KQUEUE AND NOT HAVE_EPOLL AND NOT HAVE_POLL)
        check_c_source_compiles("
        #define _GNU_SOURCE
        #include <stdlib.h>
        #include <sys/event.h>
        int main(void) {
            int fd = kqueue();
        }
    "            HAVE_KQUEUE)

        if(HAVE_KQUEUE)
            set(FLAGS "${FLAGS} FIO_ENGINE_KQUEUE HAVE_KQUEUE")
        else()
            check_c_source_compiles("
            #define _GNU_SOURCE
            #include <stdlib.h>
            #include <stdio.h>
            #include <sys/types.h>
            #include <sys/stat.h>
            #include <fcntl.h>
            #include <sys/epoll.h>
            int main(void) {
                int fd = epoll_create1(EPOLL_CLOEXEC);
            }
        "                HAVE_EPOLL)

            if(HAVE_EPOLL)
                set(FLAGS "${FLAGS} FIO_ENGINE_EPOLL HAVE_EPOLL")
            else()
                check_c_source_compiles("
                #define _GNU_SOURCE
                #include <stdlib.h>
                #include <poll.h>
                int main(void) {
                    struct pollfd plist[18];
                    memset(plist, 0, sizeof(plist[0]) * 18);
                    poll(plist, 1, 1);
                }
            "                    HAVE_POLL)

                if(HAVE_POLL)
                    set(FLAGS "${FLAGS} FIO_ENGINE_POLL HAVE_POLL")
                else()
                    message(FATAL_ERROR "No supped polling engine detected. Unable to compile facil.io.")
                endif()
            endif()
        endif()
    endif()
endif()


# Define the source files for the library
file(GLOB_RECURSE LIB_SOURCES ${LIB_ROOT}/*.c ${LIB_ROOT}/*.cpp ${LIB_ROOT}/*.cxx ${LIB_ROOT}/*.c++)
set(SOURCES ${LIB_SOURCES})

# Define compiler flags and sanitizers
set(COMMON_FLAGS "")
set(RELEASE_FLAGS "")
set(DEBUG_FLAGS "")
# Add sanitizers flags for Debug builds
set(SANITIZERS "")

# facil-io defines
set(FLAGS "-DFIO_LEAK_COUNTER -DFIOBJ_MALLOC")

# Compiler-specific flags
if(MSVC)
    # MSVC compiler flags
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
    set(COMMON_FLAGS "/W4 /EHsc")
    set(RELEASE_FLAGS "${COMMON_FLAGS} /O2")
    set(DEBUG_FLAGS "${COMMON_FLAGS} /Od /Zi")
else()
    # GNU compiler flags (e.g., GCC or Clang)
    set(RELEASE_FLAGS "${COMMON_FLAGS} -O2")
    set(DEBUG_FLAGS "${COMMON_FLAGS} -O0 -g -DDEBUG=1")
endif()

if(MINGW)
    set(MINGW32_LIBRARY mingw32 "-mwindows" CACHE STRING "link flags for MinGW")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        # Add sanitizers for MSVC
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /fsanitize=address")
    else()
        # Add sanitizers for GNU compilers (GCC or Clang)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -fstack-protector-all -g -DMALLOC_CHECK_=3 -DTRACE_IS_ON=1 -O0 -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address -fsanitize=undefined")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_FLAGS}")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${RELEASE_FLAGS}")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    CHECK_CXX_COMPILER_FLAG("-stdlib=libc++" COMPILER_SUPPORTS_STATIC_CLANG)
    if(COMPILER_SUPPORTS_STATIC_CLANG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")

# Create the library target
if(FACIL_SHARED_BUILD)
    add_library(${PROJECT_NAME} SHARED ${SOURCES})
    if(OpenSSL_FOUND)
        if(NOT WIN32)
            target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads OpenSSL::SSL OpenSSL::Crypto PUBLIC ${MATH_LIBRARY})
        else()
            if(MSVC)
                target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads OpenSSL::SSL OpenSSL::Crypto ws2_32)
            else()
                target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ws2_32 Sodium::Sodium PUBLIC ${MINGW32_LIBRARY} ${MATH_LIBRARY})
            endif()
        endif()
    elseif(Sodium_FOUND)
        if(NOT WIN32)
            target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads Sodium::Sodium PUBLIC ${MATH_LIBRARY})
        else()
            if(MSVC)
                target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads Sodium::Sodium ws2_32)
            else()
                target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ws2_32 Sodium::Sodium PUBLIC ${MINGW32_LIBRARY} ${MATH_LIBRARY})
            endif()
        endif()
    else()
        if(NOT WIN32)
            target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads PUBLIC ${MATH_LIBRARY})
        else()
            if(MSVC)
                target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ws2_32)
            else()
                target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads PUBLIC ws2_32 ${MINGW32_LIBRARY} ${MATH_LIBRARY})
            endif()
        endif()
    endif()
    add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
endif()
if(FACIL_STATIC_BUILD)
    add_library(${PROJECT_NAME}-static STATIC ${SOURCES})
    add_library(${PROJECT_NAME}::${PROJECT_NAME}-static ALIAS ${PROJECT_NAME}-static)
endif()
target_include_directories(${PROJECT_NAME} PUBLIC ".")
target_compile_definitions(${PROJECT_NAME} PRIVATE ${FLAGS})


set(PKG_CONFIG_FILE "${CMAKE_BINARY_DIR}/facilio.pc")
configure_file(facilio.pc.in ${PKG_CONFIG_FILE} @ONLY)

# Install the generated .pc file
install(FILES ${PKG_CONFIG_FILE} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)

if(FACIL_EXAMPLES_BUILD)
    # Define a function to build and run an example
    function(build_and_run_example EXAMPLE_NAME)
        add_executable(${EXAMPLE_NAME} examples/${EXAMPLE_NAME}.c)
        target_link_libraries(${EXAMPLE_NAME} PUBLIC ${PROJECT_NAME})
        target_compile_definitions(${EXAMPLE_NAME} PRIVATE ${FLAGS})
        add_custom_target(run_${EXAMPLE_NAME}
            COMMAND ${EXAMPLE_NAME}
            DEPENDS ${EXAMPLE_NAME}
        )
    endfunction()

    # List of example files to build and run
    # comment all examples w/ issue build
    set(EXAMPLES
        array
        bstr
        chat
        client
        fiobj
        map
        server
        string
    )

    # Build and run each example
    foreach(EXAMPLE ${EXAMPLES})
        build_and_run_example(${EXAMPLE})
    endforeach()
endif()

if(FACIL_TESTS_BUILD)
    # Define a function to build and run a test
    function(build_and_run_test TEST_NAME)
        if(${TEST_NAME} STREQUAL stl-mutex AND NOT OpenSSL_FOUND)
            message(WARNING "Skipping build of ${TEST_NAME} without OpenSSL")
            return()
        endif()
        if(${TEST_NAME} STREQUAL cpp)
            add_executable(${TEST_NAME} tests/${TEST_NAME}.cpp)
        else()
            add_executable(${TEST_NAME} tests/${TEST_NAME}.c)
        endif()
        target_link_libraries(${TEST_NAME} PUBLIC ${PROJECT_NAME})
        target_compile_definitions(${TEST_NAME} PRIVATE ${FLAGS} "-DTESTS=1")
        add_custom_target(run_${TEST_NAME}
            COMMAND ${TEST_NAME}
            DEPENDS ${TEST_NAME}
        )
    endfunction()

    # List of test files to build and run
    # comment all tests w/ issue build
    set(TESTS
        # array
        base64
        # cpp
        http1-parser-old
        http1-parser
        json
        json_find
        json_minify
        malloc
        mempool
        mustache
        noop
        random
        slowloris
        stl-mutex
        stl
        url
    )

    # Build and run each test
    foreach(TEST ${TESTS})
        build_and_run_test(${TEST})
    endforeach()
endif()
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
