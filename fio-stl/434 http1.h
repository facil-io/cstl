/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_HTTP               /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************

              HTTP/1.1 - Request/Response Glue, Protocol and Controller

Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_HTTP) && !defined(FIO___RECURSIVE_INCLUDE) &&                  \
    !defined(H___FIO_HTTP1___H) &&                                             \
    (defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN))
#define H___FIO_HTTP1___H

/* *****************************************************************************
HTTP/1.1 Request / Response Completed
***************************************************************************** */

/** called when either a request or a response was received. */
static void fio_http1_on_complete(void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_io_dup(c->io); /* make sure the IO and its data are valid in callback */
  fio_io_suspend(c->io);
  fio_http_s *h = c->h;
  c->h = NULL;
  c->state.http.header_bytes = 0;
  c->suspend = 1;
  // fio_io_defer(c->state.http.on_http_callback, h, NULL);
  fio_queue_push(fio_io_queue(), c->state.http.on_http_callback, h);
}

/* *****************************************************************************
HTTP/1.1 Parser callbacks
***************************************************************************** */

FIO_IFUNC void fio___http_request_too_big(fio___http_connection_s *c) {
  fio_http_s *h = c->h;
  fio_io_dup(c->io); /* sending the response will result in fio_undup */
  fio_io_suspend(c->io);
  c->h = NULL;
  c->suspend = 1;
  if (fio_http_send_error_response(h, 413))
    fio_io_free(c->io); /* response not sent, we need to fio_undup */
  fio_http_free(h);
}

FIO_IFUNC void fio_http1_attach_handle(fio___http_connection_s *c) {
  c->h = fio_http_new();
  FIO_ASSERT_ALLOC(c->h);
  fio_http_controller_set(
      c->h,
      &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings))
           ->state[FIO___HTTP_PROTOCOL_HTTP1]
           .controller);
  fio_http_udata_set(c->h, c->udata);
  fio_http_cdata_set(c->h, fio___http_connection_dup(c));
  if (c->settings->compress_dynamic)
    fio_http_cflags_set(c->h, FIO_HTTP_CFLAG_COMPRESS_DYNAMIC);
  if (c->settings->compress_ws)
    fio_http_cflags_set(c->h, FIO_HTTP_CFLAG_COMPRESS_WS);
}

/** called when a request method is parsed. */
static int fio_http1_on_method(fio_buf_info_s method, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (c->h)
    return -1;
  fio_http1_attach_handle(c);
  fio_http_method_set(c->h, FIO_BUF2STR_INFO(method));
  return 0;
}
/** called when a response status is parsed. the status_str is the string
 * without the prefixed numerical status indicator.*/
static int fio_http1_on_status(size_t istatus,
                               fio_buf_info_s status,
                               void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_http_clear_response(c->h, istatus != 301 && istatus != 302);
  fio_http_status_set(c->h, istatus);
  return 0;
  (void)status;
}
/** called when a request URL is parsed. */
static int fio_http1_on_url(fio_buf_info_s url, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_url_s u = fio_url_parse(url.buf, url.len);
  if (!u.path.len || u.path.buf[0] != '/')
    return -1;
  fio_http_path_set(c->h, FIO_BUF2STR_INFO(u.path));
  fio_http_opath_set(c->h, FIO_BUF2STR_INFO(u.path));
  if (u.query.len)
    fio_http_query_set(c->h, FIO_BUF2STR_INFO(u.query));
  if (u.host.len)
    (!(c->h) ? fio_http_request_header_set
             : fio_http_response_header_set)(c->h,
                                             FIO_STR_INFO1((char *)"host"),
                                             FIO_BUF2STR_INFO(u.host));
  return 0;
}
/** called when a the HTTP/1.x version is parsed. */
static int fio_http1_on_version(fio_buf_info_s version, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  FIO_ASSERT_DEBUG(c->h, "on_version called without a pre-existing handle!");
  if (!c->h)
    return -1;
  fio_http_version_set(c->h, FIO_BUF2STR_INFO(version));
  return 0;
}
/** called when a header is parsed. */
static int fio_http1_on_header(fio_buf_info_s name,
                               fio_buf_info_s value,
                               void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h)
    return 0; /* ignore possible post-error response headers */
  const size_t line_len = value.len + name.len;
  c->state.http.header_bytes += line_len;
  if ((unsigned)(c->state.http.header_bytes > c->state.http.max_header) |
      (line_len > c->state.http.max_line))
    goto headers_too_big;
  (!fio_http_status(c->h)
       ? fio_http_request_header_add
       : fio_http_response_header_add)(c->h,
                                       FIO_BUF2STR_INFO(name),
                                       FIO_BUF2STR_INFO(value));
  return 0;
