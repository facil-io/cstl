# Version and Settings

The knobs that control how facil.io identifies itself and behaves at compile time.

For core types and everyday helpers, see [`./000 core.md`](./000%20core.md). This page covers version macros and core behavioral defaults; for per-inclusion hooks such as allocator routing and pointer tagging, see [`./001 header.h`](./001%20header.h).

## Version Macros

facil.io follows [semantic versioning](https://semver.org). These macros are defined in [`./000 core.h`](./000%20core.h) and can be used to detect the library version at compile time.

- **`FIO_VERSION_MAJOR`** — Major version. API/ABI breaking changes bump this.  
  Default: `0`
- **`FIO_VERSION_MINOR`** — Minor version. Significant features or deprecations. May break ABI.  
  Default: `8`
- **`FIO_VERSION_PATCH`** — Patch version. Bug fixes and small additions.  
  Default: `0`
- **`FIO_VERSION_BUILD`** — Optional build metadata, such as `"rc.02"`.  
  Default: `"rc.02"`
- **`FIO_VERSION_STRING`** — Full version as a string literal, built from the macros above. The guard is `#ifdef FIO_VERSION_BUILD`, so the build metadata is included whenever `FIO_VERSION_BUILD` is defined, including when defined as an empty string: `"0.8.0-rc.02"`. If `FIO_VERSION_BUILD` is undefined, the string reads `"0.8.0"`.  
  Because [`./000 core.h`](./000%20core.h) unconditionally `#define`s `FIO_VERSION_BUILD "rc.02"` before the `#ifdef` guard, omitting the suffix requires `#undef FIO_VERSION_BUILD` before inclusion, not merely leaving it undefined.

Override these only when you are vendoring a fork or need to report a custom build string.

## Compile-Time Behavioral Defaults

Most defaults are guarded with `#ifndef`; define them before including the header to override the default. `FIO_NO_LOG` and `DEBUG` are presence flags rather than `#ifndef` defaults.

### Memory and Safety

- **`FIO_LEAK_COUNTER`** — Enables memory-leak counting. When enabled, the leak counter tracks allocations made through facil.io allocators and prints a summary at exit.  
  Default: `1` (enabled).  
  Set to `0` to disable counting and the exit report.  
  Note: this conflicts with `FIO_NO_LOG`, because the leak report uses the logging layer.
- **`FIO_LEAK_COUNTER_SKIP_EXIT`** — Skip the automatic leak report at exit when leak counting is enabled.  
  Default: `0` (report at exit).
- **`FIO_LEAK_COUNTER_DEF(name)`** — Declares a named leak counter.
- **`FIO_LEAK_COUNTER_ON_ALLOC(name)`** — Records an allocation against the named counter.
- **`FIO_LEAK_COUNTER_ON_FREE(name)`** — Records a free against the named counter.
- **`FIO_LEAK_COUNTER_COUNT(name)`** — Returns the current value of the named counter.
- **`FIO_MEMORY_INITIALIZE_ALLOCATIONS_DEFAULT`** — When non-zero, facil.io allocators zero-initialize returned memory by default.  
  Default: `1` (secure by default).  
  Set to `0` if you want raw allocations and prefer to do your own initialization.
- **`FIO_MEM_PAGE_SIZE_LOG`** — Log₂ of the OS memory page size. The memory allocator uses this to round allocations and map memory efficiently.  
  Default: `12` (4096-byte pages).  
  Change it if you are targeting a system with a different page size.
- **`FIO_STATIC_ALLOC_SAFE_CONCURRENCY_MAX`** — Maximum safe concurrent calls for static allocators created with `FIO_STATIC_ALLOC_DEF`.  
  Default: `256`.  
  Raise it if you use many static allocator calls or threads and are seeing reused buffers.

### Concurrency

- **`FIO_USE_THREAD_MUTEX`** — Choose the locking primitive used by thread-safe modules.  
  Default: `0` (use facil.io spinlocks).  
  Set to `1` to use OS native mutexes (`pthread_mutex_t` on POSIX) instead.  
  If you need this decision to apply to only a single `#include`, use `FIO_USE_THREAD_MUTEX_TMP` in [`./001 header.h`](./001%20header.h) instead.

### Performance and Alignment

- **`FIO_UNALIGNED_ACCESS`** — Allows facil.io to attempt unaligned memory access on CPUs that support it. When enabled, the library may use faster paths in helpers such as `fio_buf2uXX`.  
  Default: `1` (attempt detection).  
  Set to `0` on strict-alignment architectures or if you want to forbid unaligned access entirely. The actual capability is reflected by `FIO_UNALIGNED_MEMORY_ACCESS_ENABLED`, which is derived at compile time.
- **`FIO_LIMIT_INTRINSIC_BUFFER`** — Limits register pressure in some pseudo-intrinsic loops by processing smaller batches and looping more.  
  Default: `1`.  
  Set to `0` if you prefer larger unrolled buffers and do not mind the extra register use.

### Maps

- **`FIO_MAP_WARNING_BITSIZE`** — Bit size at which hash-map allocations trigger an internal warning threshold in the `imap`/`map` templates.  
  Default: `24`.  
  Raise or lower it to tune when the library starts worrying about map size during growth.

### Logging

- **`FIO_NO_LOG`** — Define this to remove all logging output. Cannot be combined with `FIO_LEAK_COUNTER`; the two are mutually exclusive.
- **`FIO_LOG2STDERR`** — Low-level sink used by the logging macros.  
  Default: no-op.  
  Override to print to `stderr` or route messages to a custom destination.
- **`FIO_LOG_LENGTH_LIMIT`** — Approximate cap on a single formatted log message, limiting stack memory use.  
  Default: `1024`.
- **`DEBUG`** — Define this to enable debug-only log macros (`FIO_LOG_D*`) and active `FIO_ASSERT_DEBUG` checks. Also disables SIMD/Neon intrinsics in [`./000 core.h`](./000%20core.h).

### Memory allocator routing

These macros are defined in [`./001 header.h`](./001%20header.h) and are re-evaluated on every inclusion cycle. Define them before the first fio-stl include to replace memory routing globally.

- **`FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)`** — Reallocate a block, copying at least `copy_len` bytes if a move is required. Defaults to the system `realloc` unless the facil.io allocator is available, in which case it routes to `fio_realloc2`. Override to plug in a custom allocator.
- **`FIO_MEM_FREE(ptr, size)`** — Free a block. Defaults to the system `free` or `fio_free`. Override alongside `FIO_MEM_REALLOC`.
- **`FIO_MEMORY_DISABLE`** — Define to disable all custom facil.io allocators and force system `malloc`/`free` for temporary allocations.
- **`FIO_MALLOC_TMP_USE_SYSTEM`** — Force the recursive allocator variant to use the system `realloc`/`free`. Useful when a custom `FIO_MEM_REALLOC` needs an allocation path that will not recurse back into facil.io.
- **`FIO_MEM_REALLOC_` / `FIO_MEM_FREE_`** — Recursive-safe copies of `FIO_MEM_REALLOC`/`FIO_MEM_FREE`. Override these when the custom allocator itself needs to allocate or free memory without re-entering the override.

### Pointer tagging

These macros are defined in [`./001 header.h`](./001%20header.h) and let included types store a small marker in the unused low bits of aligned pointers.

- **`FIO_PTR_TAG(p)`** — Apply the tag to a pointer. Default is the identity function.
- **`FIO_PTR_UNTAG(p)`** — Remove the tag from a pointer. Default is the identity function.
- **`FIO_PTR_TAG_TYPE`** — If defined, functions returning a type's pointer return this type instead.
- **`FIO_PTR_TAG_VALIDATE(ptr)`** — Returns a true value if the tagged pointer is valid. Default checks for non-`NULL`.
- **`FIO_PTR_TAG_VALID_OR_RETURN(tagged_ptr, value)`** — Validates the tagged pointer and returns `value` if it fails validation.
- **`FIO_PTR_TAG_VALID_OR_RETURN_VOID(tagged_ptr)`** — Validates the tagged pointer and returns `void` if it fails validation.
- **`FIO_PTR_TAG_VALID_OR_GOTO(tagged_ptr, label)`** — Validates the tagged pointer and jumps to `label` if it fails validation.
- **`FIO_PTR_TAG_GET_UNTAGGED(untagged_type, tagged_ptr)`** — Cast the untagged pointer to `untagged_type *`.

Pointer tagging is used by dynamic-type wrappers (for example, FIOBJ) to distinguish object kinds without a separate type field.

## See Also

- [`./000 core.md`](./000%20core.md) — documentation for core types and helpers.
- [`./000 core.h`](./000%20core.h) — the source of truth for version macros and the defaults above.
- [`./001 header.h`](./001%20header.h) — per-inclusion overrides such as `FIO_USE_THREAD_MUTEX_TMP` and allocator routing macros.
