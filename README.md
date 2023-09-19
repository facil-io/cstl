# facil.io 0.8.x for C - now with an integrated C STL

## The Web microFramework and Server Toolbox library for C

[![POSIX C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/c-cpp.yml) [![Windows C/C++ CI](https://github.com/facil-io/cstl/actions/workflows/windows.yml/badge.svg)](https://github.com/facil-io/cstl/actions/workflows/windows.yml)

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
make tests/json        # test JSON roundtrip with external JSON files
make tests/json_minify # JSON minification example
make tests/cpp         # Test template compilation in a C++ file (no run)... may fail on some compilers
```

It is possible to use the same `makefile` to compile source code and static library code. See the makefile for details.

On Windows you might want to skip the makefile (if you do not have `make` and `gcc` installed) and run:

```dos
cls && cl /Ox tests\stl.c /I. && stl.exe 
```

## Quick Examples

There are a number of examples in the [./examples](examples) folder, including:

### Examples for authoring network applications:

* [An HTTP Echo and WebSockets/SSE Chat Server with static file service](examples/server.c).
* [A simple network client example](examples/client.c).
* [Pub/Sub based text Chat server](examples/chat.c).

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

## Contribution Notice

If you're submitting a PR, make sure to update the corresponding code slice (file) in the `fio-stl` folder, the `makefile` will re-produce the `fio-stl.h` file automatically.

Note that the master branch is currently as unstable as it gets. Commits may get squashed, the branch may be overwritten (force push), etc'. I will play nicer when the code stabilizes.

Also, contributions are subject to the terms and conditions set in [the facil.io contribution guide](CONTRIBUTING.md). 

## Documentation

[Documentation is available in the (auto-generated) `fio-stl.md` file](fio-stl.md).