headers_too_big:
  fio_http_send_error_response(c->h, 431);
  return -1;
}
/** called when the special content-length header is parsed. */
static int fio_http1_on_header_content_length(fio_buf_info_s name,
                                              fio_buf_info_s value,
                                              size_t content_length,
                                              void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  fio_http_s *h = c->h;
  if (!h)
    return 0;
  if (content_length > c->settings->max_body_size)
    goto too_big;
  if (content_length)
    fio_http_body_expect(c->h, content_length);
#if FIO_HTTP_SHOW_CONTENT_LENGTH_HEADER
  (!(h->status) ? fio_http_request_header_add
                : fio_http_response_header_add)(h,
                                                FIO_BUF2STR_INFO(name),
                                                FIO_BUF2STR_INFO(value));
#endif
  return 0;
too_big:
  fio___http_request_too_big(c);
  return 0; /* should we disconnect (return -1), or not? */
  (void)name, (void)value;
}
/** called when `Expect` arrives and may require a 100 continue response. */
static int fio_http1_on_expect(void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  const fio_buf_info_s response =
      FIO_BUF_INFO1((char *)"HTTP/1.1 100 Continue\r\n\r\n");
  fio_http_s *h = c->h;
  if (!h)
    return 1;
  c->h = NULL;
  /* test for body size violation and deny request if payload too big. */
  if (FIO_HTTP1_EXPECTED_CHUNKED != fio_http1_expected(&c->state.http.parser) &&
      c->settings->max_body_size < fio_http1_expected(&c->state.http.parser))
    goto payload_too_big;
  c->settings->pre_http_body(h);
  if (fio_http_status(h))
    goto response_sent;
  c->h = h;
  fio_io_write2(c->io, .buf = response.buf, .len = response.len, .copy = 0);
  return 0; /* TODO?: improve support for `expect` headers? */
payload_too_big:
  fio_io_dup(c->io);
  if (fio_http_send_error_response(h, 413))
    fio_io_free(c->io); /* response not sent, we need to fio_undup */
                        /* fall through */
response_sent:
  // c->h = NULL;
  fio_http_free(h);
  return 1;
}

/** called when a body chunk is parsed. */
static int fio_http1_on_body_chunk(fio_buf_info_s chunk, void *udata) {
  fio___http_connection_s *c = (fio___http_connection_s *)udata;
  if (!c->h)
    return -1; /* close connection if a large payload is unstoppable */
  if (c->is_client &&
      (fio_http_status(c->h) == 301 || fio_http_status(c->h) == 302))
    return 0; /* don't overwrite client payload on redirect */
  if (chunk.len + fio_http_body_length(c->h) > c->settings->max_body_size)
    goto too_big;
  fio_http_body_write(c->h, chunk.buf, chunk.len);
  return 0;
too_big:
  fio___http_request_too_big(c);
  return 0;
}

/* *****************************************************************************
HTTP/1.1 Protocol
***************************************************************************** */

FIO_SFUNC int fio___http1_process_data(fio_io_s *io,
                                       fio___http_connection_s *c) {
  (void)io;
  for (;;) {
    size_t consumed = fio_http1_parse(&c->state.http.parser,
                                      FIO_BUF_INFO2(c->buf, c->len),
                                      (void *)c);
    if (!consumed)
      goto nothing_consumed;
    if (consumed == FIO_HTTP1_PARSER_ERROR)
      goto http1_error;
    c->len -= consumed;
    if (c->len)
      FIO_MEMMOVE(c->buf, c->buf + consumed, c->len);
    if (c->suspend)
      return -1;
  }
  return 0;

nothing_consumed:
  if (c->len == c->capa)
    goto http1_abuse;
  else
    return -1;

http1_error:
  FIO_LOG_DDEBUG2("(%d) HTTP/1.1 parser error! disconnecting client at %d",
                  fio_io_pid(),
                  fio_io_fd(io));
  if (c->h) {
    fio_http_s *h = c->h;
    c->h = NULL;
    if (!c->is_client) {
      fio_io_dup(c->io);
      if (fio_http_send_error_response(h, 400))
        fio_io_free(c->io);
    }
    fio_http_free(h);
  }
  fio_io_close(io);
  return -1;

http1_abuse:
  FIO_LOG_DDEBUG2(
      "(%d) HTTP/1.1 hit security limit, disconnecting client at %d",
      fio_io_pid(),
      fio_io_fd(io));
  if (c->h) {
    fio_http_s *h = c->h;
    c->h = NULL;
    if (!c->is_client) {
      fio_io_dup(c->io);
      if (fio_http_send_error_response(h, 431))
        fio_io_free(c->io);
    }
    fio_http_free(h);
  }
  fio_io_close(io);
  return -1;
}

