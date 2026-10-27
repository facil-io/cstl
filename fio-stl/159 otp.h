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

/**
 * Prints out an OTP secret (big endian number) as a Byte32 encoded String.
 *
 * `dest` must hold `len * 2 + 1` bytes; if `key` is NULL a new 16 byte key is
 * generated (`len` is ignored) and `dest` must hold 33 bytes.
 *
 * Returns the encoded length, or 0 on error.
 */
FIO_IFUNC size_t fio_otp_print_key(char *dest, uint8_t *key, size_t len);

/** Returned by `fio_otp` / `fio_otp_at` on error (never a valid OTP). */
#define FIO_OTP_ERROR ((uint32_t)0xFFFFFFFFUL)

/**
 * Returns a TOTP based on `secret` and the otp settings.
 *
 * Returns `FIO_OTP_ERROR` for an invalid secret encoding, a decoded secret
 * longer than 128 bytes or `digits` above 10.
 */
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
  size_t r = 0;
  if (!dest)
    return r;
  dest[0] = 0;
  if (!key) {
    buf = fio_otp_generate_key();
    key = buf.u8;
    len = sizeof(buf); /* 16 bytes: 26 chars + NUL (`dest` needs 33 bytes) */
  }
  if (len <= ((SIZE_MAX - 1) >> 1)) {
    /* unpadded Base32 (authenticator apps): ceil(len * 8 / 5) + NUL fits in
     * `len * 2 + 1`; full 5 byte groups need no padding, the tail is encoded
     * separately and its RFC 4648 `=` padding is dropped. */
    char tail[16];
    const size_t full = len - (len % 5);
    fio_str_info_s s = FIO_STR_INFO3(dest, 0, (len * 2) + 1);
    fio_str_info_s t = FIO_STR_INFO3(tail, 0, sizeof(tail));
    if (!fio_string_write_base32enc(&s, NULL, key, full) &&
        !fio_string_write_base32enc(&t, NULL, key + full, len - full)) {
      for (size_t i = 0; i < t.len && tail[i] != '='; ++i)
        dest[s.len++] = tail[i];
      dest[s.len] = 0;
      r = s.len;
    } else
      dest[0] = 0;
    fio_secure_zero(tail, sizeof(tail));
  }
  if (key == buf.u8)
    fio_secure_zero(&buf, sizeof(buf));
  return r;
}

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* Internal: true for separators ignored in Hex / Base32 secrets. */
FIO_IFUNC int fio___otp_is_sep(uint8_t c) {
  return c == ' ' || (c >= '\b' && c <= '\r') || c == '-' || c == ':';
}

/*
 * Internal: decodes a Base32 secret (any case, separators, optional `=`
 * padding, any trailing bits) into `dest`. Returns the length, or -1 on an
 * invalid character, data after padding or more than `capa` bytes.
 */
FIO_SFUNC size_t fio___otp_base32_decode(uint8_t *dest,
                                         size_t capa,
                                         fio_buf_info_s key) {
  size_t len = 0, bits = 0;
  uint32_t val = 0;
  uint8_t padding = 0;
  for (size_t i = 0; i < key.len; ++i) {
    const uint8_t c = (uint8_t)key.buf[i];
    uint8_t n;
    if (fio___otp_is_sep(c))
      continue;
    if (c == '=') {
      padding = 1;
      continue;
    }
    if ((c | 32) >= 'a' && (c | 32) <= 'z')
      n = (uint8_t)((c | 32) - 'a');
    else if (c >= '2' && c <= '7')
      n = (uint8_t)(c - '2' + 26);
    else
      return (size_t)-1;
    if (padding)
      return (size_t)-1;
    val = (val << 5) | n;
    bits += 5;
    if (bits < 8)
      continue;
    bits -= 8;
    if (len == capa)
      return (size_t)-1;
    dest[len++] = (uint8_t)(val >> bits);
  }
  return len;
}

/* Internal: compute OTP from raw secret and time counter */
FIO_SFUNC uint32_t fio___otp_compute(fio_buf_info_s key,
                                     uint64_t unix_time,
                                     fio_otp_settings_s settings) {
  /* 10^digits for 1..9 digits; 10 digits need no reduction (r < 2^31). */
  static const uint32_t modulus[10] = {0,
                                       10,
                                       100,
                                       1000,
                                       10000,
                                       100000,
                                       1000000,
                                       10000000,
                                       100000000,
                                       1000000000};
  uint32_t r = FIO_OTP_ERROR;
  uint64_t t = unix_time;
  fio_u1024 s; /* decoded key buffer, always written before it is read */
  fio_sha1_s hash;
  fio_str_info_s secret = FIO_STR_INFO3((char *)s.u8, 0, sizeof(s));
  if (!settings.interval)
    settings.interval = 30;
  if (!settings.digits)
    settings.digits = 6;
  if (settings.digits > 10 || (!key.buf && key.len))
    return r;

  /* Prep time (unsigned math wraps, never undefined) */
  t -= (uint64_t)settings.offset * (uint64_t)settings.interval;
  t /= (uint64_t)settings.interval;
  /* t should be big endian */
  t = fio_lton64(t);

  if (settings.is_raw) {
    /* raw key - HMAC-SHA1 hashes keys longer than its 64 byte block */
    secret = FIO_BUF2STR_INFO(key);
  } else if (settings.is_hex) {
    /* decode Hex key (an odd trailing nibble becomes a high nibble) */
    size_t written = 0;
    for (size_t i = 0; i < key.len; ++i) {
      const uint8_t c = (uint8_t)key.buf[i];
      if (fio___otp_is_sep(c))
        continue;
      const uint8_t n = fio_c2i(c);
      if (n > 15 || (written >> 1) >= sizeof(s))
        goto cleanup; /* invalid hex digit or key too long */
      if (!(written & 1))
        s.u8[written >> 1] = (uint8_t)(n << 4);
      else
        s.u8[written >> 1] |= n;
      ++written;
    }
    secret.len = (written >> 1) + (written & 1);
  } else {
    /* decode Byte32 key */
    secret.len = fio___otp_base32_decode(s.u8, sizeof(s), key);
    if (secret.len == (size_t)-1)
      goto cleanup;
  }

  /* compute HMAC (HOTP of T / TOTP)  */
  hash = fio_sha1_hmac(secret.buf, secret.len, &t, sizeof(t));

  /* dynamic truncation (RFC 4226, section 5.3) */
  {
    const size_t offset = (fio_sha1_digest(&hash)[fio_sha1_len() - 1] & 0x0F);
    r = fio_buf2u32_be(fio_sha1_digest(&hash) + offset) & 0x7FFFFFFFUL;
  }
  fio_secure_zero(&hash, sizeof(hash));

  /*  reduce number of digits */
  if (settings.digits < 10)
    r %= modulus[settings.digits];

cleanup:
  if (!settings.is_raw)
    fio_secure_zero(&s, sizeof(s));
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
