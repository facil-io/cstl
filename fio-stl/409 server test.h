/* *****************************************************************************
Simple Server Testing
***************************************************************************** */
#if defined(FIO_TEST_CSTL) && defined(FIO_SERVER) &&                           \
    !defined(FIO___STL_KEEP) && !defined(FIO_FIO_TEST_SERVER_ONLY_ONCE) &&     \
    (!defined(FIO_EXTERN) || defined(FIO_EXTERN_COMPLETE))
#define FIO_FIO_TEST_SERVER_ONLY_ONCE 1
/* *****************************************************************************
Test IO ENV support
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
  fio_tls_alpn_select(t, "tst", (fio_s *)&counter);
  FIO_ASSERT(counter == 1, "fio_tls_alpn_select failed.");
  fio_tls_free(t);
}

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
          .udata = &a,
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close)},
      1);
  fio___srv_env_safe_set(
      &env,
      (char *)"a_key",
      5,
      2,
      (fio___srv_env_obj_s){
          .udata = &a,
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close)},
      2);
  fio___srv_env_safe_set(
      &env,
      (char *)"a_key",
      5,
      3,
      (fio___srv_env_obj_s){
          .udata = &a,
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close)},
      1);
  fio___srv_env_safe_set(
      &env,
      (char *)"b_key",
      5,
      1,
      (fio___srv_env_obj_s){
          .udata = &b,
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close)},
      1);
  fio___srv_env_safe_set(
      &env,
      (char *)"c_key",
      5,
      1,
      (fio___srv_env_obj_s){
          .udata = &c,
          .on_close = FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env_on_close)},
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
  FIO_NAME_TEST(FIO_NAME_TEST(stl, server), tls_helpers)();
}

/* *****************************************************************************
Test summery
***************************************************************************** */

FIO_SFUNC void FIO_NAME_TEST(stl, server)(void) {
  fprintf(stderr, "* Testing fio_srv units (TODO).\n");
  FIO_NAME_TEST(FIO_NAME_TEST(stl, server), env)();
}

/* *****************************************************************************
Simple Server Cleanup
***************************************************************************** */
#undef FIO_SERVER
#endif /* FIO_TEST_CSTL */