// /** Called when a data is available. */
FIO_SFUNC void fio___http1_on_data(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  size_t r;
  for (;;) {
    if (c->capa == c->len)
      return;
    if (!(r = fio_io_read(io, c->buf + c->len, c->capa - c->len)))
      return;
    c->len += r;
    if (fio___http1_process_data(io, c))
      return;
  }
}

// /** Called when an IO is attached to a protocol. */
FIO_SFUNC void fio___http1_on_attach(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  if (c->len)
    fio___http1_process_data(io, c);
  return;
}

/* *****************************************************************************
HTTP/1.1 Client Protocol
***************************************************************************** */

/** Iterates through all cookies. A non-zero return will stop iteration. */
FIO_SFUNC int fio_http1___write_client_cookie_callback(fio_http_s *h,
                                                       fio_str_info_s name,
                                                       fio_str_info_s value,
                                                       void *udata) {
  fio_str_info_s *buf = (fio_str_info_s *)udata;
  fio_string_write2(buf,
                    FIO_STRING_REALLOC,
                    FIO_STRING_WRITE_STR2("cookie:", 7),
                    FIO_STRING_WRITE_STR_INFO(name),
                    FIO_STRING_WRITE_STR2("=", 1),
                    FIO_STRING_WRITE_STR_INFO(value),
                    FIO_STRING_WRITE_STR2("\r\n", 2));
  return 0;
  (void)h;
}

/** called by the HTTP handle for each header. */
FIO_SFUNC int fio_http1___write_header_callback(fio_http_s *h,
                                                fio_str_info_s name,
                                                fio_str_info_s value,
                                                void *out_) {
  (void)h;
  /* manually copy, as this is an "all or nothing" copy (no truncation) */
  fio_str_info_s *out = (fio_str_info_s *)out_;
  return fio_string_write2(out,
                           FIO_STRING_REALLOC,
                           FIO_STRING_WRITE_STR2(name.buf, name.len),
                           FIO_STRING_WRITE_STR2(":", 1),
                           FIO_STRING_WRITE_STR2(value.buf, value.len),
                           FIO_STRING_WRITE_STR2("\r\n", 2));
}

