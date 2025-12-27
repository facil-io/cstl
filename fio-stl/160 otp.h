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

/**
 * Returns a TOTP for a specific unix timestamp (for testing/verification).
 * This is useful for verifying OTPs at specific times or for RFC test vectors.
 */
SFUNC uint32_t fio_otp_at(fio_buf_info_s secret,
                          uint64_t unix_time,
                          fio_otp_settings_s settings);
#define fio_otp_at(secret, unix_time, ...)                                     \
  fio_otp_at(secret, unix_time, (fio_otp_settings_s){__VA_ARGS__})

/* *****************************************************************************
TOTP Implementation
***************************************************************************** */

/**
 * Generates a cryptographically secure random 128 bit key for TOTP processing.
 * Uses system CSPRNG via fio_rand_bytes_secure().
 */
FIO_IFUNC fio_u128 fio_otp_generate_key(void) {
  fio_u128 k = {0};
  /* Ensure non-zero (extremely unlikely to be zero with 128 bits) */
  while (k.u64[0] == 0 || k.u64[1] == 0) {
    if (fio_rand_bytes_secure(k.u8, sizeof(k)) != 0)
      k = fio_rand128();
  }
  return k;
}

/** Prints out an OTP secret (big endian number) as a Byte32 encoded String. */
FIO_IFUNC size_t fio_otp_print_key(char *dest, uint8_t *key, size_t len) {
  fio_u128 buf;
  if (!key) {
    buf = fio_otp_generate_key();
    key = buf.u8;
    len = sizeof(buf); /* 16 bytes */
  }
  /* Base32 encoding: 5 bits per char, so len*8/5 chars + null terminator */
  /* Using len*2 as safe upper bound (1.6x actual need) */
  fio_str_info_s s = FIO_STR_INFO3(dest, 0, (len * 2) + 1);
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

/* Internal: compute OTP from raw secret and time counter */
FIO_SFUNC uint32_t fio___otp_compute(fio_buf_info_s key,
                                     uint64_t unix_time,
                                     fio_otp_settings_s settings) {
  uint32_t r = 0;
  uint64_t t = unix_time;
  fio_u1024 s = fio_u1024_init64(0);
  fio_sha1_s hash;
  fio_str_info_s secret = FIO_STR_INFO3((char *)s.u8, 0, (1024 / 8));
  fio___otp_settings_validate(&settings);

  /* Prep time */
  t -= (settings.offset * settings.interval);
  t /= settings.interval;
  /* t should be big endian */
  t = fio_lton64(t);

  if (settings.is_raw) {
    /* raw key - hash if too long for HMAC-SHA1 (block size 64 bytes) */
    if (key.len > 64) {
      s.u512[0] = fio_sha512(key.buf, key.len);
      secret.len = 64;
      FIO_LOG_WARNING("OTP hex key too long (%zu bytes), hashing to 64 bytes",
                      key.len);
    } else {
      secret = FIO_BUF2STR_INFO(key);
    }
  } else if (settings.is_hex) {
    /* decode Hex key input OTP */
    if (key.len >= (1024 / (8 * 2))) {
      FIO_LOG_WARNING("OTP hex key too long (%zu bytes), hashing to 64 bytes",
                      key.len);
    }
    size_t written = 0; /* fun times... */
    for (size_t i = 0; i < key.len; ++i) {
      if (key.buf[i] == '-' || key.buf[i] == ' ' || key.buf[i] == '\n')
        continue;
      const size_t pos = written >> 1;
      if (pos >= (1024 / 8))
        break; /* stop if we exceed buffer */
      if (!(written & 1))
        secret.buf[pos] = 0;
      secret.buf[pos] |= (fio_c2i(key.buf[i]) << (((++written) & 1) << 2));
    }
    secret.len = (written >> 1) + (written & 1);
    /* if decoded key is too long for HMAC-SHA1, hash it */
    if (secret.len > 64) {
      fio_u512 tmp = fio_sha512(secret.buf, secret.len);
      s.u512[0] = tmp;
      secret.len = 64;
    }
  } else {
    /* decode Byte32 key input OTP */
    if (fio_string_write_base32dec(&secret, NULL, key.buf, key.len))
      return -1;
    /* if decoded key is too long for HMAC-SHA1, hash it */
    if (secret.len > 64) {
      FIO_LOG_WARNING(
          "OTP base32 key too long (%zu bytes), truncating decode buffer",
          key.len);
      fio_u512 tmp = fio_sha512(secret.buf, secret.len);
      s.u512[0] = tmp;
      secret.len = 64;
      if (key.len >= ((64 * 8) / 5)) {
        fio_sha1_s h = fio_sha1(key.buf, key.len);
        for (int i = 0; i < 5; ++i)
          s.u32[i] ^= h.v[i];
        for (int i = 0; i < 5; ++i)
          s.u32[i + 7] ^= h.v[i];
      }
    }
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

uint32_t fio_otp___(void);
SFUNC uint32_t fio_otp FIO_NOOP(fio_buf_info_s key,
                                fio_otp_settings_s settings) {
  return fio___otp_compute(key, (uint64_t)fio_time_real().tv_sec, settings);
}

SFUNC uint32_t fio_otp_at FIO_NOOP(fio_buf_info_s key,
                                   uint64_t unix_time,
                                   fio_otp_settings_s settings) {
  return fio___otp_compute(key, unix_time, settings);
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_OTP */
#undef FIO_OTP
