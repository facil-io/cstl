## Logging and Assertions:

If the `FIO_LOG_LENGTH_LIMIT` macro is defined (it's recommended that it be greater than 128), than the `FIO_LOG2STDERR` (weak) function and the `FIO_LOG2STDERR2` macro will be defined.

#### `FIO_LOG_LEVEL`

An application wide integer with a value of either:

- `FIO_LOG_LEVEL_NONE` (0)
- `FIO_LOG_LEVEL_FATAL` (1)
- `FIO_LOG_LEVEL_ERROR` (2)
- `FIO_LOG_LEVEL_WARNING` (3)
- `FIO_LOG_LEVEL_INFO` (4)
- `FIO_LOG_LEVEL_DEBUG` (5)

The initial value can be set using the `FIO_LOG_LEVEL_DEFAULT` macro. By default, the level is 4 (`FIO_LOG_LEVEL_INFO`) for normal compilation and 5 (`FIO_LOG_LEVEL_DEBUG`) for DEBUG compilation.

#### `FIO_LOG2STDERR(msg, ...)`

This `printf` style **function** will log a message to `stderr`, without allocating any memory on the heap for the string (`fprintf` might).

The function is defined as `weak`, allowing it to be overridden during the linking stage, so logging could be diverted... although, it's recommended to divert `stderr` rather then the logging function.

#### `FIO_LOG2STDERR2(msg, ...)`

This macro routs to the `FIO_LOG2STDERR` function after prefixing the message with the file name and line number in which the error occurred.

#### `FIO_LOG_DEBUG(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_DEBUG`.

#### `FIO_LOG_INFO(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_INFO`.

#### `FIO_LOG_WARNING(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_WARNING`.

#### `FIO_LOG_ERROR(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_ERROR`.

#### `FIO_LOG_SECURITY(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_ERROR`.

#### `FIO_LOG_FATAL(msg, ...)`

Logs `msg` **if** log level is equal or above requested log level of `FIO_LOG_LEVEL_FATAL`.

#### `FIO_ASSERT(cond, msg, ...)`

Reports an error unless condition is met, printing out `msg` using `FIO_LOG_FATAL` and exiting (not aborting) the application.

In addition, a `SIGINT` will be sent to the process and any of it's children before exiting the application, supporting debuggers everywhere :-)

#### `FIO_ASSERT_ALLOC(cond, msg, ...)`

Reports an error unless condition is met, printing out `msg` using `FIO_LOG_FATAL` and exiting (not aborting) the application.

In addition, a `SIGINT` will be sent to the process and any of it's children before exiting the application, supporting debuggers everywhere :-)

#### `FIO_ASSERT_DEBUG(cond, msg, ...)`

Ignored unless `DEBUG` is defined.

Reports an error unless condition is met, printing out `msg` using `FIO_LOG_FATAL` and aborting (not exiting) the application.

In addition, a `SIGINT` will be sent to the process and any of it's children before aborting the application, because consistency is important.

**Note**: `msg` MUST be a string literal.

-------------------------------------------------------------------------------