FIO_SFUNC void fio___http1_send_request(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_io_is_open(c->io))
    return;
  fio_str_info_s buf = FIO_STR_INFO2(NULL, 0);
  /* set Content-Length (client is never streaming) */
  if (fio_http_body_length(h)) {
    char ibuf[32];
    fio_str_info_s k = FIO_STR_INFO2((char *)"content-length", 14);
    fio_str_info_s v = FIO_STR_INFO3(ibuf, 0, 32);
    v.len = fio_digits10u(fio_http_body_length(h));
    fio_ltoa10u(v.buf, fio_http_body_length(h), v.len);
    fio_http_request_header_set(h, k, v);
  }
  { /* set sensible defaults for common headers (Accept, User-Agent) */
    fio_http_request_header_set_if_missing(h,
                                           FIO_STR_INFO1((char *)"accept"),
                                           FIO_STR_INFO1((char *)"*/*"));
    fio_http_request_header_set_if_missing(
        h,
        FIO_STR_INFO1((char *)"user-agent"),
        FIO_STR_INFO1((char *)"facil.io/" FIO_VERSION_STRING));
  }
  { /* write status string */
    fio_str_info_s method = fio_http_method(h);
    fio_str_info_s path = fio_http_path(h);
    fio_str_info_s version = fio_http_version(h);
    if (!path.len)
      path = FIO_STR_INFO1((char *)"/");
    if ((version.len - 1) > 15)
      version = FIO_STR_INFO1((char *)"HTTP/1.1");
    fio_string_write2(&buf,
                      FIO_STRING_REALLOC,
                      FIO_STRING_WRITE_STR_INFO(method),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_STR_INFO(path),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_STR_INFO(version),
                      FIO_STRING_WRITE_STR2("\r\n", 2));
  }
  /* write headers */
  fio_http_request_header_each(h, fio_http1___write_header_callback, &buf);
  /* write cookies */
  fio_http_cookie_each(h, fio_http1___write_client_cookie_callback, &buf);
  fio_string_write(&buf, FIO_STRING_REALLOC, "\r\n", 2);
  /* send data (moves memory ownership) */
  fio_io_write2(c->io,
                .buf = buf.buf,
                .len = buf.len,
                .dealloc = FIO_STRING_FREE,
                .copy = 0);
  /* make sure we listen to incoming data */
  c->suspend = 0;
  fio_io_unsuspend(c->io);
  /* Write Body */
  if (!fio_http_body_length(h))
    return;
  fio_http_body_seek(h, 0);
  if (fio_http_body_fd(h) == -1) {
    buf = fio_http_body_read(h, (size_t)-1);
    fio_io_write2(c->io,
                  .buf = (char *)fio_http_dup(h),
                  .len = buf.len,
                  .offset = (size_t)((char *)h - buf.buf),
                  .dealloc = (void (*)(void *))fio_http_free);
  } else {
    fio_io_write2(c->io,
                  .fd = fio_http_body_fd(h),
                  .len = fio_http_body_length(h),
                  .copy = 1);
  }
}

FIO_SFUNC void fio___http1_on_attach_client(fio_io_s *io) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_io_udata(io);
  // c->io = fio_io_dup(io);
  c->io = io;
  fio___http1_send_request(c->h);
  if (c->len)
    fio___http1_process_data(io, c);
  return;
}

/* *****************************************************************************
HTTP/1 Controller
***************************************************************************** */
FIO_SFUNC int fio___http_controller_get_fd(fio_http_s *h) {
  return fio_io_fd(fio_http_io(h));
}

/** Informs the controller that request / response headers must be sent. */
FIO_SFUNC void fio___http_controller_http1_send_headers(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_io_is_open(c->io))
    return;
  fio_str_info_s buf = FIO_STR_INFO2(NULL, 0);
  { /* write status string */
    fio_str_info_s ver = fio_http_version(h);
    fio_str_info_s status = fio_http_status2str(fio_http_status(h));
    if (ver.len > 15) {
      FIO_LOG_ERROR("HTTP/1.1 client version string too long!");
      ver = FIO_STR_INFO1((char *)"HTTP/1.1");
    }
    fio_string_write2(&buf,
                      FIO_STRING_REALLOC,
                      FIO_STRING_WRITE_STR2(ver.buf, ver.len),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_NUM(fio_http_status(h)),
                      FIO_STRING_WRITE_STR2(" ", 1),
                      FIO_STRING_WRITE_STR2(status.buf, status.len),
                      FIO_STRING_WRITE_STR2("\r\n", 2));
  }

  /* write headers */
  fio_http_response_header_each(h, fio_http1___write_header_callback, &buf);
  /* write cookies */
  fio_http_set_cookie_each(h, fio_http1___write_header_callback, &buf);
  /* add streaming headers? */
  if (fio_http_is_streaming(h))
    fio_string_write(&buf,
                     FIO_STRING_REALLOC,
                     "transfer-encoding: chunked\r\n",
                     28);
  fio_string_write(&buf, FIO_STRING_REALLOC, "\r\n", 2);
  /* send data (move memory ownership)? */
  c->state.http.buf = buf;
  return;
  // fio_io_write2(c->io,
  //               .buf = buf.buf,
  //               .len = buf.len,
  //               .dealloc = FIO_STRING_FREE,
  //               .copy = 0);
}
/** called by the HTTP handle for each body chunk (or to finish a response. */
FIO_SFUNC void fio___http_controller_http1_write_body(
    fio_http_s *h,
    fio_http_write_args_s args) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (!c->io || !fio_io_is_open(c->io))
    goto no_write_err;
  if (fio_http_is_streaming(h))
    goto stream_chunk;
  if (c->state.http.buf.len) {
    if (args.buf && args.len) {
      fio_string_write(&c->state.http.buf,
                       FIO_STRING_REALLOC,
                       (char *)args.buf + args.offset,
                       args.len);
      if (args.dealloc)
        args.dealloc((void *)args.buf);
    }
    fio_io_write2(c->io,
                  .buf = (void *)c->state.http.buf.buf,
                  .len = c->state.http.buf.len,
                  .dealloc = FIO_STRING_FREE);
    c->state.http.buf = FIO_STR_INFO0;
    if (args.buf && args.len)
      return;
  }

  fio_io_write2(c->io,
                .buf = (void *)args.buf,
                .fd = args.fd,
                .len = args.len,
                .offset = args.offset,
                .dealloc = args.dealloc,
                .copy = (uint8_t)args.copy);
  return;

