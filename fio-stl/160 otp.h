/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_OTP                /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                                    OTP (SHA1)



Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_OTP) && !defined(H___FIO_OTP___H)
#define H___FIO_OTP___H

/* *****************************************************************************
TOTP API
***************************************************************************** */

typedef struct {
  /** The time interval for TOTP rotation. */
  size_t interval; /* 30 == Google OTP */
  /** The number of digits in the OTP. */
  size_t digits; /* 6 == Google OTP */
  /** The time offset (in `interval` units) from the current time. */
  int64_t offset; /* 0 == Google OTP */
  /** Set to true if the secret / key is in Hex instead of Byte32 encoding. */
  uint8_t is_hex;
  /** Set to true if the secret / key is raw bit data (no encoding). */
  uint8_t is_raw;
} fio_otp_settings_s;

/** Generates a random 128 bit key for TOTP processing. */
FIO_IFUNC fio_u128 fio_otp_generate_key(void);

/** Prints out an OTP secret (big endian number) as a Byte32 encoded String. */
FIO_IFUNC size_t fio_otp_print_key(char *dest, uint8_t *key, size_t len);

/** Returns a TOTP based on `secret` and the otp settings. */
SFUNC uint32_t fio_otp(fio_buf_info_s secret, fio_otp_settings_s settings);
#define fio_otp(secret, ...) fio_otp(secret, (fio_otp_settings_s){__VA_ARGS__})

/* *****************************************************************************
TOTP Implementation
***************************************************************************** */

/** Generates a random 128 bit key for TOTP processing. */
FIO_IFUNC fio_u128 fio_otp_generate_key(void) {
  fio_u128 k = fio_u128_init64(fio_rand64(), fio_rand64());
  while (k.u64[0] == 0)
    k.u64[0] = fio_rand64();
  while (k.u64[1] == 0)
    k.u64[1] = fio_rand64();
  return k;
}

/** Prints out an OTP secret (big endian number) as a Byte32 encoded String. */
FIO_IFUNC size_t fio_otp_print_key(char *dest, uint8_t *key, size_t len) {
  fio_str_info_s s = FIO_STR_INFO3(dest, 0, (len * 2) + 1);
  fio_u128 buf;
  if (!key) {
    buf = fio_otp_generate_key();
    s.capa = 20;
    key = buf.u8;
  }
  FIO_ASSERT(!fio_string_write_base32enc(&s, NULL, key, len),
             "writing the generated OTP key failed");
  return s.len;
}

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

FIO_IFUNC void fio___otp_settings_validate(fio_otp_settings_s *s) {
  if (!s->interval)
    s->interval = 30;
  if (!s->digits)
    s->digits = 6;
}
uint32_t fio_otp___(void);
SFUNC uint32_t fio_otp FIO_NOOP(fio_buf_info_s key,
                                fio_otp_settings_s settings) {
  uint32_t r = 0;
  uint64_t t = fio_time_real().tv_sec;
  fio_u1024 s = fio_u1024_init64(0);
  fio_sha1_s hash;
  fio_str_info_s secret = FIO_STR_INFO3((char *)s.u8, 0, (1024 / 8));
  fio___otp_settings_validate(&settings);

  /* Prep time */
  t -= (settings.offset * settings.interval);
  t /= settings.interval;
  /* t should be big endian */
  t = fio_lton64(t);

  if (settings.is_raw)
    secret = FIO_BUF2STR_INFO(key);
  else if (settings.is_hex) {
    /* decode Hex key input OTP */
    FIO_ASSERT(key.len < (1024 / (8 * 2)), "key too long");
    size_t written = 0; /* fun times... */
    for (size_t i = 0; i < key.len; ++i) {
      if (key.buf[i] == '-' || key.buf[i] == ' ' || key.buf[i] == '\n')
        continue;
      const size_t pos = written >> 1;
      if (!(written & 1))
        secret.buf[pos] = 0;
      secret.buf[pos] |= (fio_c2i(key.buf[i]) << (((++written) & 1) << 2));
    }
    secret.len = (written >> 1) + (written & 1);
  } else {
    /* decode Byte32 key input OTP */
    FIO_ASSERT(key.len < ((64 * 8) / 5), "key too long");
    if (fio_string_write_base32dec(&secret, NULL, key.buf, key.len))
      return -1;
  }

  /* compute HMAC (HOTP of T / TOTP)  */
  hash = fio_sha1_hmac(secret.buf, secret.len, &t, sizeof(t));

  /* compute unsigned int as a function of offset from hash */
  size_t offset = (fio_sha1_digest(&hash)[fio_sha1_len() - 1] & 0x0F);
  fio_sha1_digest(&hash)[offset] &= 0x7F;
  r = fio_buf2u32_be(fio_sha1_digest(&hash) + offset);

  /*  reduce number of digits */
  switch (settings.digits) {
  case 1: offset = 10; break;
  case 2: offset = 100; break;
  case 3: offset = 1000; break;
  case 4: offset = 10000; break;
  case 5: offset = 100000; break;
  case 6: offset = 1000000; break;
  case 7: offset = 10000000; break;
  case 8: offset = 100000000; break;
  case 9: offset = 1000000000; break;
  case 10: offset = 10000000000; break;
  }
  r %= offset;

  return r;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_SHA1 */
#undef FIO_SHA1
