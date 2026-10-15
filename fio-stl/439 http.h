/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

                  HTTP Implementation for FIO_SERVER

This file is the HTTP module's cleanup unit. Its tail is the ONLY
`#undef FIO_HTTP` site in the module: during recursive (template) inclusion it
evicts stale preprocessor state so the HTTP module content is skipped exactly
once per template instantiation pass.

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if !defined(FIO___RECURSIVE_INCLUDE) && defined(FIO_HTTP)
#undef FIO___HTTP_STATE_STREAMING
#undef FIO___HTTP_STATE_FINISHED
#undef FIO___HTTP_STATE_UPGRADED
#undef FIO___HTTP_STATE_WEBSOCKET
#undef FIO___HTTP_STATE_SSE
#undef FIO___HTTP_STATE_COOKIES_PARSED
#undef FIO___HTTP_STATE_FREEING
#undef FIO___HTTP_STATE_NO_CONTENT
#undef FIO___HTTP_CFLAG_COMPRESS_DYNAMIC
#undef FIO___HTTP_CFLAG_COMPRESS_WS
#undef FIO___HTTP_CFLAG_COMPRESS_STATIC
#undef FIO_HTTP
#endif
