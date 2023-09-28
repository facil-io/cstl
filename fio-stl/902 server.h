/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TEST_ALL           /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                            Server Test Helper




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TEST_ALL) && !defined(FIO___TEST_REINCLUDE) &&                 \
    !defined(H___FIO_SERVER_TEST___H)
#define H___FIO_SERVER_TEST___H
#ifndef H___FIO_SERVER___H
#define FIO_SERVER
#define FIO___TEST_REINCLUDE
#include FIO_INCLUDE_FILE
#undef FIO___TEST_REINCLUDE
#endif
/* *****************************************************************************
Test TLS support
***************************************************************************** */

FIO_SFUNC int FIO_NAME_TEST(FIO_NAME_TEST(stl, server),
                            tls_each_cert)(fio_tls_each_s *e,
                                           const char *nm,
                                           const char *public_cert_file,
                                           const char *private_key_file,
                                           const char *pk_password) {
  size_t *result = (size_t *)e->udata2;
  *result += 0x01U;
  const size_t step = result[0] & 0xFF;
  struct {
    const char *s[4];
  } d =
      {
          {nm, public_cert_file, private_key_file, pk_password},
      },
    ex = {{NULL, "cert.pem", "key.pem", "1234"}};
  FIO_ASSERT(nm && nm[0] == (char)('0' + step), "nm error for tls_each_cert");
  for (size_t i = 1; i < 4; ++i) {
    FIO_ASSERT(d.s[i] && ex.s[i] && FIO_STRLEN(ex.s[i]) == FIO_STRLEN(d.s[i]) &&
                   !memcmp(ex.s[i], d.s[i], FIO_STRLEN(d.s[i])),
               "tls_each_cert string error for argument %zu",
               i);
  }
  return 0;
}
FIO_SFUNC int FIO_NAME_TEST(FIO_NAME_TEST(stl, server),
                            tls_each_alpn)(fio_tls_each_s *e,
                                           const char *nm,
                                           void (*fn)(fio_s *)) {
  size_t *result = (size_t *)e->udata2;
  *result += 0x0100U;
  const size_t step = (result[0] >> 8) & 0xFF;
  FIO_ASSERT(nm && nm[0] == (char)('0' + step), "nm error for tls_each_alpn");
  FIO_ASSERT((uintptr_t)fn == step, "fn value error for tls_each_alpn");
  return 0;
}
FIO_SFUNC int FIO_NAME_TEST(FIO_NAME_TEST(stl, server),
                            tls_each_trust)(fio_tls_each_s *e, const char *nm) {

  size_t *result = (size_t *)e->udata2;
  *result += 0x010000U;
  const size_t step = (result[0] >> 16) & 0xFF;
  FIO_ASSERT(nm && nm[0] == (char)('0' + step), "nm error for tls_each_trust");
  return 0;
}

FIO_SFUNC void FIO_NAME_TEST(FIO_NAME_TEST(stl, server),
                             tls_each_alpn_cb)(fio_s *io) {
  ((size_t *)io)[0]++;
}

