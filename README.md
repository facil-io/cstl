# facil.io - C STL - a Simple Template Library for C

At the core of the [facil.io library](https://facil.io) is its powerful Simple Template Library for C (and C++).

The Simple Template Library is a "swiss-army-knife" library, that uses MACROS to generate code for different common types, such as Hash Maps, Arrays, Linked Lists, Binary-Safe Strings, etc'.

In addition, the Simple Template Library offers common functional primitives and helpers, such as bit operations, atomic operations, CLI parsing, JSON, task queues, and a custom memory allocator.

In other words, all the common building blocks one could need in a C project are placed in this single header file.

The header could be included multiple times with different results, creating different types or exposing different functionality.

### Running Tests

Testing the STL locally is easy using:

```bash
make test/stl
```

The GNU `make` command will compile and run any file in the `tests` folder if it is explicitly listed. i.e.,

```bash
make test/malloc      # speed test facil.io's memory allocator
make test/json        # test JSON roundtrip with external JSON files
make test/json_minify # JSON minification example
make test/cpp         # Test template compilation in a C++ file (no run)... may fail on some compilers
```

It is possible to use the same `makefile` to compile source code and static library code. See the makefile for details.

### Contribution Notice

If you're submitting a PR, make sure to update the corresponding code slice (file) in the `stl_slices` folder.

Also, contributions are subject to the terms and conditions set in [the facil.io contribution guide](https://github.com/boazsegev/facil.io/CONTRIBUTING.md). 

## Documentation

[Documentation is available in the (auto-generated) `fio-stl.md` file](fio-stl.md).
