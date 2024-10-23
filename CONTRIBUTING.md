# How to Contribute

Thank you for inquiring `facil.io`'s contribution guide. It's people like you and me, that are willing to share our efforts, who help make the world of open source development so inspiring and wonderful.

## TL;DR;

* Play nice.

* Contributions and edits to the code should be placed in the `/fio-stl` folder and its many header files which are numbered by priority and classification (**NOT** in the auto-generated `fio-stl.h` header).

* PRs are appreciated. Most likely the code you're looking for is @ the [facil.io C STL repository](https://github.com/facil-io/cstl).

* Always add a comment in the CHANGELOG to say what you did and credit yourself.

* All copyrights for whatever you contribute will be owned by myself (Boaz Segev) and between us we'll consider them public domain (I don't want to deal legal stuff, moral rights and all that).

* Use `clang-format` if you can.

* Use `snake_case`. Return `-1` on failure and `0` on success (kernel style API).

## Guidelines 

### General Guidelines

"Facil" comes from the Spanish word "easy", and this is embedded in `facil.io`'s DNA.

`facil.io` contributions should (ideally) be:

* **Easy to use**:

    Clear and concise API, with macros that emulate "named arguments" when appropriate.

* **Easy to maintain**:

    * *Modular*: even at the price of performance and even (although less desired) at the price of keeping things DRY.

        Developers should be able to simply remove the module from their implementation if they're not using it... though this is obviously not the case for many core modules (i.e., `FIO_STR`).

        To clarify, a module should have as small a responsibility as possible, use only public API when interacting with other modules and minimize the use of other modules. This makes the module easier to maintain and minimizes code fragility and code entanglement.

    * *Succinctly Commented*: Too much commenting is noise (we can read code), but too little and a future maintainer might not understand why the code was written in the first place.

    * *Documented*: document functions using Doxygen style comments, but without Doxygen keywords (assume human readers). 

* **Easy to port**:

    When possible, code should be portable. This is both true in regards to CPU architecture and in regards to OS and environment.

    The project currently has the following limitation that might be addressed in the future:

    * The code requires `kqueue` or `epoll` for socket polling services, which means Linux / BSD / macOS. The code also supports `poll` as a fallback for Windows and the rest of the POSIX family... but this might not work for higher network loads.

    * The code assumes a Unix environment (file naming etc').

    * Some of the code (namely some HTTP parts) uses unaligned memory access (requiring newer CPUs and possibly introducing undefined behavior).

* **Easy to compile**:

    The code uses GNU `make` and although I tried to provide CMake support, neither CMake nor `configure` should be *required* at any point. In fact. `make` should be considered optional.

* **Easy to manage**:

    See the License section below. Contributions must relinquish ownership of contributed code, so licensing and copyright can be managed without the need to reach out to every past contributor.

### File Naming and Edits

The `fio-stl.h` header is **Auto-Generated** - do **not** edit it. 

Any new features or changes should be made in the individual headers placed in the `./fio-stl` **folder**. This avoids manual edits and ensures consistency during the generation process.

#### Boilerplate for New Modules

There's a template or boilerplate for new modules provided the [`./fio-stl/699 empty module.h`](https://github.com/facil-io/cstl/blob/master/fio-stl/699%20empty%20module.h), which should be used as the starting point for adding any new functionality. This includes the necessary structure and naming conventions for creating new modules within the CSTL.

#### Numbering Scheme for Header Files

The numbering at the beginning of the header filenames serves a dual purpose:

* Prioritization in the generated fio-stl.h file (e.g., headers that require dynamic memory allocation begin from 100).

* Feature classification (e.g., core types that require memory allocation are in the 100-199 range while new type templates are placed in the 200-299 range). This numbering ensures that modules are added in the correct sequence and can manage their dependencies efficiently.


These are the current numbering ranges:

* 000-099: core features, ordered by priority and dependency. This may also include non-core helpers and parsers that require no memory allocation.

* 100-198: non-template / core types and building blocks. This may also include parsers that require memory allocations.

* 200-298: template types – types that can be customized using macros, such as hash maps and dynamic arrays.

* 300-398: cryptography and cryptographic tools - these should be considered as fallback elements when a cryptography library is missing.

* 400-498: server, web and IO, such as HTTP, WebSockets, communication protocols, etc'.

* 500: FIOBJ soft types.

* 501-598: to be decided.

* 600-698: to be decided.

* 700: cleanup.

* 900-998: tests.

Please remember to add your module to both the `include.h` and `000 dependencies.h` header files, so people can access it.

Please write tests to test your module. See the `902 empty module tests.h` boilerplate.

### Community Guideline - Play Nice

As a child, I wasn't any good with people (I'm not sure I'm any better now that I'm older)... which is how come I became good with computers and why we have `facil.io` and other open source projects ;-)

However, I promise to do my best to be a respectful communicator and I ask of you to do your best as well.