FIO_SFUNC void FIO_NAME_TEST(FIO_NAME_TEST(stl, server), tls_helpers)(void) {
  fprintf(stderr, "   * Testing fio_tls_s helpers.\n");
  struct {
    const char *nm;
    const char *public_cert_file;
    const char *private_key_file;
    const char *pk_password;
  } tls_test_cert_data[] = {
      {
          .nm = "1",
          .public_cert_file = "c.pem",
          .private_key_file = "k.pem",
          .pk_password = NULL,
      },
      {
          .nm = "2",
          .public_cert_file = "cert.pem",
          .private_key_file = "key.pem",
          .pk_password = "1234",
      },
      {
          .nm = "1",
          .public_cert_file = "cert.pem",
          .private_key_file = "key.pem",
          .pk_password = "1234",
      },
      {
          .nm = "3",
          .public_cert_file = "cert.pem",
          .private_key_file = "key.pem",
          .pk_password = "1234",
      },
      {NULL},
  };
  struct {
    const char *nm;
    void (*fn)(fio_s *);
  } tls_test_alpn_data[] = {
      {
          .nm = "1",
          .fn = (void (*)(fio_s *))(uintptr_t)3,
      },
      {
          .nm = "2",
          .fn = (void (*)(fio_s *))(uintptr_t)2,
      },
      {
          .nm = "1",
          .fn = (void (*)(fio_s *))(uintptr_t)1,
      },
      {NULL},
  };
  struct {
    const char *nm;
  } tls_test_trust_data[] = {
      {
          .nm = "1",
      },
      {
          .nm = "2",
      },
      {NULL},
  };
  size_t counter = 0;
  void *data_containers[] = {
      (void *)&tls_test_cert_data,
      (void *)&tls_test_alpn_data,
      (void *)&tls_test_trust_data,
      NULL,
  };
  fio_tls_s *t = fio_tls_new();
  FIO_ASSERT(t, "fio_tls_new should return a valid fio_tls_s object");
  for (size_t i = 0; tls_test_cert_data[i].nm; ++i) {
    fio_tls_s *r = fio_tls_cert_add(t,
                                    tls_test_cert_data[i].nm,
                                    tls_test_cert_data[i].public_cert_file,
                                    tls_test_cert_data[i].private_key_file,
                                    tls_test_cert_data[i].pk_password);
    FIO_ASSERT(r == t, "`fio_tls_X_add` functions should return `self`.");
  }
  for (size_t i = 0; tls_test_alpn_data[i].nm; ++i) {
    fio_tls_s *r =
        fio_tls_alpn_add(t, tls_test_alpn_data[i].nm, tls_test_alpn_data[i].fn);
    FIO_ASSERT(r == t, "`fio_tls_X_add` functions should return `self`.");
  }
  for (size_t i = 0; tls_test_trust_data[i].nm; ++i) {
    fio_tls_s *r = fio_tls_trust_add(t, tls_test_trust_data[i].nm);
    FIO_ASSERT(r == t, "`fio_tls_X_add` functions should return `self`.");
  }

  fio_tls_each(
      t,
      .udata = data_containers,
      .udata2 = &counter,
      .each_cert = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), tls_each_cert),
      .each_alpn = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), tls_each_alpn),
      .each_trust = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), tls_each_trust));
  FIO_ASSERT(counter == 0x020203, "fio_tls_each iteration count error.");
  fio_tls_alpn_add(t,
                   "tst",
                   FIO_NAME_TEST(FIO_NAME_TEST(stl, server), tls_each_alpn_cb));
  counter = 0;
  fio_tls_alpn_select(t, "tst", 3, (fio_s *)&counter);
  FIO_ASSERT(counter == 1, "fio_tls_alpn_select failed.");
  fio_tls_free(t);

  const struct {
    fio_buf_info_s url;
    size_t is_tls;
  } url_tests[] = {
      {FIO_BUF_INFO1((char *)"ws://ex.com"), 0},
      {FIO_BUF_INFO1((char *)"wss://ex.com"), 1},
      {FIO_BUF_INFO1((char *)"sse://ex.com"), 0},
      {FIO_BUF_INFO1((char *)"sses://ex.com"), 1},
      {FIO_BUF_INFO1((char *)"http://ex.com"), 0},
      {FIO_BUF_INFO1((char *)"https://ex.com"), 1},
      {FIO_BUF_INFO1((char *)"tcp://ex.com"), 0},
      {FIO_BUF_INFO1((char *)"tcps://ex.com"), 1},
      {FIO_BUF_INFO1((char *)"udp://ex.com"), 0},
      {FIO_BUF_INFO1((char *)"udps://ex.com"), 1},
      {FIO_BUF_INFO1((char *)"tls://ex.com"), 1},
      {FIO_BUF_INFO1((char *)"ws://ex.com/?TLSN"), 0},
      {FIO_BUF_INFO1((char *)"ws://ex.com/?TLS"), 1},
      {FIO_BUF_INFO0, 0},
  };
  for (size_t i = 0; url_tests[i].url.buf; ++i) {
    t = NULL;
    fio_url_s u = fio_url_parse(url_tests[i].url.buf, url_tests[i].url.len);
    t = fio_tls_from_url(t, u);
    FIO_ASSERT((!url_tests[i].is_tls && !t) || (url_tests[i].is_tls && t),
               "fio_tls_from_url result error @ %s",
               url_tests[i].url.buf);
    fio_tls_free(t);
  }
}

/* *****************************************************************************
Test IO ENV support
***************************************************************************** */

/* State callback test task */
FIO_SFUNC void FIO_NAME_TEST(FIO_NAME_TEST(stl, server),
                             env_on_close)(void *udata) {
  size_t *p = (size_t *)udata;
  ++p[0];
}

/* State callback tests */
FIO_SFUNC void FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env)(void) {
  fprintf(stderr, "   * Testing fio_env.\n");
  size_t a = 0, b = 0, c = 0;
  fio___srv_env_safe_s env = FIO___SRV_ENV_SAFE_INIT;
  fio___srv_env_safe_set(
      &env,
      (char *)"a_key",
      5,
      1,
      (fio___srv_env_obj_s){
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close),
          .udata = &a},
      1);
  FIO_ASSERT(fio___srv_env_safe_get(&env, (char *)"a_key", 5, 1) == &a,
             "fio___srv_env_safe_set/get round-trip error!");
  fio___srv_env_safe_set(
      &env,
      (char *)"a_key",
      5,
      2,
      (fio___srv_env_obj_s){
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close),
          .udata = &a},
      2);
  fio___srv_env_safe_set(
      &env,
      (char *)"a_key",
      5,
      3,
      (fio___srv_env_obj_s){
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close),
          .udata = &a},
      1);
  fio___srv_env_safe_set(
      &env,
      (char *)"b_key",
      5,
      1,
      (fio___srv_env_obj_s){
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close),
          .udata = &b},
      1);
  fio___srv_env_safe_set(
      &env,
      (char *)"c_key",
      5,
      1,
      (fio___srv_env_obj_s){
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close),
          .udata = &c},
      1);
  fio___srv_env_safe_unset(&env, (char *)"a_key", 5, 3);
  FIO_ASSERT(!a,
             "unset should have removed an object without calling callback.");
  fio___srv_env_safe_remove(&env, (char *)"a_key", 5, 3);
  FIO_ASSERT(!a, "remove after unset should have no side-effects.");
  fio___srv_env_safe_remove(&env, (char *)"a_key", 5, 2);
  FIO_ASSERT(a == 1, "remove should call callbacks.");
  fio___srv_env_safe_destroy(&env);
  FIO_ASSERT(a == 2 && b == 1 && c == 1, "destroy should call callbacks.");
}

/* *****************************************************************************
Test Server Modules
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, server)(void) {
  fprintf(stderr, "* Testing fio_srv units (TODO).\n");
  FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env)();
  FIO_NAME_TEST(FIO_NAME_TEST(stl, server), tls_helpers)();
}
/* *****************************************************************************
Cleanup
***************************************************************************** */
#endif /* FIO_TEST_ALL */