stream_chunk:
  if (args.len && args.buf) { /* String */
    if (args.copy || args.len < (1 << 16)) {
      fio_string_write2(
          &c->state.http.buf,
          FIO_STRING_REALLOC,
          FIO_STRING_WRITE_HEX(args.len),   /* chunk header - length */
          FIO_STRING_WRITE_STR2("\r\n", 2), /* chunk header - EOL */
          FIO_STRING_WRITE_STR2((char *)args.buf, args.buf ? args.len : 0),
          FIO_STRING_WRITE_STR2("\r\n", 2)); /* chunk trailer - EOL */
      fio_io_write2(c->io,
                    .buf = (void *)c->state.http.buf.buf,
                    .len = c->state.http.buf.len,
                    .dealloc = FIO_STRING_FREE);
      c->state.http.buf = FIO_STR_INFO0;
      if (args.dealloc)
        args.dealloc((void *)args.buf);
      return;
    } else { /* avoid copying the incoming data if possible */
      FIO_STR_INFO_TMP_VAR(buf, 32);
      if (c->state.http.buf.buf)
        buf = c->state.http.buf;
      c->state.http.buf = FIO_STR_INFO0;
      fio_string_write2(
          &buf,
          NULL,
          FIO_STRING_WRITE_HEX(args.len),    /* chunk header - length */
          FIO_STRING_WRITE_STR2("\r\n", 2)); /* chunk header - EOL */
      fio_io_write2(c->io,
                    .buf = buf.buf,
                    .len = buf.len,
                    .copy = !FIO_STR_INFO_TMP_IS_REALLOCATED(buf),
                    .dealloc = FIO_STR_INFO_TMP_IS_REALLOCATED(buf)
                                   ? FIO_STRING_FREE
                                   : NULL);
      fio_io_write2(c->io,
                    .buf = (void *)args.buf,
                    .len = args.len,
                    .dealloc = args.dealloc);
      /* chunk trailer - EOL */
      fio_io_write2(c->io, .buf = (void *)"\r\n", .len = 2);
      return;
    }
  } else if ((uint32_t)(args.fd + 1) > 1U) { /* File? */
    if (!args.len) {
      if (fio_fd_type(args.fd) != S_IFREG)
        goto no_length_err;
      /* collect remaining file length */
      off_t len = fio_fd_size(args.fd);
      off_t offset = lseek(args.fd, 0, SEEK_CUR);
      if (len > 0 && offset > 0)
        len -= offset;
      if (len <= 0)
        goto no_length_err;
      args.len = (size_t)len;
    }
    FIO_STR_INFO_TMP_VAR(buf, 32);
    if (c->state.http.buf.buf)
      buf = c->state.http.buf;
    c->state.http.buf = FIO_STR_INFO0;
    fio_string_write2(
        &buf,
        NULL,
        FIO_STRING_WRITE_HEX(args.len),    /* chunk header - length */
        FIO_STRING_WRITE_STR2("\r\n", 2)); /* chunk header - EOL */
    fio_io_write2(
        c->io,
        .buf = buf.buf,
        .len = buf.len,
        .copy = !FIO_STR_INFO_TMP_IS_REALLOCATED(buf),
        .dealloc =
            (FIO_STR_INFO_TMP_IS_REALLOCATED(buf) ? FIO_STRING_FREE : NULL));
    fio_io_write2(c->io,
                  .fd = args.fd,
                  .len = args.len,
                  .copy = (bool)args.copy,
                  .dealloc = args.dealloc);
    /* chunk trailer - EOL */
    fio_io_write2(c->io, .buf = (void *)"\r\n", .len = 2);
  } else
    goto no_write_err;
  return;