No matter if discussing a PR (where we might find ourselves entering a heated discussion) or answering an issue (where sometime we find ourselves wondering why people think we work for them)... we should all remember that a little compassion and respect goes a long way.

### Style Guide and Guidelines

A few pointers about code styling (pun intended).

* Use `clang-format` with the provided `.clang-format` style file. It's not always the best, but it will offer uniformity.

    The style is based on the `LLVM` style with a few adjustments, so have a look at our `.clang-format` file.

* Initialize all variables during declaration (or make best attempt to do so).

* Use `snake_case` with `readable_names` and avoid CamelCase or VeryLongAndOverlyVerboseNames.

* Use the `fio___` prefix for internal helper functions (note the 3 underscores).

* Prefer verbose readable code. Optimize later (but please optimize).

* Common practice abbreviations, context-specific abbreviations (when in context) and auto-complete optimizations are preferred **only when readability isn't significantly affected**.

* Function names **should** be as succinct as possible.

* Use `goto` to move less-likely code branches to the end of a function's body (specifically, error branches should go to a `goto` label).

    It makes the main body of the function more readable (IMHO) and could help with branch prediction (similar to how `unlikely` might help, but using a different approach).

* Use `goto` before returning from a function when a spinlock / mutex unlock is required (specifically, repetition of the unlock code should be avoided).

## License

The project requires that all the code is licensed under a choose-your-license scheme offering both ISC and MIT licensing options (though that may change).

Please refrain from using or offering code that requires a change to the licensing scheme or that might prevent future updates to the licensing scheme.

I discovered GitHub doesn't offer a default CLA (Copyright and Licensing Agreement), so I adopted the one I once read when reviewing the [BearSSL](https://www.bearssl.org/contrib.html) library, meaning:

* the resulting code uses the ISC/MIT license, listing me (and only me) as the author. You can take credit by stating that the code was written by yourself, but should attribute copyright and authorship to me (Boaz Segev). This is similar to a "work for hire" approach so all copyrights and moral rights will be collated in one place.

* I usually list meaningful contributions in the CHANGELOG. When adding a PR, feel free to credit yourselves in the CHANGELOG.

This is meant to lift the burden of dealing with legal rights on my part. This allows me to circumvent any future licensing concerns and prevent contributors from revoking the license attached to their code.

### Where to start / Roadmap

Before you start working on a feature, please consider opening a PR to edit this CONTRIBUTING file and letting the community know that you took this feature upon yourself.

Add the feature you want to work on to the following list (or assign an existing feature to yourself). This will also allow us to discuss, in the PR's thread, any questions you might have or any expectations that might effect the API or the feature.

Once you have all the information you need to implementing the feature, the discussion can move to the actual feature's PR.

These are the features that have been requested so far. Even if any of them are assigned, feel free to offer your help:

|      Feature      |      assigned      |      remarks                                        |
|-------------------|--------------------|-----------------------------------------------------|
|   Documentation   |   Help, Please     |
|-------------------|--------------------|-----------------------------------------------------|
|   Security        |                    |  Some more security features would be nice.
|-------------------|--------------------|-----------------------------------------------------|
|   WebSockets/SSE  |                    |  Rewrite.
|-------------------|--------------------|-----------------------------------------------------|
|       Redis       |                    |  Rewrite.
|-------------------|--------------------|-----------------------------------------------------|
|       ED25519     |                    |  
|-------------------|--------------------|-----------------------------------------------------|
| Pub/Sub Encryption|                    |  Yes, pub/sub will be going public...
|-------------------|--------------------|-----------------------------------------------------|
| FIO_PTR_TAG_VALID |                    |  implement pointer tag validation for all types.
|-------------------|--------------------|-----------------------------------------------------|

## Notable Contributions


### pre-0.8.x

* @area55git ([Area55](https://github.com/area55git)) contributed the logo under a [Creative Commons Attribution 4.0 International License.](https://creativecommons.org/licenses/by/4.0/).

* @cdkrot took the time to test some of the demo code using valgrind, detecting a shutdown issue with in core `defer` library and offering a quick fix.

* @madsheep and @nilclass took the time to expose a very quite issue (#16) that involved a long processing `on_open` websocket callback and very short network roundtrips, exposing a weakness in the HTTP/1.x logic.

* @64 took the time to test the pre-released 0.6.0 version and submit [PR #25](https://github.com/boazsegev/facil.io/pull/25), fixing a silent error and some warnings.

* Florian Weber (@Florianjw) took time to challenge the RiskyHash draft and [exposed a byte ordering error (last 7 byte reading order)](https://www.reddit.com/r/crypto/comments/9kk5gl/break_my_ciphercollectionpost/eekxw2f/?context=3).

* Chris Anderson (@injinj) did amazing work exploring a 128 bit variation and attacking RiskyHash using a variation on a Meet-In-The-Middle attack, written by Hening Makholm (@hmakholm) on his ([SMHasher fork](https://github.com/hmakholm/smhasher)). The RiskyHash dfraft was updated to address this attack.

