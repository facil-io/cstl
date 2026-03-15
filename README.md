# facil.io 0.8.x - The C Server Toolbox Library – C STL :)

[![facil.io logo](./extras/logo.svg?raw=true "facil.io")](https://github.com/facil-io/cstl)

[![POSIX C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml) [![MacOS C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/macos-c-cpp.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/macos-c-cpp.yml) [![Windows C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/windows.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/windows.yml) [![Windows Clang CI](https://github.com/facil-io/cstl/actions/workflows/windows_clang.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/windows_clang.yml)

> **The [facil.io](http://facil.io) C STL aims to provide C developers with easy-to-use tools to write memory safe and performant programs**.


## This C Server Toolbox Library Powers a microFramework

The [facil.io library](https://facil.io) is much more than a Web Application Framework and includes core tools and type templates that any C (and C++) project will find useful.

In addition to useful helpers, [facil.io](https://facil.io) allows developers to use MACROS to generate code for different common types, such as Hash Maps, Arrays, Binary-Safe Strings, etc'.

In other words, some of the most common building blocks one would need in any C project are placed in this convenient header file library.

### Installing

Simply copy the `fio-stl.h` file to your project's folder (using a single header file).  Done.

**Or**... copy the `fio-stl` folder to your project's folder (using `"fio-stl/include.h"`). Done.

Include the file as many times as required and enjoy.

### Running Tests

To test the STL locally you need to first fork the project or download the whole project source code. Then, from the project's root folder run:

```bash
make tests/stl
```

The GNU `make` command will compile and run any file in the `tests` folder if it is explicitly listed. i.e.,

```bash
make tests/malloc      # speed test facil.io's memory allocator
make tests/json        # test JSON parser
make tests/redis       # requires local redis/valkey - tests pub/sub & database access
```

It is possible to use the same `makefile` to compile source code and static library code. See the makefile for details.

On Windows you might want to skip the makefile (if you do not have `make` and `gcc` installed) and run:

```dos
cls && cl /Ox tests\stl.c /I. && stl.exe 
```

## Quick Examples

There are a number of examples in the [./examples](examples) folder, including:

### Examples for authoring network applications:

* [Mix an HTTP/1.1 Server to serve data with a WebSockets/SSE Chat Server](examples/server.c).
* [A simple network client example](examples/client.c).
* [Pub/Sub based TCP/IP text Chat server](examples/chat.c).

### Examples using the C Server Toolbox Library (STL) Types:

* [Creating your own Array type](examples/array.c).
* [Using the included copy-on-write Binary Safe Strings (`bstr`)](examples/bstr.c).
* [Making your own reference-counted shared (binary safe) String type](examples/string.c).
* [using FIOBJ types with JSON for soft / mix typed data structures](examples/fiobj.c).
* [Authoring your own Hash Maps for your own types](examples/map.c).

### Running the example code

Examples can be compiled and executed using:

```bash
make examples/<name>
```

For example, to compile and run the server example, use:

```bash
make examples/server
```

### CMake Build

```bash
# default config (tests and examples build OFF, static library)
cmake -B path/to/build_directory
# or add examples and test build
cmake -B path/to/build_directory -DTESTS_BUILD=ON -DEXAMPLES_BUILD=ON -DCMAKE_BUILD_TYPE=Release
# build
cmake --build path/to/build_directory
# install
cmake --install path/to/build_directory --prefix path/to/directory
```

Add this on your cmake project

```cmake
include(FetchContent)

find_package(facil-io 0.8.0)
if (NOT facil-io_FOUND)
    FetchContent_Declare(facil-io GIT_REPOSITORY https://github.com/facil-io/cstl.git
        GIT_TAG master)
    FetchContent_GetProperties(facil-io)
    FetchContent_MakeAvailable(facil-io)
endif()
    
target_link_libraries(${PROJECT_NAME} PRIVATE facil-io::facil-io)
```

## Contribution Notice

If you're submitting a PR, make sure to update the corresponding code slice (file) in the `fio-stl` folder, the `makefile` will re-produce the `fio-stl.h` file automatically.

Note that the master branch is currently as unstable as it gets. Commits may get squashed, the branch may be overwritten (force push), etc'. I will play nicer when the code stabilizes.

Also, contributions are subject to the terms and conditions set in [the facil.io contribution guide](CONTRIBUTING.md).

Logo contributed by ([Area55](https://github.com/area55git)) under a [Creative Commons Attribution 4.0 International License.](https://creativecommons.org/licenses/by/4.0/).

## Documentation

[Documentation is available in the (auto-generated) `fio-stl.md` file](fio-stl.md).

## Naming Conventions

- `snake_case`

- Namespace prefix: All public APIs use fio_ prefix (e.g., `fio_io_start`, `fio_subscribe`)

- Internal functions: Use `fio___` triple underscore for internal/private functions

- Type naming:
  - Structs: `<name>_s` suffix (e.g., `fio_io_s`, `fio_msg_s`)
  - Enums: `<name>_e` suffix
  - No `_t` suffix (reserved for POSIX)

- Function naming:
  - `<namespace>_<verb>` pattern (e.g., `fio_io_attach_fd`, `fio_pubsub_attach`)
  - Named arguments pattern: Variadic macros with struct initialization
      `#define fio_io_listen(...) fio_io_listen((fio_io_listen_args_s){__VA_ARGS__})`
  - Argument structs: `_args_s` suffix (e.g., `fio_subscribe_args_s`)
  - Property naming (getter / setter): `<namespace>_<property>(_set)` pattern (e.g., `fio_bstr_len`, `fio_bstr_len_set`)
     The setter functions SHOULD return the value that was set, unless it requires memory allocations or causes other performance issues (i.e., when the inner representation doesn't much the setter input).
  - Data retrieval: `<namespace>_<property>_get` - avoid - implies non-trivial data retrieval with possible side effects.
  - Use `object_new`/`object_free` for heap allocations and `object_init`/`object_destroy` for supporting stack allocations. the `object_destroy` should re-initialize memory.

- Function Parameters / Arguments:
  - If parameters have possible defaults, use named arguments pattern.
  - "target" or "destination" parameter should be first (i.e., when writing to a buffer).
  - If a `this` style parameter exists it MUST ALWAYS be first (then a target parameter, if exists).

##  Error Handling

- Return values: 
  - Pointers: `NULL` on error
  - Integers: `-1` on error, `0` on success
  - Boolean: `0` for `false`, non-zero for `true`