no_length_err:
  FIO_LOG_ERROR("HTTP1 streaming requires a correctly pre-determined "
                "length per chunk.");
no_write_err:
  if (args.buf) {
    if (args.dealloc)
      args.dealloc((void *)args.buf);
  } else if ((uint32_t)(args.fd + 1) > 1U) {
    close(args.fd);
  }
}

FIO_SFUNC void fio___http_controller_http1_on_finish_task(void *c_,
                                                          void *upgraded) {
  fio___http_connection_s *c = (fio___http_connection_s *)c_;
  c->suspend = 0;

  if (upgraded)
    goto upgraded;

  if (!c->io)
    goto no_io;

  if (fio_io_is_open(c->io)) {
    /* TODO: test for connection:close header and h->status values */
    fio___http1_process_data(c->io, c);
  }
  if (!c->suspend)
    fio_io_unsuspend(c->io);
  fio_io_free(c->io);
  return;

upgraded:
  if (c->h || !fio_io_is_open(c->io))
    goto something_is_wrong;
  c->h = (fio_http_s *)upgraded;
  { /* TODO! test if safe to move to user thread for callback execution? */
    const size_t pr_i = fio_http_is_websocket(c->h) ? FIO___HTTP_PROTOCOL_WS
                                                    : FIO___HTTP_PROTOCOL_SSE;
    fio_http_controller_set(
        c->h,
        &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
              ->state[pr_i]
              .controller));
    fio_io_protocol_set(
        c->io,
        &(FIO_PTR_FROM_FIELD(fio___http_protocol_s, settings, c->settings)
              ->state[pr_i]
              .protocol));
    if (pr_i == FIO___HTTP_PROTOCOL_SSE) {
      fio_str_info_s last_id =
          fio_http_request_header(c->h,
                                  FIO_STR_INFO2((char *)"last-event-id", 13),
                                  0);
      if (last_id.buf)
        c->settings->on_eventsource_reconnect(c->h, FIO_STR2BUF_INFO(last_id));
    }
  }
  fio_io_unsuspend(c->io);
  fio_io_free(c->io);
  return;

something_is_wrong:
  if (fio_io_is_open(c->io))
    FIO_LOG_DEBUG2("(%d) Connection upgrade went wrong for fd %d - closing",
                   fio_io_pid(),
                   fio_io_fd(c->io));
  fio_io_protocol_set(c->io, NULL); /* make zombie, timeout will clear it. */
  fio_io_free(c->io);
/* fall through */
no_io:
  fio___http_connection_free(c); /* free HTTP connection element */
}

/** called once a request / response had finished */
FIO_SFUNC void fio___http_controller_http1_on_finish(fio_http_s *h) {
  fio___http_connection_s *c = (fio___http_connection_s *)fio_http_cdata(h);
  if (c->state.http.buf.len) {
    if (fio_http_is_streaming(h))
      fio_string_write(&c->state.http.buf, FIO_STRING_REALLOC, "0\r\n\r\n", 5);
    fio_io_write2(c->io,
                  .buf = (void *)c->state.http.buf.buf,
                  .len = c->state.http.buf.len,
                  .dealloc = FIO_STRING_FREE);
    c->state.http.buf = FIO_STR_INFO0;
  } else {
    if (fio_http_is_streaming(h))
      fio_io_write2(c->io, .buf = (char *)"0\r\n\r\n", .len = 5, .copy = 1);
  }
  if (c->log)
    fio_http_write_log(h);
  if (fio_http_is_upgraded(h))
    goto upgraded;
  /* once the function returns, `h` may be freed (auto-finish on free).
   * so we must call this callback here (sync), no matter the thread */
  c->state.http.on_finish(h);
  fio_io_defer(fio___http_controller_http1_on_finish_task, (void *)(c), NULL);
  return;

upgraded:
  fio_io_defer(fio___http_controller_http1_on_finish_task,
               (void *)(c),
               (void *)h);
}

/* *****************************************************************************
HTTP/1.1 Finish
***************************************************************************** */
#endif /* FIO_HTTP */
