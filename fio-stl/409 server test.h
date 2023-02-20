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
