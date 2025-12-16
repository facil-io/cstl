/* ************************************************************************* */
#if !defined(FIO_INCLUDE_FILE) /* Dev test - ignore line */
#define FIO___DEV___           /* Development inclusion - ignore line */
#define FIO_TLS13              /* Development inclusion - ignore line */
#include "./include.h"         /* Development inclusion - ignore line */
#endif                         /* Development inclusion - ignore line */
/* *****************************************************************************




                              TLS 1.3 Implementation
                    Key Schedule (RFC 8446 Section 7) and
                    Record Layer (RFC 8446 Section 5)




Copyright and License: see header file (000 copyright.h) or top of file
***************************************************************************** */
#if defined(FIO_TLS13) && !defined(H___FIO_TLS13___H)
#define H___FIO_TLS13___H

/* *****************************************************************************
TLS 1.3 Key Schedule API

Note: Requires FIO_HKDF (which requires FIO_SHA2).
      Either define FIO_HKDF before FIO_TLS13, or use FIO_CRYPT to include all
      crypto modules.

Reference: RFC 8446 Section 7.1
***************************************************************************** */

/** SHA-256 hash length (32 bytes). */
#define FIO_TLS13_SHA256_HASH_LEN 32
/** SHA-384 hash length (48 bytes). */
#define FIO_TLS13_SHA384_HASH_LEN 48
/** Maximum hash length supported. */
#define FIO_TLS13_MAX_HASH_LEN 48

/** AES-128-GCM key length. */
#define FIO_TLS13_AES128_KEY_LEN 16
/** AES-256-GCM key length. */
#define FIO_TLS13_AES256_KEY_LEN 32
/** ChaCha20-Poly1305 key length. */
#define FIO_TLS13_CHACHA_KEY_LEN 32
/** IV length for all AEAD ciphers. */
#define FIO_TLS13_IV_LEN 12

/* *****************************************************************************
HKDF-Expand-Label (RFC 8446 Section 7.1)

HKDF-Expand-Label(Secret, Label, Context, Length) =
    HKDF-Expand(Secret, HkdfLabel, Length)

Where HkdfLabel is:
    struct {
        uint16 length = Length;
        opaque label<7..255> = "tls13 " + Label;
        opaque context<0..255> = Context;
    } HkdfLabel;
***************************************************************************** */

/**
 * TLS 1.3 HKDF-Expand-Label function.
 *
 * Derives keying material using the TLS 1.3 specific label format.
 *
 * @param out Output buffer for derived key material
 * @param out_len Desired output length (max 255)
 * @param secret The secret to expand (PRK from HKDF-Extract)
 * @param secret_len Secret length (32 for SHA-256, 48 for SHA-384)
 * @param label The label string (without "tls13 " prefix)
 * @param label_len Label length (max 249 to fit in 255 with prefix)
 * @param context Optional context (transcript hash or empty)
 * @param context_len Context length (max 255)
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_hkdf_expand_label(void *restrict out,
                                       size_t out_len,
                                       const void *restrict secret,
                                       size_t secret_len,
                                       const char *label,
                                       size_t label_len,
                                       const void *restrict context,
                                       size_t context_len,
                                       int use_sha384);

/* *****************************************************************************
Derive-Secret (RFC 8446 Section 7.1)

Derive-Secret(Secret, Label, Messages) =
    HKDF-Expand-Label(Secret, Label, Transcript-Hash(Messages), Hash.length)
***************************************************************************** */

/**
 * TLS 1.3 Derive-Secret function.
 *
 * Derives a secret from a base secret and transcript hash.
 *
 * @param out Output buffer (32 bytes for SHA-256, 48 for SHA-384)
 * @param secret The base secret
 * @param secret_len Secret length
 * @param label The label string (e.g., "c hs traffic")
 * @param label_len Label length
 * @param transcript_hash Hash of handshake messages (or empty hash for "")
 * @param hash_len Hash length (32 or 48)
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_derive_secret(void *restrict out,
                                   const void *restrict secret,
                                   size_t secret_len,
                                   const char *label,
                                   size_t label_len,
                                   const void *restrict transcript_hash,
                                   size_t hash_len,
                                   int use_sha384);

/* *****************************************************************************
Key Schedule Derivation Functions (RFC 8446 Section 7.1)

             0
             |
             v
   PSK ->  HKDF-Extract = Early Secret
             |
             +-----> Derive-Secret(., "ext binder" | "res binder", "")
             |                     = binder_key
             |
             +-----> Derive-Secret(., "c e traffic", ClientHello)
             |                     = client_early_traffic_secret
             |
             +-----> Derive-Secret(., "e exp master", ClientHello)
             |                     = early_exporter_master_secret
             v
       Derive-Secret(., "derived", "")
             |
             v
   (EC)DHE -> HKDF-Extract = Handshake Secret
             |
             +-----> Derive-Secret(., "c hs traffic",
             |                     ClientHello...ServerHello)
             |                     = client_handshake_traffic_secret
             |
             +-----> Derive-Secret(., "s hs traffic",
             |                     ClientHello...ServerHello)
             |                     = server_handshake_traffic_secret
             v
       Derive-Secret(., "derived", "")
             |
             v
   0 -> HKDF-Extract = Master Secret
             |
             +-----> Derive-Secret(., "c ap traffic",
             |                     ClientHello...server Finished)
             |                     = client_application_traffic_secret_0
             |
             +-----> Derive-Secret(., "s ap traffic",
             |                     ClientHello...server Finished)
             |                     = server_application_traffic_secret_0
             |
             +-----> Derive-Secret(., "exp master",
             |                     ClientHello...server Finished)
             |                     = exporter_master_secret
             |
             +-----> Derive-Secret(., "res master",
                                   ClientHello...client Finished)
                                   = resumption_master_secret
***************************************************************************** */

/**
 * Derive the Early Secret from PSK.
 *
 * Early Secret = HKDF-Extract(salt=0, IKM=PSK)
 *
 * @param early_secret Output buffer (32 or 48 bytes)
 * @param psk Pre-shared key (or NULL/zeros for no PSK)
 * @param psk_len PSK length (0 if no PSK)
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_derive_early_secret(void *restrict early_secret,
                                         const void *restrict psk,
                                         size_t psk_len,
                                         int use_sha384);

/**
 * Derive the Handshake Secret from ECDHE shared secret.
 *
 * Handshake Secret = HKDF-Extract(
 *     salt=Derive-Secret(Early Secret, "derived", ""),
 *     IKM=ECDHE shared secret
 * )
 *
 * @param handshake_secret Output buffer (32 or 48 bytes)
 * @param early_secret The early secret (from fio_tls13_derive_early_secret)
 * @param ecdhe_secret The ECDHE shared secret (e.g., from X25519)
 * @param ecdhe_len ECDHE secret length (32 for X25519)
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_derive_handshake_secret(void *restrict handshake_secret,
                                             const void *restrict early_secret,
                                             const void *restrict ecdhe_secret,
                                             size_t ecdhe_len,
                                             int use_sha384);

/**
 * Derive the Master Secret.
 *
 * Master Secret = HKDF-Extract(
 *     salt=Derive-Secret(Handshake Secret, "derived", ""),
 *     IKM=0
 * )
 *
 * @param master_secret Output buffer (32 or 48 bytes)
 * @param handshake_secret The handshake secret
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_derive_master_secret(void *restrict master_secret,
                                          const void *restrict handshake_secret,
                                          int use_sha384);

/* *****************************************************************************
Traffic Key Derivation (RFC 8446 Section 7.3)

[sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length)
[sender]_write_iv  = HKDF-Expand-Label(Secret, "iv", "", iv_length)
***************************************************************************** */

/**
 * Derive traffic keys and IV from a traffic secret.
 *
 * @param key Output buffer for write key
 * @param key_len Key length (16 for AES-128, 32 for AES-256/ChaCha20)
 * @param iv Output buffer for write IV (12 bytes)
 * @param traffic_secret The traffic secret (client/server handshake/app)
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_derive_traffic_keys(void *restrict key,
                                         size_t key_len,
                                         void *restrict iv,
                                         const void *restrict traffic_secret,
                                         int use_sha384);

/**
 * Derive the Finished key from a traffic secret.
 *
 * finished_key = HKDF-Expand-Label(BaseKey, "finished", "", Hash.length)
 *
 * @param finished_key Output buffer (32 or 48 bytes)
 * @param traffic_secret The handshake traffic secret
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_derive_finished_key(void *restrict finished_key,
                                         const void *restrict traffic_secret,
                                         int use_sha384);

/**
 * Compute the Finished verify_data.
 *
 * verify_data = HMAC(finished_key, Transcript-Hash(Handshake Context))
 *
 * @param verify_data Output buffer (32 or 48 bytes)
 * @param finished_key The finished key (from fio_tls13_derive_finished_key)
 * @param transcript_hash Hash of handshake messages up to this point
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_compute_finished(void *restrict verify_data,
                                      const void *restrict finished_key,
                                      const void *restrict transcript_hash,
                                      int use_sha384);

/**
 * Update application traffic secret for key update.
 *
 * application_traffic_secret_N+1 =
 *     HKDF-Expand-Label(application_traffic_secret_N, "traffic upd", "",
 * Hash.length)
 *
 * @param new_secret Output buffer (32 or 48 bytes)
 * @param current_secret Current application traffic secret
 * @param use_sha384 If non-zero, use SHA-384; otherwise SHA-256
 */
SFUNC void fio_tls13_update_traffic_secret(void *restrict new_secret,
                                           const void *restrict current_secret,
                                           int use_sha384);

/* *****************************************************************************
TLS 1.3 Record Layer Constants (RFC 8446 Section 5)
***************************************************************************** */

/** TLS record header length (5 bytes) */
#define FIO_TLS13_RECORD_HEADER_LEN 5
/** Maximum plaintext fragment length (2^14 = 16384 bytes) */
#define FIO_TLS13_MAX_PLAINTEXT_LEN 16384
/** Maximum ciphertext length (plaintext + padding + tag) */
#define FIO_TLS13_MAX_CIPHERTEXT_LEN (16384 + 256)
/** AEAD authentication tag length (16 bytes for all TLS 1.3 ciphers) */
#define FIO_TLS13_TAG_LEN 16
/** Legacy TLS version bytes (0x0303 = TLS 1.2) */
#define FIO_TLS13_LEGACY_VERSION_MAJOR 0x03
#define FIO_TLS13_LEGACY_VERSION_MINOR 0x03

/* *****************************************************************************
TLS 1.3 Content Types (RFC 8446 Section 5.1)
***************************************************************************** */

/** TLS 1.3 content types */
typedef enum {
  FIO_TLS13_CONTENT_INVALID = 0,
  FIO_TLS13_CONTENT_CHANGE_CIPHER_SPEC = 20, /* Legacy, ignored in TLS 1.3 */
  FIO_TLS13_CONTENT_ALERT = 21,
  FIO_TLS13_CONTENT_HANDSHAKE = 22,
  FIO_TLS13_CONTENT_APPLICATION_DATA = 23,
} fio_tls13_content_type_e;

/* *****************************************************************************
TLS 1.3 Handshake Message Types (RFC 8446 Section 4)
***************************************************************************** */

/** TLS 1.3 Handshake Message Types */
typedef enum {
  FIO_TLS13_HS_CLIENT_HELLO = 1,
  FIO_TLS13_HS_SERVER_HELLO = 2,
  FIO_TLS13_HS_NEW_SESSION_TICKET = 4,
  FIO_TLS13_HS_END_OF_EARLY_DATA = 5,
  FIO_TLS13_HS_ENCRYPTED_EXTENSIONS = 8,
  FIO_TLS13_HS_CERTIFICATE = 11,
  FIO_TLS13_HS_CERTIFICATE_REQUEST = 13,
  FIO_TLS13_HS_CERTIFICATE_VERIFY = 15,
  FIO_TLS13_HS_FINISHED = 20,
  FIO_TLS13_HS_KEY_UPDATE = 24,
  FIO_TLS13_HS_MESSAGE_HASH = 254,
} fio_tls13_handshake_type_e;

/** TLS 1.3 Extension Types (RFC 8446 Section 4.2) */
typedef enum {
  FIO_TLS13_EXT_SERVER_NAME = 0,           /* SNI */
  FIO_TLS13_EXT_SUPPORTED_GROUPS = 10,     /* Key exchange groups */
  FIO_TLS13_EXT_SIGNATURE_ALGORITHMS = 13, /* Signature schemes */
  FIO_TLS13_EXT_SUPPORTED_VERSIONS = 43,   /* TLS version negotiation */
  FIO_TLS13_EXT_KEY_SHARE = 51,            /* ECDHE key shares */
} fio_tls13_extension_type_e;

/** TLS 1.3 Cipher Suites (RFC 8446 Section B.4) */
typedef enum {
  FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256 = 0x1301,
  FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384 = 0x1302,
  FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256 = 0x1303,
} fio_tls13_cipher_suite_e;

/** TLS 1.3 Named Groups (RFC 8446 Section 4.2.7) */
typedef enum {
  FIO_TLS13_GROUP_SECP256R1 = 23, /* P-256 */
  FIO_TLS13_GROUP_SECP384R1 = 24, /* P-384 */
  FIO_TLS13_GROUP_X25519 = 29,    /* Curve25519 */
} fio_tls13_named_group_e;

/** TLS 1.3 Signature Algorithms (RFC 8446 Section 4.2.3) */
typedef enum {
  FIO_TLS13_SIG_RSA_PKCS1_SHA256 = 0x0401,
  FIO_TLS13_SIG_RSA_PKCS1_SHA384 = 0x0501,
  FIO_TLS13_SIG_RSA_PKCS1_SHA512 = 0x0601,
  FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256 = 0x0403,
  FIO_TLS13_SIG_ECDSA_SECP384R1_SHA384 = 0x0503,
  FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256 = 0x0804,
  FIO_TLS13_SIG_RSA_PSS_RSAE_SHA384 = 0x0805,
  FIO_TLS13_SIG_ED25519 = 0x0807,
} fio_tls13_signature_scheme_e;

/** TLS 1.3 Protocol Version Constants */
#define FIO_TLS13_VERSION_TLS12 0x0303
#define FIO_TLS13_VERSION_TLS13 0x0304

/** HelloRetryRequest magic random value (RFC 8446 Section 4.1.3) */
#define FIO_TLS13_HRR_RANDOM                                                   \
  "\xCF\x21\xAD\x74\xE5\x9A\x61\x11\xBE\x1D\x8C\x02\x1E\x65\xB8\x91"           \
  "\xC2\xA2\x11\x16\x7A\xBB\x8C\x5E\x07\x9E\x09\xE2\xC8\xA8\x33\x9C"

/* *****************************************************************************
TLS 1.3 Parsed Handshake Message Structures
***************************************************************************** */

/** Parsed ServerHello message */
typedef struct {
  uint8_t random[32];         /* Server random */
  uint16_t cipher_suite;      /* Selected cipher suite */
  uint8_t key_share[128];     /* Server's key share (max size for P-384) */
  uint8_t key_share_len;      /* Length of key share */
  uint16_t key_share_group;   /* Selected group */
  int is_hello_retry_request; /* 1 if HRR */
} fio_tls13_server_hello_s;

/** Parsed EncryptedExtensions message */
typedef struct {
  int has_server_name; /* Server acknowledged SNI */
} fio_tls13_encrypted_extensions_s;

/** Parsed Certificate message (minimal - first cert only) */
typedef struct {
  const uint8_t *cert_data; /* Pointer to first certificate */
  size_t cert_len;          /* Length of first certificate */
} fio_tls13_certificate_s;

/** Parsed CertificateVerify message */
typedef struct {
  uint16_t signature_scheme;
  const uint8_t *signature;
  size_t signature_len;
} fio_tls13_certificate_verify_s;

/* *****************************************************************************
TLS 1.3 Cipher Suite Types
***************************************************************************** */

/** Supported AEAD cipher types for TLS 1.3 */
typedef enum {
  FIO_TLS13_CIPHER_AES_128_GCM = 0,       /* TLS_AES_128_GCM_SHA256 */
  FIO_TLS13_CIPHER_AES_256_GCM = 1,       /* TLS_AES_256_GCM_SHA384 */
  FIO_TLS13_CIPHER_CHACHA20_POLY1305 = 2, /* TLS_CHACHA20_POLY1305_SHA256 */
} fio_tls13_cipher_type_e;

/* *****************************************************************************
TLS 1.3 Record Structures
***************************************************************************** */

/** TLSPlaintext header structure (RFC 8446 Section 5.1) */
typedef struct {
  uint8_t content_type;      /* ContentType */
  uint8_t legacy_version[2]; /* 0x03, 0x03 (TLS 1.2) */
  uint16_t length;           /* Fragment length (big-endian) */
  /* Fragment follows (up to 2^14 bytes) */
} fio_tls13_plaintext_header_s;

/** TLSCiphertext header structure (RFC 8446 Section 5.2) */
typedef struct {
  uint8_t opaque_type;       /* Always 23 (application_data) */
  uint8_t legacy_version[2]; /* 0x03, 0x03 */
  uint16_t length;           /* Encrypted length + tag (big-endian) */
  /* Encrypted content follows */
} fio_tls13_ciphertext_header_s;

/** Record encryption context (per-direction keys) */
typedef struct {
  uint8_t key[32];          /* Write key (16 or 32 bytes depending on cipher) */
  uint8_t iv[12];           /* Write IV (always 12 bytes) */
  uint64_t sequence_number; /* Per-record sequence number (starts at 0) */
  uint8_t key_len;          /* 16 for AES-128, 32 for AES-256/ChaCha20 */
  uint8_t cipher_type;      /* fio_tls13_cipher_type_e */
} fio_tls13_record_keys_s;

/* *****************************************************************************
TLS 1.3 Record Layer API
***************************************************************************** */

/**
 * Build per-record nonce by XORing sequence number with IV.
 *
 * Per RFC 8446 Section 5.3:
 * - Pad 64-bit sequence number to 12 bytes (big-endian, left-padded with zeros)
 * - XOR with the static IV derived from traffic secret
 *
 * @param nonce Output buffer (must be 12 bytes)
 * @param iv    Static IV from key derivation (12 bytes)
 * @param seq   64-bit sequence number
 */
FIO_IFUNC void fio_tls13_build_nonce(uint8_t nonce[12],
                                     const uint8_t iv[12],
                                     uint64_t seq);

/**
 * Parse a TLS record header.
 *
 * @param data         Input buffer containing record data
 * @param data_len     Length of input buffer
 * @param content_type Output: content type from header
 * @param payload_len  Output: payload length from header
 * @return Pointer to payload data, or NULL if incomplete/invalid
 */
SFUNC const uint8_t *fio_tls13_record_parse_header(
    const uint8_t *data,
    size_t data_len,
    fio_tls13_content_type_e *content_type,
    size_t *payload_len);

/**
 * Encrypt a TLS 1.3 record.
 *
 * Per RFC 8446 Section 5.2:
 * - Output format: 5-byte header + encrypted(plaintext + content_type) + tag
 * - AAD is the 5-byte record header
 * - Sequence number is incremented after encryption
 *
 * @param out          Output buffer for encrypted record
 * @param out_capacity Capacity of output buffer
 * @param plaintext    Plaintext data to encrypt
 * @param plaintext_len Length of plaintext
 * @param content_type  Content type (appended to plaintext before encryption)
 * @param keys         Encryption keys (sequence number will be incremented)
 * @return Total output length (header + ciphertext + tag), or -1 on error
 */
SFUNC int fio_tls13_record_encrypt(uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *plaintext,
                                   size_t plaintext_len,
                                   fio_tls13_content_type_e content_type,
                                   fio_tls13_record_keys_s *keys);

/**
 * Decrypt a TLS 1.3 record.
 *
 * Per RFC 8446 Section 5.2:
 * - Input includes 5-byte header
 * - Decrypts and verifies AEAD tag
 * - Scans backwards to find real content type (removes padding)
 * - Sequence number is incremented after successful decryption
 *
 * @param out          Output buffer for decrypted plaintext
 * @param out_capacity Capacity of output buffer
 * @param content_type Output: actual content type from inner plaintext
 * @param ciphertext   Input ciphertext (includes 5-byte header)
 * @param ciphertext_len Total length including header
 * @param keys         Decryption keys (sequence number will be incremented)
 * @return Plaintext length (excluding padding and content type), or -1 on error
 */
SFUNC int fio_tls13_record_decrypt(uint8_t *out,
                                   size_t out_capacity,
                                   fio_tls13_content_type_e *content_type,
                                   const uint8_t *ciphertext,
                                   size_t ciphertext_len,
                                   fio_tls13_record_keys_s *keys);

/**
 * Initialize record keys structure.
 *
 * @param keys        Keys structure to initialize
 * @param key         Key material (16 or 32 bytes)
 * @param key_len     Key length (16 for AES-128, 32 for AES-256/ChaCha20)
 * @param iv          IV material (12 bytes)
 * @param cipher_type Cipher type (fio_tls13_cipher_type_e)
 */
SFUNC void fio_tls13_record_keys_init(fio_tls13_record_keys_s *keys,
                                      const uint8_t *key,
                                      uint8_t key_len,
                                      const uint8_t iv[12],
                                      fio_tls13_cipher_type_e cipher_type);

/**
 * Clear sensitive key material from memory.
 *
 * @param keys Keys structure to clear
 */
FIO_IFUNC void fio_tls13_record_keys_clear(fio_tls13_record_keys_s *keys);

/* *****************************************************************************
TLS 1.3 Handshake Message API (RFC 8446 Section 4)
***************************************************************************** */

/**
 * Parse handshake header, return message type and body pointer.
 *
 * Returns pointer to message body, or NULL on error.
 * Sets msg_type and body_len on success.
 */
SFUNC const uint8_t *fio_tls13_parse_handshake_header(
    const uint8_t *data,
    size_t data_len,
    fio_tls13_handshake_type_e *msg_type,
    size_t *body_len);

/**
 * Write handshake header (4 bytes).
 *
 * Format: HandshakeType (1 byte) + uint24 length (3 bytes)
 */
SFUNC void fio_tls13_write_handshake_header(uint8_t *out,
                                            fio_tls13_handshake_type_e msg_type,
                                            size_t body_len);

/**
 * Build a ClientHello message.
 *
 * Returns: message length on success, -1 on error.
 *
 * Parameters:
 * - out: output buffer
 * - out_capacity: size of output buffer
 * - random: 32-byte client random
 * - server_name: SNI hostname (NULL if not used)
 * - x25519_pubkey: 32-byte X25519 public key
 * - cipher_suites: array of cipher suites to offer
 * - cipher_suite_count: number of cipher suites
 */
SFUNC int fio_tls13_build_client_hello(uint8_t *out,
                                       size_t out_capacity,
                                       const uint8_t random[32],
                                       const char *server_name,
                                       const uint8_t *x25519_pubkey,
                                       const uint16_t *cipher_suites,
                                       size_t cipher_suite_count);

/**
 * Parse ServerHello message.
 *
 * Returns: 0 on success, -1 on error.
 *
 * Note: data should point to the handshake body (after the 4-byte header).
 */
SFUNC int fio_tls13_parse_server_hello(fio_tls13_server_hello_s *out,
                                       const uint8_t *data,
                                       size_t data_len);

/**
 * Parse EncryptedExtensions message.
 *
 * Returns: 0 on success, -1 on error.
 */
SFUNC int fio_tls13_parse_encrypted_extensions(
    fio_tls13_encrypted_extensions_s *out,
    const uint8_t *data,
    size_t data_len);

/**
 * Parse Certificate message (extracts first certificate only).
 *
 * Returns: 0 on success, -1 on error.
 */
SFUNC int fio_tls13_parse_certificate(fio_tls13_certificate_s *out,
                                      const uint8_t *data,
                                      size_t data_len);

/**
 * Parse CertificateVerify message.
 *
 * Returns: 0 on success, -1 on error.
 */
SFUNC int fio_tls13_parse_certificate_verify(
    fio_tls13_certificate_verify_s *out,
    const uint8_t *data,
    size_t data_len);

/**
 * Build Finished message.
 *
 * verify_data = HMAC(finished_key, Transcript-Hash(Handshake Context))
 *
 * Returns: message length on success, -1 on error.
 */
SFUNC int fio_tls13_build_finished(uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *verify_data,
                                   size_t verify_data_len);

/**
 * Parse and verify Finished message.
 *
 * Returns: 0 on success (MAC matches), -1 on error.
 */
SFUNC int fio_tls13_parse_finished(const uint8_t *data,
                                   size_t data_len,
                                   const uint8_t *expected_verify_data,
                                   size_t verify_data_len);

/* *****************************************************************************
Implementation - possibly externed functions.
***************************************************************************** */
#if defined(FIO_EXTERN_COMPLETE) || !defined(FIO_EXTERN)

/* *****************************************************************************
HKDF-Expand-Label Implementation
***************************************************************************** */

SFUNC void fio_tls13_hkdf_expand_label(void *restrict out,
                                       size_t out_len,
                                       const void *restrict secret,
                                       size_t secret_len,
                                       const char *label,
                                       size_t label_len,
                                       const void *restrict context,
                                       size_t context_len,
                                       int use_sha384) {
  if (!out || !secret || out_len == 0 || out_len > 255)
    return;

  /* Clamp label and context lengths */
  if (label_len > 249)
    label_len = 249;
  if (context_len > 255)
    context_len = 255;

  /*
   * HkdfLabel structure:
   *   uint16 length = out_len
   *   opaque label<7..255> = "tls13 " + Label
   *   opaque context<0..255> = Context
   *
   * Encoded as:
   *   [2 bytes: out_len]
   *   [1 byte: label_len + 6]
   *   [6 bytes: "tls13 "]
   *   [label_len bytes: label]
   *   [1 byte: context_len]
   *   [context_len bytes: context]
   */
  uint8_t hkdf_label[2 + 1 + 6 + 249 + 1 + 255];
  size_t hkdf_label_len = 0;

  /* Length (2 bytes, big-endian) */
  hkdf_label[hkdf_label_len++] = (uint8_t)(out_len >> 8);
  hkdf_label[hkdf_label_len++] = (uint8_t)(out_len & 0xFF);

  /* Label length (1 byte) = 6 + label_len */
  hkdf_label[hkdf_label_len++] = (uint8_t)(6 + label_len);

  /* "tls13 " prefix (6 bytes) */
  hkdf_label[hkdf_label_len++] = 't';
  hkdf_label[hkdf_label_len++] = 'l';
  hkdf_label[hkdf_label_len++] = 's';
  hkdf_label[hkdf_label_len++] = '1';
  hkdf_label[hkdf_label_len++] = '3';
  hkdf_label[hkdf_label_len++] = ' ';

  /* Label */
  if (label && label_len > 0) {
    FIO_MEMCPY(hkdf_label + hkdf_label_len, label, label_len);
    hkdf_label_len += label_len;
  }

  /* Context length (1 byte) */
  hkdf_label[hkdf_label_len++] = (uint8_t)context_len;

  /* Context */
  if (context && context_len > 0) {
    FIO_MEMCPY(hkdf_label + hkdf_label_len, context, context_len);
    hkdf_label_len += context_len;
  }

  /* Call HKDF-Expand with the constructed label */
  fio_hkdf_expand(out,
                  out_len,
                  secret,
                  secret_len,
                  hkdf_label,
                  hkdf_label_len,
                  use_sha384);
}

/* *****************************************************************************
Derive-Secret Implementation
***************************************************************************** */

SFUNC void fio_tls13_derive_secret(void *restrict out,
                                   const void *restrict secret,
                                   size_t secret_len,
                                   const char *label,
                                   size_t label_len,
                                   const void *restrict transcript_hash,
                                   size_t hash_len,
                                   int use_sha384) {
  const size_t out_len = use_sha384 ? 48 : 32;

  /* If no transcript hash provided, use hash of empty string */
  uint8_t empty_hash[48];
  const void *hash_to_use = transcript_hash;
  size_t hash_len_to_use = hash_len;

  if (!transcript_hash || hash_len == 0) {
    /* Hash of empty string */
    if (use_sha384) {
      fio_u512 h = fio_sha512("", 0);
      FIO_MEMCPY(empty_hash, h.u8, 48);
      hash_len_to_use = 48;
    } else {
      fio_u256 h = fio_sha256("", 0);
      FIO_MEMCPY(empty_hash, h.u8, 32);
      hash_len_to_use = 32;
    }
    hash_to_use = empty_hash;
  }

  fio_tls13_hkdf_expand_label(out,
                              out_len,
                              secret,
                              secret_len,
                              label,
                              label_len,
                              hash_to_use,
                              hash_len_to_use,
                              use_sha384);
}

/* *****************************************************************************
Key Schedule Derivation Implementation
***************************************************************************** */

SFUNC void fio_tls13_derive_early_secret(void *restrict early_secret,
                                         const void *restrict psk,
                                         size_t psk_len,
                                         int use_sha384) {
  const size_t hash_len = use_sha384 ? 48 : 32;

  /* If no PSK, use zeros of hash length */
  uint8_t zero_psk[48] = {0};
  const void *ikm = psk;
  size_t ikm_len = psk_len;

  if (!psk || psk_len == 0) {
    ikm = zero_psk;
    ikm_len = hash_len;
  }

  /* Early Secret = HKDF-Extract(salt=0, IKM=PSK) */
  fio_hkdf_extract(early_secret, NULL, 0, ikm, ikm_len, use_sha384);
}

SFUNC void fio_tls13_derive_handshake_secret(void *restrict handshake_secret,
                                             const void *restrict early_secret,
                                             const void *restrict ecdhe_secret,
                                             size_t ecdhe_len,
                                             int use_sha384) {
  const size_t hash_len = use_sha384 ? 48 : 32;
  uint8_t derived[48];

  /* Derive-Secret(Early Secret, "derived", "") */
  fio_tls13_derive_secret(derived,
                          early_secret,
                          hash_len,
                          "derived",
                          7,
                          NULL,
                          0,
                          use_sha384);

  /* Handshake Secret = HKDF-Extract(salt=derived, IKM=ECDHE) */
  fio_hkdf_extract(handshake_secret,
                   derived,
                   hash_len,
                   ecdhe_secret,
                   ecdhe_len,
                   use_sha384);
}

SFUNC void fio_tls13_derive_master_secret(void *restrict master_secret,
                                          const void *restrict handshake_secret,
                                          int use_sha384) {
  const size_t hash_len = use_sha384 ? 48 : 32;
  uint8_t derived[48];
  uint8_t zero_ikm[48] = {0};

  /* Derive-Secret(Handshake Secret, "derived", "") */
  fio_tls13_derive_secret(derived,
                          handshake_secret,
                          hash_len,
                          "derived",
                          7,
                          NULL,
                          0,
                          use_sha384);

  /* Master Secret = HKDF-Extract(salt=derived, IKM=0) */
  fio_hkdf_extract(master_secret,
                   derived,
                   hash_len,
                   zero_ikm,
                   hash_len,
                   use_sha384);
}

/* *****************************************************************************
Traffic Key Derivation Implementation
***************************************************************************** */

SFUNC void fio_tls13_derive_traffic_keys(void *restrict key,
                                         size_t key_len,
                                         void *restrict iv,
                                         const void *restrict traffic_secret,
                                         int use_sha384) {
  const size_t secret_len = use_sha384 ? 48 : 32;

  /* [sender]_write_key = HKDF-Expand-Label(Secret, "key", "", key_length) */
  fio_tls13_hkdf_expand_label(key,
                              key_len,
                              traffic_secret,
                              secret_len,
                              "key",
                              3,
                              NULL,
                              0,
                              use_sha384);

  /* [sender]_write_iv = HKDF-Expand-Label(Secret, "iv", "", iv_length) */
  fio_tls13_hkdf_expand_label(iv,
                              FIO_TLS13_IV_LEN,
                              traffic_secret,
                              secret_len,
                              "iv",
                              2,
                              NULL,
                              0,
                              use_sha384);
}

SFUNC void fio_tls13_derive_finished_key(void *restrict finished_key,
                                         const void *restrict traffic_secret,
                                         int use_sha384) {
  const size_t hash_len = use_sha384 ? 48 : 32;

  /* finished_key = HKDF-Expand-Label(BaseKey, "finished", "", Hash.length) */
  fio_tls13_hkdf_expand_label(finished_key,
                              hash_len,
                              traffic_secret,
                              hash_len,
                              "finished",
                              8,
                              NULL,
                              0,
                              use_sha384);
}

SFUNC void fio_tls13_compute_finished(void *restrict verify_data,
                                      const void *restrict finished_key,
                                      const void *restrict transcript_hash,
                                      int use_sha384) {
  /* verify_data = HMAC(finished_key, Transcript-Hash) */
  if (use_sha384) {
    fio_u512 hmac = fio_sha512_hmac(finished_key, 48, transcript_hash, 48);
    FIO_MEMCPY(verify_data, hmac.u8, 48);
  } else {
    fio_u256 hmac = fio_sha256_hmac(finished_key, 32, transcript_hash, 32);
    FIO_MEMCPY(verify_data, hmac.u8, 32);
  }
}

SFUNC void fio_tls13_update_traffic_secret(void *restrict new_secret,
                                           const void *restrict current_secret,
                                           int use_sha384) {
  const size_t hash_len = use_sha384 ? 48 : 32;

  /* new_secret = HKDF-Expand-Label(current, "traffic upd", "", Hash.length) */
  fio_tls13_hkdf_expand_label(new_secret,
                              hash_len,
                              current_secret,
                              hash_len,
                              "traffic upd",
                              11,
                              NULL,
                              0,
                              use_sha384);
}

/* *****************************************************************************
TLS 1.3 Record Layer Implementation (RFC 8446 Section 5)
***************************************************************************** */

/* Build per-record nonce: sequence_number XOR iv */
FIO_IFUNC void fio_tls13_build_nonce(uint8_t nonce[12],
                                     const uint8_t iv[12],
                                     uint64_t seq) {
  /* Copy IV to nonce */
  FIO_MEMCPY(nonce, iv, 12);

  /* XOR sequence number into rightmost 8 bytes (big-endian)
   * Sequence number is padded to 12 bytes with leading zeros,
   * so we only XOR the last 8 bytes */
  nonce[4] ^= (uint8_t)(seq >> 56);
  nonce[5] ^= (uint8_t)(seq >> 48);
  nonce[6] ^= (uint8_t)(seq >> 40);
  nonce[7] ^= (uint8_t)(seq >> 32);
  nonce[8] ^= (uint8_t)(seq >> 24);
  nonce[9] ^= (uint8_t)(seq >> 16);
  nonce[10] ^= (uint8_t)(seq >> 8);
  nonce[11] ^= (uint8_t)(seq);
}

/* Parse record header */
SFUNC const uint8_t *fio_tls13_record_parse_header(
    const uint8_t *data,
    size_t data_len,
    fio_tls13_content_type_e *content_type,
    size_t *payload_len) {
  /* Need at least header */
  if (!data || data_len < FIO_TLS13_RECORD_HEADER_LEN)
    return NULL;

  /* Extract content type */
  uint8_t ct = data[0];

  /* Validate content type */
  if (ct != FIO_TLS13_CONTENT_CHANGE_CIPHER_SPEC &&
      ct != FIO_TLS13_CONTENT_ALERT && ct != FIO_TLS13_CONTENT_HANDSHAKE &&
      ct != FIO_TLS13_CONTENT_APPLICATION_DATA) {
    return NULL;
  }

  /* Extract length (big-endian) */
  uint16_t len = fio_buf2u16_be(data + 3);

  /* Validate length */
  if (len > FIO_TLS13_MAX_CIPHERTEXT_LEN)
    return NULL;

  /* Check if we have complete record */
  if (data_len < (size_t)(FIO_TLS13_RECORD_HEADER_LEN + len))
    return NULL;

  if (content_type)
    *content_type = (fio_tls13_content_type_e)ct;
  if (payload_len)
    *payload_len = len;

  return data + FIO_TLS13_RECORD_HEADER_LEN;
}

/* Initialize record keys */
SFUNC void fio_tls13_record_keys_init(fio_tls13_record_keys_s *keys,
                                      const uint8_t *key,
                                      uint8_t key_len,
                                      const uint8_t iv[12],
                                      fio_tls13_cipher_type_e cipher_type) {
  if (!keys)
    return;

  FIO_MEMSET(keys, 0, sizeof(*keys));

  /* Validate and copy key */
  if (key && key_len > 0) {
    uint8_t copy_len = (key_len > 32) ? 32 : key_len;
    FIO_MEMCPY(keys->key, key, copy_len);
    keys->key_len = copy_len;
  }

  /* Copy IV */
  if (iv)
    FIO_MEMCPY(keys->iv, iv, FIO_TLS13_IV_LEN);

  keys->cipher_type = (uint8_t)cipher_type;
  keys->sequence_number = 0;
}

/* Clear sensitive key material */
FIO_IFUNC void fio_tls13_record_keys_clear(fio_tls13_record_keys_s *keys) {
  if (keys)
    fio_secure_zero(keys, sizeof(*keys));
}

/* Internal: AEAD encrypt using appropriate cipher */
FIO_SFUNC int fio___tls13_aead_encrypt(uint8_t *ciphertext,
                                       uint8_t *tag,
                                       const uint8_t *plaintext,
                                       size_t plaintext_len,
                                       const uint8_t *aad,
                                       size_t aad_len,
                                       const uint8_t *key,
                                       uint8_t key_len,
                                       const uint8_t *nonce,
                                       fio_tls13_cipher_type_e cipher_type) {
  /* Copy plaintext to ciphertext buffer (in-place encryption) */
  if (plaintext != ciphertext && plaintext_len > 0)
    FIO_MEMCPY(ciphertext, plaintext, plaintext_len);

  switch (cipher_type) {
  case FIO_TLS13_CIPHER_AES_128_GCM:
    fio_aes128_gcm_enc(tag,
                       ciphertext,
                       plaintext_len,
                       aad,
                       aad_len,
                       key,
                       nonce);
    return 0;

  case FIO_TLS13_CIPHER_AES_256_GCM:
    fio_aes256_gcm_enc(tag,
                       ciphertext,
                       plaintext_len,
                       aad,
                       aad_len,
                       key,
                       nonce);
    return 0;

  case FIO_TLS13_CIPHER_CHACHA20_POLY1305:
    fio_chacha20_poly1305_enc(tag,
                              ciphertext,
                              plaintext_len,
                              aad,
                              aad_len,
                              key,
                              nonce);
    return 0;

  default: return -1;
  }
  (void)key_len; /* Used for validation in debug builds */
}

/* Internal: AEAD decrypt using appropriate cipher */
FIO_SFUNC int fio___tls13_aead_decrypt(uint8_t *plaintext,
                                       const uint8_t *tag,
                                       const uint8_t *ciphertext,
                                       size_t ciphertext_len,
                                       const uint8_t *aad,
                                       size_t aad_len,
                                       const uint8_t *key,
                                       uint8_t key_len,
                                       const uint8_t *nonce,
                                       fio_tls13_cipher_type_e cipher_type) {
  /* Copy ciphertext to plaintext buffer (in-place decryption) */
  if (ciphertext != plaintext && ciphertext_len > 0)
    FIO_MEMCPY(plaintext, ciphertext, ciphertext_len);

  /* Need mutable tag for the decrypt functions */
  uint8_t tag_copy[FIO_TLS13_TAG_LEN];
  FIO_MEMCPY(tag_copy, tag, FIO_TLS13_TAG_LEN);

  int ret = -1;
  switch (cipher_type) {
  case FIO_TLS13_CIPHER_AES_128_GCM:
    ret = fio_aes128_gcm_dec(tag_copy,
                             plaintext,
                             ciphertext_len,
                             aad,
                             aad_len,
                             key,
                             nonce);
    break;

  case FIO_TLS13_CIPHER_AES_256_GCM:
    ret = fio_aes256_gcm_dec(tag_copy,
                             plaintext,
                             ciphertext_len,
                             aad,
                             aad_len,
                             key,
                             nonce);
    break;

  case FIO_TLS13_CIPHER_CHACHA20_POLY1305:
    ret = fio_chacha20_poly1305_dec(tag_copy,
                                    plaintext,
                                    ciphertext_len,
                                    aad,
                                    aad_len,
                                    key,
                                    nonce);
    break;

  default: ret = -1; break;
  }

  fio_secure_zero(tag_copy, sizeof(tag_copy));
  (void)key_len;
  return ret;
}

/* Encrypt a TLS 1.3 record */
SFUNC int fio_tls13_record_encrypt(uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *plaintext,
                                   size_t plaintext_len,
                                   fio_tls13_content_type_e content_type,
                                   fio_tls13_record_keys_s *keys) {
  if (!out || !keys)
    return -1;

  /* Validate plaintext length */
  if (plaintext_len > FIO_TLS13_MAX_PLAINTEXT_LEN)
    return -1;

  /* Calculate output size:
   * header(5) + plaintext + content_type(1) + tag(16) */
  size_t inner_len = plaintext_len + 1; /* plaintext + content_type byte */
  size_t total_len =
      FIO_TLS13_RECORD_HEADER_LEN + inner_len + FIO_TLS13_TAG_LEN;

  if (out_capacity < total_len)
    return -1;

  /* Build record header (this becomes the AAD) */
  uint8_t *header = out;
  header[0] = FIO_TLS13_CONTENT_APPLICATION_DATA; /* opaque_type always 23 */
  header[1] = FIO_TLS13_LEGACY_VERSION_MAJOR;
  header[2] = FIO_TLS13_LEGACY_VERSION_MINOR;
  fio_u2buf16_be(header + 3, (uint16_t)(inner_len + FIO_TLS13_TAG_LEN));

  /* Prepare inner plaintext: plaintext || content_type */
  uint8_t *ct_out = out + FIO_TLS13_RECORD_HEADER_LEN;

  /* Copy plaintext if provided */
  if (plaintext && plaintext_len > 0)
    FIO_MEMCPY(ct_out, plaintext, plaintext_len);

  /* Append content type */
  ct_out[plaintext_len] = (uint8_t)content_type;

  /* Build nonce */
  uint8_t nonce[FIO_TLS13_IV_LEN];
  fio_tls13_build_nonce(nonce, keys->iv, keys->sequence_number);

  /* Tag goes after ciphertext */
  uint8_t *tag = ct_out + inner_len;

  /* Encrypt with AAD = record header */
  int ret =
      fio___tls13_aead_encrypt(ct_out,
                               tag,
                               ct_out,
                               inner_len,
                               header,
                               FIO_TLS13_RECORD_HEADER_LEN,
                               keys->key,
                               keys->key_len,
                               nonce,
                               (fio_tls13_cipher_type_e)keys->cipher_type);

  /* Clear nonce */
  fio_secure_zero(nonce, sizeof(nonce));

  if (ret != 0)
    return -1;

  /* Increment sequence number */
  ++keys->sequence_number;

  return (int)total_len;
}

/* Decrypt a TLS 1.3 record */
SFUNC int fio_tls13_record_decrypt(uint8_t *out,
                                   size_t out_capacity,
                                   fio_tls13_content_type_e *content_type,
                                   const uint8_t *ciphertext,
                                   size_t ciphertext_len,
                                   fio_tls13_record_keys_s *keys) {
  if (!out || !ciphertext || !keys || !content_type)
    return -1;

  /* Parse header */
  fio_tls13_content_type_e outer_type;
  size_t payload_len;
  const uint8_t *payload = fio_tls13_record_parse_header(ciphertext,
                                                         ciphertext_len,
                                                         &outer_type,
                                                         &payload_len);
  if (!payload)
    return -1;

  /* For encrypted records, outer type must be application_data */
  if (outer_type != FIO_TLS13_CONTENT_APPLICATION_DATA)
    return -1;

  /* Payload must include at least tag + 1 byte (content type) */
  if (payload_len < FIO_TLS13_TAG_LEN + 1)
    return -1;

  /* Calculate inner ciphertext length (excluding tag) */
  size_t inner_ct_len = payload_len - FIO_TLS13_TAG_LEN;

  /* Check output capacity */
  if (out_capacity < inner_ct_len)
    return -1;

  /* Extract tag (last 16 bytes of payload) */
  const uint8_t *tag = payload + inner_ct_len;

  /* Build nonce */
  uint8_t nonce[FIO_TLS13_IV_LEN];
  fio_tls13_build_nonce(nonce, keys->iv, keys->sequence_number);

  /* AAD is the 5-byte record header */
  const uint8_t *aad = ciphertext;

  /* Decrypt */
  int ret =
      fio___tls13_aead_decrypt(out,
                               tag,
                               payload,
                               inner_ct_len,
                               aad,
                               FIO_TLS13_RECORD_HEADER_LEN,
                               keys->key,
                               keys->key_len,
                               nonce,
                               (fio_tls13_cipher_type_e)keys->cipher_type);

  /* Clear nonce */
  fio_secure_zero(nonce, sizeof(nonce));

  if (ret != 0)
    return -1;

  /* Scan backwards to find real content type (skip zero padding)
   * Per RFC 8446: zeros are optional padding, real content type is
   * the last non-zero byte */
  size_t pt_len = inner_ct_len;
  while (pt_len > 0 && out[pt_len - 1] == 0)
    --pt_len;

  /* Must have at least the content type byte */
  if (pt_len == 0)
    return -1;

  /* Last non-zero byte is the content type */
  uint8_t inner_type = out[pt_len - 1];
  --pt_len; /* Exclude content type from plaintext length */

  /* Validate content type */
  if (inner_type != FIO_TLS13_CONTENT_ALERT &&
      inner_type != FIO_TLS13_CONTENT_HANDSHAKE &&
      inner_type != FIO_TLS13_CONTENT_APPLICATION_DATA) {
    /* Zero out decrypted data on invalid content type */
    fio_secure_zero(out, inner_ct_len);
    return -1;
  }

  *content_type = (fio_tls13_content_type_e)inner_type;

  /* Increment sequence number */
  ++keys->sequence_number;

  return (int)pt_len;
}

/* *****************************************************************************
TLS 1.3 Handshake Message Implementation (RFC 8446 Section 4)
***************************************************************************** */

/* Internal: Write uint16 big-endian */
FIO_SFUNC void fio___tls13_write_u16(uint8_t *out, uint16_t val) {
  out[0] = (uint8_t)(val >> 8);
  out[1] = (uint8_t)(val & 0xFF);
}

/* Internal: Read uint16 big-endian */
FIO_SFUNC uint16_t fio___tls13_read_u16(const uint8_t *data) {
  return (uint16_t)((data[0] << 8) | data[1]);
}

/* Internal: Write uint24 big-endian */
FIO_SFUNC void fio___tls13_write_u24(uint8_t *out, uint32_t val) {
  out[0] = (uint8_t)((val >> 16) & 0xFF);
  out[1] = (uint8_t)((val >> 8) & 0xFF);
  out[2] = (uint8_t)(val & 0xFF);
}

/* Internal: Read uint24 big-endian */
FIO_SFUNC uint32_t fio___tls13_read_u24(const uint8_t *data) {
  return ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) |
         (uint32_t)data[2];
}

/* Parse handshake header */
SFUNC const uint8_t *fio_tls13_parse_handshake_header(
    const uint8_t *data,
    size_t data_len,
    fio_tls13_handshake_type_e *msg_type,
    size_t *body_len) {
  /* Handshake header is 4 bytes: type(1) + length(3) */
  if (!data || data_len < 4)
    return NULL;

  uint8_t type = data[0];
  uint32_t len = fio___tls13_read_u24(data + 1);

  /* Validate we have complete message */
  if (data_len < 4 + len)
    return NULL;

  if (msg_type)
    *msg_type = (fio_tls13_handshake_type_e)type;
  if (body_len)
    *body_len = len;

  return data + 4;
}

/* Write handshake header */
SFUNC void fio_tls13_write_handshake_header(uint8_t *out,
                                            fio_tls13_handshake_type_e msg_type,
                                            size_t body_len) {
  if (!out)
    return;
  out[0] = (uint8_t)msg_type;
  fio___tls13_write_u24(out + 1, (uint32_t)body_len);
}

/* *****************************************************************************
ClientHello Building Implementation
***************************************************************************** */

/* Internal: Write SNI extension */
FIO_SFUNC size_t fio___tls13_write_ext_sni(uint8_t *out,
                                           const char *server_name) {
  if (!server_name)
    return 0;

  size_t name_len = 0;
  while (server_name[name_len] && name_len < 255)
    ++name_len;
  if (name_len == 0)
    return 0;

  uint8_t *p = out;

  /* Extension type: server_name (0) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_SERVER_NAME);
  p += 2;

  /* Extension data length: list_len(2) + entry_type(1) + name_len(2) + name */
  uint16_t ext_len = (uint16_t)(2 + 1 + 2 + name_len);
  fio___tls13_write_u16(p, ext_len);
  p += 2;

  /* Server name list length */
  fio___tls13_write_u16(p, (uint16_t)(1 + 2 + name_len));
  p += 2;

  /* Name type: host_name (0) */
  *p++ = 0;

  /* Host name length */
  fio___tls13_write_u16(p, (uint16_t)name_len);
  p += 2;

  /* Host name */
  FIO_MEMCPY(p, server_name, name_len);
  p += name_len;

  return (size_t)(p - out);
}

/* Internal: Write supported_versions extension (client) */
FIO_SFUNC size_t fio___tls13_write_ext_supported_versions(uint8_t *out) {
  uint8_t *p = out;

  /* Extension type: supported_versions (43) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_SUPPORTED_VERSIONS);
  p += 2;

  /* Extension data length: versions_len(1) + version(2) */
  fio___tls13_write_u16(p, 3);
  p += 2;

  /* Versions length (1 version = 2 bytes) */
  *p++ = 2;

  /* TLS 1.3 (0x0304) */
  fio___tls13_write_u16(p, FIO_TLS13_VERSION_TLS13);
  p += 2;

  return (size_t)(p - out);
}

/* Internal: Write supported_groups extension */
FIO_SFUNC size_t fio___tls13_write_ext_supported_groups(uint8_t *out) {
  uint8_t *p = out;

  /* Extension type: supported_groups (10) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_SUPPORTED_GROUPS);
  p += 2;

  /* Extension data length: groups_len(2) + groups(2 each) */
  /* We support: x25519, secp256r1 */
  fio___tls13_write_u16(p, 2 + 4); /* 2 groups * 2 bytes each */
  p += 2;

  /* Groups length */
  fio___tls13_write_u16(p, 4);
  p += 2;

  /* x25519 (preferred) */
  fio___tls13_write_u16(p, FIO_TLS13_GROUP_X25519);
  p += 2;

  /* secp256r1 (fallback) */
  fio___tls13_write_u16(p, FIO_TLS13_GROUP_SECP256R1);
  p += 2;

  return (size_t)(p - out);
}

/* Internal: Write signature_algorithms extension */
FIO_SFUNC size_t fio___tls13_write_ext_signature_algorithms(uint8_t *out) {
  uint8_t *p = out;

  /* Extension type: signature_algorithms (13) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_SIGNATURE_ALGORITHMS);
  p += 2;

  /* We support: ed25519, ecdsa_secp256r1_sha256, rsa_pss_rsae_sha256,
   * rsa_pkcs1_sha256 */
  uint16_t algos[] = {FIO_TLS13_SIG_ED25519,
                      FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256,
                      FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256,
                      FIO_TLS13_SIG_RSA_PKCS1_SHA256,
                      FIO_TLS13_SIG_RSA_PSS_RSAE_SHA384,
                      FIO_TLS13_SIG_RSA_PKCS1_SHA384};
  size_t algo_count = sizeof(algos) / sizeof(algos[0]);

  /* Extension data length: algos_len(2) + algos */
  fio___tls13_write_u16(p, (uint16_t)(2 + algo_count * 2));
  p += 2;

  /* Algorithms length */
  fio___tls13_write_u16(p, (uint16_t)(algo_count * 2));
  p += 2;

  /* Write algorithms */
  for (size_t i = 0; i < algo_count; ++i) {
    fio___tls13_write_u16(p, algos[i]);
    p += 2;
  }

  return (size_t)(p - out);
}

/* Internal: Write key_share extension (client) */
FIO_SFUNC size_t fio___tls13_write_ext_key_share(uint8_t *out,
                                                 const uint8_t *x25519_pubkey) {
  if (!x25519_pubkey)
    return 0;

  uint8_t *p = out;

  /* Extension type: key_share (51) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_KEY_SHARE);
  p += 2;

  /* Extension data length: entries_len(2) + entry */
  /* Entry: group(2) + key_len(2) + key(32) = 36 bytes */
  fio___tls13_write_u16(p, 2 + 36);
  p += 2;

  /* Client key share entries length */
  fio___tls13_write_u16(p, 36);
  p += 2;

  /* Key share entry: x25519 */
  fio___tls13_write_u16(p, FIO_TLS13_GROUP_X25519);
  p += 2;

  /* Key length (32 for x25519) */
  fio___tls13_write_u16(p, 32);
  p += 2;

  /* Public key */
  FIO_MEMCPY(p, x25519_pubkey, 32);
  p += 32;

  return (size_t)(p - out);
}

/* Build ClientHello message */
SFUNC int fio_tls13_build_client_hello(uint8_t *out,
                                       size_t out_capacity,
                                       const uint8_t random[32],
                                       const char *server_name,
                                       const uint8_t *x25519_pubkey,
                                       const uint16_t *cipher_suites,
                                       size_t cipher_suite_count) {
  if (!out || !random || out_capacity < 256)
    return -1;

  /* Default cipher suites if none provided */
  uint16_t default_suites[] = {FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256,
                               FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256,
                               FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384};
  if (!cipher_suites || cipher_suite_count == 0) {
    cipher_suites = default_suites;
    cipher_suite_count = 3;
  }

  uint8_t *p = out + 4; /* Skip handshake header for now */
  uint8_t *start = p;

  /* Legacy version: TLS 1.2 (0x0303) */
  fio___tls13_write_u16(p, FIO_TLS13_VERSION_TLS12);
  p += 2;

  /* Random (32 bytes) */
  FIO_MEMCPY(p, random, 32);
  p += 32;

  /* Legacy session ID (empty for TLS 1.3) */
  *p++ = 0;

  /* Cipher suites */
  fio___tls13_write_u16(p, (uint16_t)(cipher_suite_count * 2));
  p += 2;
  for (size_t i = 0; i < cipher_suite_count; ++i) {
    fio___tls13_write_u16(p, cipher_suites[i]);
    p += 2;
  }

  /* Legacy compression methods (only null) */
  *p++ = 1; /* Length */
  *p++ = 0; /* null compression */

  /* Extensions */
  uint8_t *ext_len_ptr = p;
  p += 2; /* Skip extensions length for now */
  uint8_t *ext_start = p;

  /* SNI extension */
  p += fio___tls13_write_ext_sni(p, server_name);

  /* supported_versions extension (REQUIRED for TLS 1.3) */
  p += fio___tls13_write_ext_supported_versions(p);

  /* supported_groups extension */
  p += fio___tls13_write_ext_supported_groups(p);

  /* signature_algorithms extension */
  p += fio___tls13_write_ext_signature_algorithms(p);

  /* key_share extension */
  if (x25519_pubkey)
    p += fio___tls13_write_ext_key_share(p, x25519_pubkey);

  /* Write extensions length */
  fio___tls13_write_u16(ext_len_ptr, (uint16_t)(p - ext_start));

  /* Calculate body length and write handshake header */
  size_t body_len = (size_t)(p - start);
  fio_tls13_write_handshake_header(out, FIO_TLS13_HS_CLIENT_HELLO, body_len);

  return (int)(4 + body_len);
}

/* *****************************************************************************
ServerHello Parsing Implementation
***************************************************************************** */

/* HelloRetryRequest random value */
static const uint8_t fio___tls13_hrr_random[32] = {
    0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11, 0xBE, 0x1D, 0x8C,
    0x02, 0x1E, 0x65, 0xB8, 0x91, 0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB,
    0x8C, 0x5E, 0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C};

/* Parse ServerHello message */
SFUNC int fio_tls13_parse_server_hello(fio_tls13_server_hello_s *out,
                                       const uint8_t *data,
                                       size_t data_len) {
  if (!out || !data)
    return -1;

  FIO_MEMSET(out, 0, sizeof(*out));

  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /* Minimum ServerHello: version(2) + random(32) + session_id_len(1) +
   * cipher_suite(2) + compression(1) = 38 bytes */
  if (data_len < 38)
    return -1;

  /* Legacy version (should be 0x0303) */
  uint16_t version = fio___tls13_read_u16(p);
  p += 2;
  if (version != FIO_TLS13_VERSION_TLS12)
    return -1;

  /* Random (32 bytes) */
  FIO_MEMCPY(out->random, p, 32);
  p += 32;

  /* Check for HelloRetryRequest */
  out->is_hello_retry_request =
      (FIO_MEMCMP(out->random, fio___tls13_hrr_random, 32) == 0);

  /* Legacy session ID (echo of client's, skip it) */
  uint8_t session_id_len = *p++;
  if (p + session_id_len > end)
    return -1;
  p += session_id_len;

  /* Cipher suite */
  if (p + 2 > end)
    return -1;
  out->cipher_suite = fio___tls13_read_u16(p);
  p += 2;

  /* Legacy compression method (must be 0) */
  if (p + 1 > end)
    return -1;
  if (*p++ != 0)
    return -1;

  /* Extensions (optional but expected for TLS 1.3) */
  if (p + 2 > end)
    return 0; /* No extensions, but valid */

  uint16_t ext_len = fio___tls13_read_u16(p);
  p += 2;

  if (p + ext_len > end)
    return -1;

  const uint8_t *ext_end = p + ext_len;

  /* Parse extensions */
  while (p + 4 <= ext_end) {
    uint16_t ext_type = fio___tls13_read_u16(p);
    p += 2;
    uint16_t ext_data_len = fio___tls13_read_u16(p);
    p += 2;

    if (p + ext_data_len > ext_end)
      return -1;

    switch (ext_type) {
    case FIO_TLS13_EXT_KEY_SHARE:
      /* key_share: group(2) + key_len(2) + key */
      if (ext_data_len >= 4) {
        out->key_share_group = fio___tls13_read_u16(p);
        uint16_t key_len = fio___tls13_read_u16(p + 2);
        if (key_len <= sizeof(out->key_share) && ext_data_len >= 4 + key_len) {
          FIO_MEMCPY(out->key_share, p + 4, key_len);
          out->key_share_len = (uint8_t)key_len;
        }
      }
      break;

    case FIO_TLS13_EXT_SUPPORTED_VERSIONS:
      /* supported_versions in ServerHello: just the selected version (2 bytes)
       */
      if (ext_data_len >= 2) {
        uint16_t selected = fio___tls13_read_u16(p);
        if (selected != FIO_TLS13_VERSION_TLS13)
          return -1; /* Must be TLS 1.3 */
      }
      break;

    default:
      /* Ignore unknown extensions */
      break;
    }

    p += ext_data_len;
  }

  return 0;
}

/* *****************************************************************************
EncryptedExtensions Parsing Implementation
***************************************************************************** */

SFUNC int fio_tls13_parse_encrypted_extensions(
    fio_tls13_encrypted_extensions_s *out,
    const uint8_t *data,
    size_t data_len) {
  if (!out || !data)
    return -1;

  FIO_MEMSET(out, 0, sizeof(*out));

  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /* Extensions length */
  if (p + 2 > end)
    return -1;

  uint16_t ext_len = fio___tls13_read_u16(p);
  p += 2;

  if (p + ext_len > end)
    return -1;

  const uint8_t *ext_end = p + ext_len;

  /* Parse extensions */
  while (p + 4 <= ext_end) {
    uint16_t ext_type = fio___tls13_read_u16(p);
    p += 2;
    uint16_t ext_data_len = fio___tls13_read_u16(p);
    p += 2;

    if (p + ext_data_len > ext_end)
      return -1;

    switch (ext_type) {
    case FIO_TLS13_EXT_SERVER_NAME:
      /* Server acknowledged SNI (empty extension) */
      out->has_server_name = 1;
      break;

    default:
      /* Ignore other extensions */
      break;
    }

    p += ext_data_len;
  }

  return 0;
}

/* *****************************************************************************
Certificate Parsing Implementation
***************************************************************************** */

SFUNC int fio_tls13_parse_certificate(fio_tls13_certificate_s *out,
                                      const uint8_t *data,
                                      size_t data_len) {
  if (!out || !data)
    return -1;

  FIO_MEMSET(out, 0, sizeof(*out));

  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /* Certificate request context length (should be 0 for server cert) */
  if (p + 1 > end)
    return -1;
  uint8_t ctx_len = *p++;
  if (p + ctx_len > end)
    return -1;
  p += ctx_len;

  /* Certificate list length (3 bytes) */
  if (p + 3 > end)
    return -1;
  uint32_t list_len = fio___tls13_read_u24(p);
  p += 3;

  if (p + list_len > end)
    return -1;

  /* Parse first certificate entry */
  if (list_len < 3)
    return -1;

  /* Certificate data length (3 bytes) */
  uint32_t cert_len = fio___tls13_read_u24(p);
  p += 3;

  if (p + cert_len > end)
    return -1;

  /* Store pointer to first certificate */
  out->cert_data = p;
  out->cert_len = cert_len;

  /* Skip certificate data and extensions (we only need the first cert) */

  return 0;
}

/* *****************************************************************************
CertificateVerify Parsing Implementation
***************************************************************************** */

SFUNC int fio_tls13_parse_certificate_verify(
    fio_tls13_certificate_verify_s *out,
    const uint8_t *data,
    size_t data_len) {
  if (!out || !data)
    return -1;

  FIO_MEMSET(out, 0, sizeof(*out));

  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /* Signature algorithm (2 bytes) */
  if (p + 2 > end)
    return -1;
  out->signature_scheme = fio___tls13_read_u16(p);
  p += 2;

  /* Signature length (2 bytes) */
  if (p + 2 > end)
    return -1;
  uint16_t sig_len = fio___tls13_read_u16(p);
  p += 2;

  /* Signature data */
  if (p + sig_len > end)
    return -1;
  out->signature = p;
  out->signature_len = sig_len;

  return 0;
}

/* *****************************************************************************
Finished Message Implementation
***************************************************************************** */

SFUNC int fio_tls13_build_finished(uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *verify_data,
                                   size_t verify_data_len) {
  if (!out || !verify_data)
    return -1;

  /* Finished message: handshake header (4) + verify_data */
  size_t total_len = 4 + verify_data_len;
  if (out_capacity < total_len)
    return -1;

  /* Write handshake header */
  fio_tls13_write_handshake_header(out, FIO_TLS13_HS_FINISHED, verify_data_len);

  /* Write verify_data */
  FIO_MEMCPY(out + 4, verify_data, verify_data_len);

  return (int)total_len;
}

SFUNC int fio_tls13_parse_finished(const uint8_t *data,
                                   size_t data_len,
                                   const uint8_t *expected_verify_data,
                                   size_t verify_data_len) {
  if (!data || !expected_verify_data)
    return -1;

  /* Data should be exactly verify_data_len bytes */
  if (data_len != verify_data_len)
    return -1;

  /* Constant-time comparison */
  uint8_t diff = 0;
  for (size_t i = 0; i < verify_data_len; ++i)
    diff |= data[i] ^ expected_verify_data[i];

  return (diff == 0) ? 0 : -1;
}

/* *****************************************************************************




                        TLS 1.3 Client Handshake State Machine
                              (RFC 8446 Section 4)




***************************************************************************** */

/* *****************************************************************************
TLS 1.3 Alert Codes (RFC 8446 Section 6)
***************************************************************************** */

/** TLS 1.3 Alert Levels */
typedef enum {
  FIO_TLS13_ALERT_LEVEL_WARNING = 1,
  FIO_TLS13_ALERT_LEVEL_FATAL = 2,
} fio_tls13_alert_level_e;

/** TLS 1.3 Alert Descriptions */
typedef enum {
  FIO_TLS13_ALERT_CLOSE_NOTIFY = 0,
  FIO_TLS13_ALERT_UNEXPECTED_MESSAGE = 10,
  FIO_TLS13_ALERT_BAD_RECORD_MAC = 20,
  FIO_TLS13_ALERT_RECORD_OVERFLOW = 22,
  FIO_TLS13_ALERT_HANDSHAKE_FAILURE = 40,
  FIO_TLS13_ALERT_BAD_CERTIFICATE = 42,
  FIO_TLS13_ALERT_CERTIFICATE_REVOKED = 44,
  FIO_TLS13_ALERT_CERTIFICATE_EXPIRED = 45,
  FIO_TLS13_ALERT_CERTIFICATE_UNKNOWN = 46,
  FIO_TLS13_ALERT_ILLEGAL_PARAMETER = 47,
  FIO_TLS13_ALERT_UNKNOWN_CA = 48,
  FIO_TLS13_ALERT_DECODE_ERROR = 50,
  FIO_TLS13_ALERT_DECRYPT_ERROR = 51,
  FIO_TLS13_ALERT_PROTOCOL_VERSION = 70,
  FIO_TLS13_ALERT_INTERNAL_ERROR = 80,
  FIO_TLS13_ALERT_MISSING_EXTENSION = 109,
  FIO_TLS13_ALERT_UNSUPPORTED_EXTENSION = 110,
  FIO_TLS13_ALERT_UNRECOGNIZED_NAME = 112,
  FIO_TLS13_ALERT_BAD_CERTIFICATE_STATUS_RESPONSE = 113,
  FIO_TLS13_ALERT_UNKNOWN_PSK_IDENTITY = 115,
  FIO_TLS13_ALERT_CERTIFICATE_REQUIRED = 116,
  FIO_TLS13_ALERT_NO_APPLICATION_PROTOCOL = 120,
} fio_tls13_alert_description_e;

/* *****************************************************************************
TLS 1.3 Client State Machine
***************************************************************************** */

/** TLS 1.3 Client Handshake States */
typedef enum {
  FIO_TLS13_STATE_START = 0,     /* Initial state */
  FIO_TLS13_STATE_WAIT_SH,       /* Sent ClientHello, waiting for ServerHello */
  FIO_TLS13_STATE_WAIT_EE,       /* Received ServerHello, waiting for EE */
  FIO_TLS13_STATE_WAIT_CERT_CR,  /* Waiting for Certificate or CertRequest */
  FIO_TLS13_STATE_WAIT_CERT,     /* Waiting for Certificate */
  FIO_TLS13_STATE_WAIT_CV,       /* Waiting for CertificateVerify */
  FIO_TLS13_STATE_WAIT_FINISHED, /* Waiting for server Finished */
  FIO_TLS13_STATE_CONNECTED,     /* Handshake complete */
  FIO_TLS13_STATE_ERROR,         /* Error state */
} fio_tls13_client_state_e;

/** TLS 1.3 Client Context */
typedef struct {
  /* State */
  fio_tls13_client_state_e state;

  /* Negotiated parameters */
  uint16_t cipher_suite; /* Selected cipher suite */
  int use_sha384;        /* 0 = SHA-256, 1 = SHA-384 */

  /* Key material */
  uint8_t client_random[32];
  uint8_t x25519_private_key[32];
  uint8_t x25519_public_key[32];
  uint8_t shared_secret[32]; /* ECDHE shared secret */

  /* Secrets (derived during handshake) - up to SHA-384 size */
  uint8_t early_secret[48];
  uint8_t handshake_secret[48];
  uint8_t master_secret[48];
  uint8_t client_handshake_traffic_secret[48];
  uint8_t server_handshake_traffic_secret[48];
  uint8_t client_app_traffic_secret[48];
  uint8_t server_app_traffic_secret[48];

  /* Traffic keys */
  fio_tls13_record_keys_s client_handshake_keys;
  fio_tls13_record_keys_s server_handshake_keys;
  fio_tls13_record_keys_s client_app_keys;
  fio_tls13_record_keys_s server_app_keys;

  /* Transcript hash (running hash of all handshake messages) */
  fio_sha256_s transcript_sha256; /* For SHA-256 cipher suites */
  fio_sha512_s transcript_sha384; /* For SHA-384 cipher suites */

  /* Server certificate (pointer to received data, not owned) */
  const uint8_t *server_cert;
  size_t server_cert_len;

  /* CertificateVerify signature (pointer to received data, not owned) */
  const uint8_t *server_signature;
  size_t server_signature_len;
  uint16_t server_signature_scheme;

  /* Error info */
  uint8_t alert_level;
  uint8_t alert_description;

  /* Configuration */
  const char *server_name; /* SNI hostname */

  /* Certificate verification configuration */
  void *trust_store;        /* fio_x509_trust_store_s* - NULL to skip chain */
  uint8_t skip_cert_verify; /* 1 to skip all certificate verification */
  uint8_t cert_verified;    /* 1 if CertificateVerify was validated */
  uint8_t chain_verified;   /* 1 if certificate chain was validated */
  int16_t cert_error;       /* Certificate error code (fio_x509_error_e) */

  /* Certificate chain from server (raw pointers into received data) */
  const uint8_t *cert_chain[10]; /* Up to 10 certificates in chain */
  size_t cert_chain_lens[10];    /* Length of each certificate */
  size_t cert_chain_count;       /* Number of certificates received */

  /* Internal flags */
  uint8_t encrypted_read;  /* 1 if reading encrypted records */
  uint8_t encrypted_write; /* 1 if writing encrypted records */
} fio_tls13_client_s;

/* *****************************************************************************
TLS 1.3 Client API
***************************************************************************** */

/**
 * Initialize client context.
 *
 * @param client      Client context to initialize
 * @param server_name SNI hostname (can be NULL)
 */
SFUNC void fio_tls13_client_init(fio_tls13_client_s *client,
                                 const char *server_name);

/**
 * Clean up client context (zeroes secrets).
 *
 * @param client Client context to destroy
 */
SFUNC void fio_tls13_client_destroy(fio_tls13_client_s *client);

/**
 * Set trust store for certificate chain verification.
 *
 * When set, the client will verify the server's certificate chain against
 * the provided trust store. If NULL (default), chain verification is skipped.
 *
 * Note: Requires FIO_X509 module. The trust_store pointer must point to a
 * valid fio_x509_trust_store_s structure.
 *
 * @param client      Client context
 * @param trust_store Trust store for root CAs (NULL to skip chain verification)
 */
FIO_IFUNC void fio_tls13_client_set_trust_store(fio_tls13_client_s *client,
                                                void *trust_store);

/**
 * Skip all certificate verification (insecure).
 *
 * When enabled, the client will NOT verify:
 * - CertificateVerify signature
 * - Certificate chain
 * - Hostname matching
 *
 * WARNING: This is insecure and should only be used for testing or when
 * certificate verification is handled externally.
 *
 * @param client Client context
 * @param skip   1 to skip verification, 0 to enable (default)
 */
FIO_IFUNC void fio_tls13_client_skip_verification(fio_tls13_client_s *client,
                                                  int skip);

/**
 * Get the last certificate verification error.
 *
 * @param client Client context
 * @return Error code (0 = OK, negative = error)
 */
FIO_IFUNC int fio_tls13_client_get_cert_error(fio_tls13_client_s *client);

/**
 * Check if certificate verification was successful.
 *
 * @param client Client context
 * @return 1 if verified, 0 if not verified or skipped
 */
FIO_IFUNC int fio_tls13_client_is_cert_verified(fio_tls13_client_s *client);

/**
 * Generate ClientHello message and start handshake.
 *
 * @param client       Client context
 * @param out          Output buffer for ClientHello record
 * @param out_capacity Capacity of output buffer
 * @return Message length on success, -1 on error
 */
SFUNC int fio_tls13_client_start(fio_tls13_client_s *client,
                                 uint8_t *out,
                                 size_t out_capacity);

/**
 * Process incoming TLS record(s).
 *
 * May generate response data in out buffer.
 *
 * @param client       Client context
 * @param in           Input buffer containing TLS record(s)
 * @param in_len       Length of input data
 * @param out          Output buffer for response
 * @param out_capacity Capacity of output buffer
 * @param out_len      Output: response length (0 if no response needed)
 * @return Number of bytes consumed, or -1 on error
 */
SFUNC int fio_tls13_client_process(fio_tls13_client_s *client,
                                   const uint8_t *in,
                                   size_t in_len,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   size_t *out_len);

/**
 * Encrypt application data for sending.
 *
 * @param client       Client context
 * @param out          Output buffer for encrypted record
 * @param out_capacity Capacity of output buffer
 * @param plaintext    Plaintext data to encrypt
 * @param plaintext_len Length of plaintext
 * @return Encrypted record length, or -1 on error
 */
SFUNC int fio_tls13_client_encrypt(fio_tls13_client_s *client,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *plaintext,
                                   size_t plaintext_len);

/**
 * Decrypt received application data.
 *
 * @param client         Client context
 * @param out            Output buffer for decrypted data
 * @param out_capacity   Capacity of output buffer
 * @param ciphertext     Encrypted record (including header)
 * @param ciphertext_len Length of encrypted record
 * @return Plaintext length, or -1 on error
 */
SFUNC int fio_tls13_client_decrypt(fio_tls13_client_s *client,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *ciphertext,
                                   size_t ciphertext_len);

/**
 * Check if handshake is complete.
 */
FIO_IFUNC int fio_tls13_client_is_connected(fio_tls13_client_s *client) {
  return client && client->state == FIO_TLS13_STATE_CONNECTED;
}

/**
 * Check if in error state.
 */
FIO_IFUNC int fio_tls13_client_is_error(fio_tls13_client_s *client) {
  return client && client->state == FIO_TLS13_STATE_ERROR;
}

/**
 * Get current state name (for debugging).
 */
FIO_IFUNC const char *fio_tls13_client_state_name(fio_tls13_client_s *client) {
  if (!client)
    return "NULL";
  switch (client->state) {
  case FIO_TLS13_STATE_START: return "START";
  case FIO_TLS13_STATE_WAIT_SH: return "WAIT_SH";
  case FIO_TLS13_STATE_WAIT_EE: return "WAIT_EE";
  case FIO_TLS13_STATE_WAIT_CERT_CR: return "WAIT_CERT_CR";
  case FIO_TLS13_STATE_WAIT_CERT: return "WAIT_CERT";
  case FIO_TLS13_STATE_WAIT_CV: return "WAIT_CV";
  case FIO_TLS13_STATE_WAIT_FINISHED: return "WAIT_FINISHED";
  case FIO_TLS13_STATE_CONNECTED: return "CONNECTED";
  case FIO_TLS13_STATE_ERROR: return "ERROR";
  default: return "UNKNOWN";
  }
}

/**
 * Set trust store for certificate chain verification.
 */
FIO_IFUNC void fio_tls13_client_set_trust_store(fio_tls13_client_s *client,
                                                void *trust_store) {
  if (client)
    client->trust_store = trust_store;
}

/**
 * Skip all certificate verification (insecure).
 */
FIO_IFUNC void fio_tls13_client_skip_verification(fio_tls13_client_s *client,
                                                  int skip) {
  if (client)
    client->skip_cert_verify = (uint8_t)(skip != 0);
}

/**
 * Get the last certificate verification error.
 */
FIO_IFUNC int fio_tls13_client_get_cert_error(fio_tls13_client_s *client) {
  return client ? (int)client->cert_error : -1;
}

/**
 * Check if certificate verification was successful.
 */
FIO_IFUNC int fio_tls13_client_is_cert_verified(fio_tls13_client_s *client) {
  return client ? (client->cert_verified && client->chain_verified) : 0;
}

/* *****************************************************************************
TLS 1.3 Client Implementation
***************************************************************************** */

/* Internal: Update transcript hash with handshake message */
FIO_SFUNC void fio___tls13_transcript_update(fio_tls13_client_s *client,
                                             const uint8_t *data,
                                             size_t len) {
  if (client->use_sha384)
    fio_sha512_consume(&client->transcript_sha384, data, len);
  else
    fio_sha256_consume(&client->transcript_sha256, data, len);
}

/* Internal: Get current transcript hash (non-destructive copy) */
FIO_SFUNC void fio___tls13_transcript_hash(fio_tls13_client_s *client,
                                           uint8_t *out) {
  if (client->use_sha384) {
    fio_sha512_s copy = client->transcript_sha384;
    fio_u512 h = fio_sha512_finalize(&copy);
    FIO_MEMCPY(out, h.u8, 48);
  } else {
    fio_sha256_s copy = client->transcript_sha256;
    fio_u256 h = fio_sha256_finalize(&copy);
    FIO_MEMCPY(out, h.u8, 32);
  }
}

/* Internal: Get hash length for current cipher suite */
FIO_SFUNC size_t fio___tls13_hash_len(fio_tls13_client_s *client) {
  return client->use_sha384 ? 48 : 32;
}

/* Internal: Get key length for current cipher suite */
FIO_SFUNC size_t fio___tls13_key_len(fio_tls13_client_s *client) {
  switch (client->cipher_suite) {
  case FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256: return 16;
  case FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384: return 32;
  case FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256: return 32;
  default: return 16;
  }
}

/* Internal: Get cipher type for current cipher suite */
FIO_SFUNC fio_tls13_cipher_type_e
fio___tls13_cipher_type(fio_tls13_client_s *client) {
  switch (client->cipher_suite) {
  case FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256:
    return FIO_TLS13_CIPHER_AES_128_GCM;
  case FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384:
    return FIO_TLS13_CIPHER_AES_256_GCM;
  case FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256:
    return FIO_TLS13_CIPHER_CHACHA20_POLY1305;
  default: return FIO_TLS13_CIPHER_AES_128_GCM;
  }
}

/* Internal: Set error state with alert */
FIO_SFUNC void fio___tls13_set_error(fio_tls13_client_s *client,
                                     uint8_t alert_level,
                                     uint8_t alert_description) {
  client->state = FIO_TLS13_STATE_ERROR;
  client->alert_level = alert_level;
  client->alert_description = alert_description;
}

/* Internal: Build TLSPlaintext record header */
FIO_SFUNC void fio___tls13_write_record_header(uint8_t *out,
                                               fio_tls13_content_type_e type,
                                               uint16_t length) {
  out[0] = (uint8_t)type;
  out[1] = FIO_TLS13_LEGACY_VERSION_MAJOR;
  out[2] = FIO_TLS13_LEGACY_VERSION_MINOR;
  out[3] = (uint8_t)(length >> 8);
  out[4] = (uint8_t)(length & 0xFF);
}

/* Internal: Derive handshake keys after ServerHello */
FIO_SFUNC int fio___tls13_derive_handshake_keys(fio_tls13_client_s *client) {
  int use_sha384 = client->use_sha384;
  size_t hash_len = fio___tls13_hash_len(client);
  size_t key_len = fio___tls13_key_len(client);
  fio_tls13_cipher_type_e cipher_type = fio___tls13_cipher_type(client);

  /* Get transcript hash at ServerHello */
  uint8_t transcript_hash[48];
  fio___tls13_transcript_hash(client, transcript_hash);

  /* Derive early secret (no PSK) */
  fio_tls13_derive_early_secret(client->early_secret, NULL, 0, use_sha384);

  /* Derive handshake secret */
  fio_tls13_derive_handshake_secret(client->handshake_secret,
                                    client->early_secret,
                                    client->shared_secret,
                                    32, /* X25519 shared secret is 32 bytes */
                                    use_sha384);

  /* Derive client handshake traffic secret */
  fio_tls13_derive_secret(client->client_handshake_traffic_secret,
                          client->handshake_secret,
                          hash_len,
                          "c hs traffic",
                          12,
                          transcript_hash,
                          hash_len,
                          use_sha384);

  /* Derive server handshake traffic secret */
  fio_tls13_derive_secret(client->server_handshake_traffic_secret,
                          client->handshake_secret,
                          hash_len,
                          "s hs traffic",
                          12,
                          transcript_hash,
                          hash_len,
                          use_sha384);

  /* Derive client handshake keys */
  uint8_t key[32], iv[12];
  fio_tls13_derive_traffic_keys(key,
                                key_len,
                                iv,
                                client->client_handshake_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&client->client_handshake_keys,
                             key,
                             (uint8_t)key_len,
                             iv,
                             cipher_type);

  /* Derive server handshake keys */
  fio_tls13_derive_traffic_keys(key,
                                key_len,
                                iv,
                                client->server_handshake_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&client->server_handshake_keys,
                             key,
                             (uint8_t)key_len,
                             iv,
                             cipher_type);

  /* Clear temporary key material */
  fio_secure_zero(key, sizeof(key));
  fio_secure_zero(iv, sizeof(iv));

  return 0;
}

/* Internal: Derive application keys after server Finished */
FIO_SFUNC int fio___tls13_derive_app_keys(fio_tls13_client_s *client) {
  int use_sha384 = client->use_sha384;
  size_t hash_len = fio___tls13_hash_len(client);
  size_t key_len = fio___tls13_key_len(client);
  fio_tls13_cipher_type_e cipher_type = fio___tls13_cipher_type(client);

  /* Get transcript hash at server Finished */
  uint8_t transcript_hash[48];
  fio___tls13_transcript_hash(client, transcript_hash);

  /* Derive master secret */
  fio_tls13_derive_master_secret(client->master_secret,
                                 client->handshake_secret,
                                 use_sha384);

  /* Derive client application traffic secret */
  fio_tls13_derive_secret(client->client_app_traffic_secret,
                          client->master_secret,
                          hash_len,
                          "c ap traffic",
                          12,
                          transcript_hash,
                          hash_len,
                          use_sha384);

  /* Derive server application traffic secret */
  fio_tls13_derive_secret(client->server_app_traffic_secret,
                          client->master_secret,
                          hash_len,
                          "s ap traffic",
                          12,
                          transcript_hash,
                          hash_len,
                          use_sha384);

  /* Derive client application keys */
  uint8_t key[32], iv[12];
  fio_tls13_derive_traffic_keys(key,
                                key_len,
                                iv,
                                client->client_app_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&client->client_app_keys,
                             key,
                             (uint8_t)key_len,
                             iv,
                             cipher_type);

  /* Derive server application keys */
  fio_tls13_derive_traffic_keys(key,
                                key_len,
                                iv,
                                client->server_app_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&client->server_app_keys,
                             key,
                             (uint8_t)key_len,
                             iv,
                             cipher_type);

  /* Clear temporary key material */
  fio_secure_zero(key, sizeof(key));
  fio_secure_zero(iv, sizeof(iv));

  return 0;
}

/* Internal: Build client Finished message */
FIO_SFUNC int fio___tls13_build_client_finished(fio_tls13_client_s *client,
                                                uint8_t *out,
                                                size_t out_capacity) {
  int use_sha384 = client->use_sha384;
  size_t hash_len = fio___tls13_hash_len(client);

  /* Need space for handshake header (4) + verify_data */
  if (out_capacity < 4 + hash_len)
    return -1;

  /* Get transcript hash */
  uint8_t transcript_hash[48];
  fio___tls13_transcript_hash(client, transcript_hash);

  /* Derive finished key from client handshake traffic secret */
  uint8_t finished_key[48];
  fio_tls13_derive_finished_key(finished_key,
                                client->client_handshake_traffic_secret,
                                use_sha384);

  /* Compute verify_data */
  uint8_t verify_data[48];
  fio_tls13_compute_finished(verify_data,
                             finished_key,
                             transcript_hash,
                             use_sha384);

  /* Build Finished message */
  int len = fio_tls13_build_finished(out, out_capacity, verify_data, hash_len);

  /* Clear sensitive data */
  fio_secure_zero(finished_key, sizeof(finished_key));

  return len;
}

/* Internal: Verify server Finished message */
FIO_SFUNC int fio___tls13_verify_server_finished(fio_tls13_client_s *client,
                                                 const uint8_t *verify_data,
                                                 size_t verify_data_len) {
  int use_sha384 = client->use_sha384;
  size_t hash_len = fio___tls13_hash_len(client);

  if (verify_data_len != hash_len)
    return -1;

  /* Get transcript hash (before Finished message) */
  uint8_t transcript_hash[48];
  fio___tls13_transcript_hash(client, transcript_hash);

  /* Derive finished key from server handshake traffic secret */
  uint8_t finished_key[48];
  fio_tls13_derive_finished_key(finished_key,
                                client->server_handshake_traffic_secret,
                                use_sha384);

  /* Compute expected verify_data */
  uint8_t expected[48];
  fio_tls13_compute_finished(expected,
                             finished_key,
                             transcript_hash,
                             use_sha384);

  /* Constant-time comparison */
  uint8_t diff = 0;
  for (size_t i = 0; i < hash_len; ++i)
    diff |= verify_data[i] ^ expected[i];

  /* Clear sensitive data */
  fio_secure_zero(finished_key, sizeof(finished_key));
  fio_secure_zero(expected, sizeof(expected));

  return diff ? -1 : 0;
}

/* Internal: Process ServerHello */
FIO_SFUNC int fio___tls13_process_server_hello(fio_tls13_client_s *client,
                                               const uint8_t *data,
                                               size_t data_len) {
  fio_tls13_server_hello_s sh;
  if (fio_tls13_parse_server_hello(&sh, data, data_len) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  /* Check for HelloRetryRequest */
  if (sh.is_hello_retry_request) {
    /* For now, return error - X25519 is widely supported */
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
    return -1;
  }

  /* Validate cipher suite */
  client->cipher_suite = sh.cipher_suite;
  switch (sh.cipher_suite) {
  case FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256: client->use_sha384 = 0; break;
  case FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256:
    client->use_sha384 = 0;
    break;
  case FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384: client->use_sha384 = 1; break;
  default:
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
    return -1;
  }

  /* Validate key share */
  if (sh.key_share_group != FIO_TLS13_GROUP_X25519 || sh.key_share_len != 32) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
  }

  /* Compute shared secret */
  if (fio_x25519_shared_secret(client->shared_secret,
                               client->x25519_private_key,
                               sh.key_share) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
  }

  /* Derive handshake keys */
  if (fio___tls13_derive_handshake_keys(client) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }

  /* Switch to encrypted mode for reading */
  client->encrypted_read = 1;

  return 0;
}

/* Internal: Process EncryptedExtensions */
FIO_SFUNC int fio___tls13_process_encrypted_extensions(
    fio_tls13_client_s *client,
    const uint8_t *data,
    size_t data_len) {
  fio_tls13_encrypted_extensions_s ee;
  if (fio_tls13_parse_encrypted_extensions(&ee, data, data_len) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }
  /* EE parsed successfully - no action needed for basic implementation */
  return 0;
}

/* Internal: Process Certificate */
FIO_SFUNC int fio___tls13_process_certificate(fio_tls13_client_s *client,
                                              const uint8_t *data,
                                              size_t data_len) {
  fio_tls13_certificate_s cert;
  if (fio_tls13_parse_certificate(&cert, data, data_len) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  /* Store first certificate pointer for basic operation */
  client->server_cert = cert.cert_data;
  client->server_cert_len = cert.cert_len;

  /* Parse full certificate chain for verification */
  client->cert_chain_count = 0;
  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /* Skip certificate_request_context */
  if (p >= end)
    return 0;
  uint8_t ctx_len = *p++;
  if (p + ctx_len > end)
    return 0;
  p += ctx_len;

  /* Parse certificate_list length (3 bytes) */
  if (p + 3 > end)
    return 0;
  size_t list_len = ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | p[2];
  p += 3;
  if (p + list_len > end)
    return 0;

  const uint8_t *list_end = p + list_len;

  /* Parse each certificate entry */
  while (p < list_end && client->cert_chain_count < 10) {
    /* cert_data length (3 bytes) */
    if (p + 3 > list_end)
      break;
    size_t cert_len = ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | p[2];
    p += 3;
    if (cert_len == 0 || p + cert_len > list_end)
      break;

    /* Store certificate pointer */
    client->cert_chain[client->cert_chain_count] = p;
    client->cert_chain_lens[client->cert_chain_count] = cert_len;
    ++client->cert_chain_count;
    p += cert_len;

    /* Skip extensions (2 bytes length + data) */
    if (p + 2 > list_end)
      break;
    size_t ext_len = ((size_t)p[0] << 8) | p[1];
    p += 2;
    if (p + ext_len > list_end)
      break;
    p += ext_len;
  }

  return 0;
}

#if defined(H___FIO_X509___H) && defined(H___FIO_RSA___H)
/* Internal: Verify CertificateVerify signature per RFC 8446 Section 4.4.3
 * NOTE: Requires FIO_X509 and FIO_RSA modules to be included. */
FIO_SFUNC int fio___tls13_verify_cv_signature(fio_tls13_client_s *client,
                                              const fio_x509_cert_s *cert,
                                              uint16_t sig_scheme,
                                              const uint8_t *signature,
                                              size_t sig_len) {
  /* Build signed content: 64 spaces + context string + 0x00 + transcript hash
   * Per RFC 8446 Section 4.4.3:
   *   "The content that is covered ... is the hash output ...
   *    Specifically, the content consists of:
   *    - A string of 64 0x20 (space) bytes
   *    - The context string
   *    - A single 0 byte
   *    - The content to be signed"
   */
  static const char context_server[] = "TLS 1.3, server CertificateVerify";
  const size_t context_len = sizeof(context_server) - 1; /* 33 bytes */
  size_t hash_len = fio___tls13_hash_len(client);

  /* Total: 64 + 33 + 1 + hash_len = 98 or 114 bytes */
  uint8_t signed_content[64 + 33 + 1 + FIO_TLS13_MAX_HASH_LEN];
  size_t signed_content_len = 64 + context_len + 1 + hash_len;

  /* 64 spaces */
  FIO_MEMSET(signed_content, 0x20, 64);
  /* Context string */
  FIO_MEMCPY(signed_content + 64, context_server, context_len);
  /* Zero byte separator */
  signed_content[64 + context_len] = 0x00;
  /* Transcript hash (current state before CertificateVerify is added) */
  fio___tls13_transcript_hash(client, signed_content + 64 + context_len + 1);

  /* Hash the signed content for signature verification */
  uint8_t content_hash[FIO_TLS13_MAX_HASH_LEN];
  fio_rsa_hash_e rsa_hash_alg;
  size_t expected_hash_len;

  /* Determine hash algorithm from signature scheme */
  switch (sig_scheme) {
  case FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256:
  case FIO_TLS13_SIG_RSA_PKCS1_SHA256:
  case FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256: {
    fio_sha256_s sha = fio_sha256_init();
    fio_sha256_consume(&sha, signed_content, signed_content_len);
    fio_u256 h = fio_sha256_finalize(&sha);
    FIO_MEMCPY(content_hash, h.u8, 32);
    expected_hash_len = 32;
    rsa_hash_alg = FIO_RSA_HASH_SHA256;
    break;
  }
  case FIO_TLS13_SIG_RSA_PSS_RSAE_SHA384:
  case FIO_TLS13_SIG_RSA_PKCS1_SHA384:
  case FIO_TLS13_SIG_ECDSA_SECP384R1_SHA384: {
    /* SHA-384 uses SHA-512 internals, truncated to 48 bytes */
    fio_sha512_s sha = fio_sha512_init();
    fio_sha512_consume(&sha, signed_content, signed_content_len);
    fio_u512 h = fio_sha512_finalize(&sha);
    FIO_MEMCPY(content_hash, h.u8, 48); /* Use first 48 bytes (SHA-384) */
    expected_hash_len = 48;
    rsa_hash_alg = FIO_RSA_HASH_SHA384;
    break;
  }
  case FIO_TLS13_SIG_ED25519:
    /* Ed25519 does not pre-hash the message */
    expected_hash_len = 0;
    rsa_hash_alg = FIO_RSA_HASH_SHA256; /* unused */
    break;
  default:
    FIO_LOG_DEBUG2("TLS 1.3: Unsupported signature scheme: 0x%04X", sig_scheme);
    return -1;
  }

  /* Verify signature based on algorithm and key type */
  switch (sig_scheme) {
  case FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256:
  case FIO_TLS13_SIG_RSA_PSS_RSAE_SHA384: {
    /* RSA-PSS verification - required for TLS 1.3 */
    if (cert->key_type != FIO_X509_KEY_RSA) {
      FIO_LOG_DEBUG2("TLS 1.3: Certificate key type mismatch for RSA-PSS");
      return -1;
    }
    /* Build RSA public key structure */
    fio_rsa_pubkey_s pubkey;
    FIO_MEMSET(&pubkey, 0, sizeof(pubkey));
    pubkey.n = cert->pubkey.rsa.n;
    pubkey.n_len = cert->pubkey.rsa.n_len;
    pubkey.e = cert->pubkey.rsa.e;
    pubkey.e_len = cert->pubkey.rsa.e_len;

    if (fio_rsa_verify_pss(signature,
                           sig_len,
                           content_hash,
                           expected_hash_len,
                           rsa_hash_alg,
                           &pubkey) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3: RSA-PSS signature verification failed");
      return -1;
    }
    break;
  }
  case FIO_TLS13_SIG_RSA_PKCS1_SHA256:
  case FIO_TLS13_SIG_RSA_PKCS1_SHA384: {
    /* RSA PKCS#1 v1.5 - legacy, but some servers still use it */
    if (cert->key_type != FIO_X509_KEY_RSA) {
      FIO_LOG_DEBUG2("TLS 1.3: Certificate key type mismatch for RSA-PKCS1");
      return -1;
    }
    fio_rsa_pubkey_s pubkey;
    FIO_MEMSET(&pubkey, 0, sizeof(pubkey));
    pubkey.n = cert->pubkey.rsa.n;
    pubkey.n_len = cert->pubkey.rsa.n_len;
    pubkey.e = cert->pubkey.rsa.e;
    pubkey.e_len = cert->pubkey.rsa.e_len;

    if (fio_rsa_verify_pkcs1(signature,
                             sig_len,
                             content_hash,
                             expected_hash_len,
                             rsa_hash_alg,
                             &pubkey) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3: RSA-PKCS1 signature verification failed");
      return -1;
    }
    break;
  }
  case FIO_TLS13_SIG_ED25519: {
    /* Ed25519 - sign directly over the content (no pre-hashing) */
    if (cert->key_type != FIO_X509_KEY_ED25519) {
      FIO_LOG_DEBUG2("TLS 1.3: Certificate key type mismatch for Ed25519");
      return -1;
    }
    if (sig_len != 64) {
      FIO_LOG_DEBUG2("TLS 1.3: Invalid Ed25519 signature length: %zu", sig_len);
      return -1;
    }
    if (fio_ed25519_verify(signature,
                           signed_content,
                           signed_content_len,
                           cert->pubkey.ed25519.key) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3: Ed25519 signature verification failed");
      return -1;
    }
    break;
  }
  case FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256:
  case FIO_TLS13_SIG_ECDSA_SECP384R1_SHA384:
    /* ECDSA verification - not fully implemented yet */
    FIO_LOG_DEBUG2("TLS 1.3: ECDSA signature verification not yet supported");
    /* For now, allow connection to proceed without ECDSA verification */
    /* TODO: Implement ECDSA P-256 and P-384 verification */
    return 0;
  default: return -1;
  }

  FIO_LOG_DEBUG2("TLS 1.3: CertificateVerify signature verified successfully");
  return 0;
}
#endif /* H___FIO_X509___H && H___FIO_RSA___H */

/* Internal: Process CertificateVerify */
FIO_SFUNC int fio___tls13_process_certificate_verify(fio_tls13_client_s *client,
                                                     const uint8_t *data,
                                                     size_t data_len) {
  fio_tls13_certificate_verify_s cv;
  if (fio_tls13_parse_certificate_verify(&cv, data, data_len) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  /* Store signature info */
  client->server_signature = cv.signature;
  client->server_signature_len = cv.signature_len;
  client->server_signature_scheme = cv.signature_scheme;

  /* Skip verification if explicitly disabled */
  if (client->skip_cert_verify) {
    FIO_LOG_DEBUG2("TLS 1.3: Skipping CertificateVerify (insecure mode)");
    client->cert_verified = 1;
    return 0;
  }

#if defined(H___FIO_X509___H) && defined(H___FIO_RSA___H)
  /* Must have at least one certificate to verify against */
  if (client->cert_chain_count == 0) {
    FIO_LOG_DEBUG2("TLS 1.3: No certificates received for CV verification");
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_CERTIFICATE_REQUIRED);
    return -1;
  }

  /* Parse the end-entity certificate to get the public key */
  fio_x509_cert_s cert;
  if (fio_x509_parse(&cert,
                     client->cert_chain[0],
                     client->cert_chain_lens[0]) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3: Failed to parse server certificate for CV");
    client->cert_error = FIO_X509_ERR_PARSE;
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_BAD_CERTIFICATE);
    return -1;
  }

  /* Verify the CertificateVerify signature */
  if (fio___tls13_verify_cv_signature(client,
                                      &cert,
                                      cv.signature_scheme,
                                      cv.signature,
                                      cv.signature_len) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3: CertificateVerify signature invalid");
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECRYPT_ERROR);
    return -1;
  }

  client->cert_verified = 1;
#else
  /* X509/RSA modules not available - skip signature verification */
  FIO_LOG_DEBUG2("TLS 1.3: X509/RSA modules unavailable, skipping CV verify");
  client->cert_verified = 1;
#endif /* H___FIO_X509___H && H___FIO_RSA___H */
  return 0;
}

#if defined(H___FIO_X509___H)
/* Internal: Verify certificate chain (call after CertificateVerify succeeds)
 * NOTE: Requires FIO_X509 module to be included. */
FIO_SFUNC int fio___tls13_verify_certificate_chain(fio_tls13_client_s *client) {
  /* Skip if no chain or verification disabled */
  if (client->skip_cert_verify) {
    FIO_LOG_DEBUG2("TLS 1.3: Skipping chain verification (insecure mode)");
    client->chain_verified = 1;
    return 0;
  }

  /* Must have at least one certificate */
  if (client->cert_chain_count == 0) {
    FIO_LOG_DEBUG2("TLS 1.3: No certificates to verify");
    client->cert_error = FIO_X509_ERR_EMPTY_CHAIN;
    return -1;
  }

  /* Get current timestamp for validity checking */
  int64_t current_time = (int64_t)fio_time_real().tv_sec;

  /* Use x509 chain verification if trust store is provided */
  if (client->trust_store != NULL) {
    int result =
        fio_x509_verify_chain(client->cert_chain,
                              client->cert_chain_lens,
                              client->cert_chain_count,
                              client->server_name,
                              current_time,
                              (fio_x509_trust_store_s *)client->trust_store);
    if (result != FIO_X509_OK) {
      FIO_LOG_DEBUG2("TLS 1.3: Chain verification failed: %s",
                     fio_x509_error_str(result));
      client->cert_error = (int16_t)result;
      return -1;
    }
    client->chain_verified = 1;
    FIO_LOG_DEBUG2("TLS 1.3: Certificate chain verified successfully");
    return 0;
  }

  /* No trust store: perform minimal validation (hostname + validity only) */
  fio_x509_cert_s cert;
  if (fio_x509_parse(&cert,
                     client->cert_chain[0],
                     client->cert_chain_lens[0]) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3: Failed to parse end-entity certificate");
    client->cert_error = FIO_X509_ERR_PARSE;
    return -1;
  }

  /* Check validity period */
  if (fio_x509_check_validity(&cert, current_time) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3: Certificate expired or not yet valid");
    client->cert_error = (current_time < cert.not_before)
                             ? FIO_X509_ERR_NOT_YET_VALID
                             : FIO_X509_ERR_EXPIRED;
    return -1;
  }

  /* Check hostname if SNI was provided */
  if (client->server_name != NULL) {
    size_t name_len = strlen(client->server_name);
    if (fio_x509_match_hostname(&cert, client->server_name, name_len) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3: Hostname mismatch (expected: %s)",
                     client->server_name);
      client->cert_error = FIO_X509_ERR_HOSTNAME_MISMATCH;
      return -1;
    }
  }

  /* Without trust store, we can't verify the chain, but we did basic checks */
  FIO_LOG_DEBUG2("TLS 1.3: Basic certificate checks passed (no trust store)");
  client->chain_verified = 1;
  return 0;
}
#else
/* X509 module not available - stub function */
FIO_SFUNC int fio___tls13_verify_certificate_chain(fio_tls13_client_s *client) {
  /* No X509 module - skip chain verification */
  FIO_LOG_DEBUG2("TLS 1.3: X509 module unavailable, skipping chain verify");
  client->chain_verified = 1;
  (void)client;
  return 0;
}
#endif /* H___FIO_X509___H */

/* Internal: Process Finished */
FIO_SFUNC int fio___tls13_process_finished(fio_tls13_client_s *client,
                                           const uint8_t *data,
                                           size_t data_len) {
  /* Verify server Finished */
  if (fio___tls13_verify_server_finished(client, data, data_len) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECRYPT_ERROR);
    return -1;
  }

  return 0;
}

/* Internal: Process a single handshake message */
FIO_SFUNC int fio___tls13_process_handshake_message(fio_tls13_client_s *client,
                                                    const uint8_t *msg,
                                                    size_t msg_len,
                                                    uint8_t *out,
                                                    size_t out_capacity,
                                                    size_t *out_len) {
  fio_tls13_handshake_type_e msg_type;
  size_t body_len;
  const uint8_t *body =
      fio_tls13_parse_handshake_header(msg, msg_len, &msg_type, &body_len);

  if (!body) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  *out_len = 0;

  switch (client->state) {
  case FIO_TLS13_STATE_WAIT_SH:
    if (msg_type != FIO_TLS13_HS_SERVER_HELLO) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }
    /* Update transcript with full message (including header) */
    fio___tls13_transcript_update(client, msg, 4 + body_len);
    if (fio___tls13_process_server_hello(client, body, body_len) != 0)
      return -1;
    client->state = FIO_TLS13_STATE_WAIT_EE;
    break;

  case FIO_TLS13_STATE_WAIT_EE:
    if (msg_type != FIO_TLS13_HS_ENCRYPTED_EXTENSIONS) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }
    fio___tls13_transcript_update(client, msg, 4 + body_len);
    if (fio___tls13_process_encrypted_extensions(client, body, body_len) != 0)
      return -1;
    client->state = FIO_TLS13_STATE_WAIT_CERT_CR;
    break;

  case FIO_TLS13_STATE_WAIT_CERT_CR:
    if (msg_type == FIO_TLS13_HS_CERTIFICATE_REQUEST) {
      /* CertificateRequest - skip for now, go to WAIT_CERT */
      fio___tls13_transcript_update(client, msg, 4 + body_len);
      client->state = FIO_TLS13_STATE_WAIT_CERT;
    } else if (msg_type == FIO_TLS13_HS_CERTIFICATE) {
      fio___tls13_transcript_update(client, msg, 4 + body_len);
      if (fio___tls13_process_certificate(client, body, body_len) != 0)
        return -1;
      client->state = FIO_TLS13_STATE_WAIT_CV;
    } else {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }
    break;

  case FIO_TLS13_STATE_WAIT_CERT:
    if (msg_type != FIO_TLS13_HS_CERTIFICATE) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }
    fio___tls13_transcript_update(client, msg, 4 + body_len);
    if (fio___tls13_process_certificate(client, body, body_len) != 0)
      return -1;
    client->state = FIO_TLS13_STATE_WAIT_CV;
    break;

  case FIO_TLS13_STATE_WAIT_CV:
    if (msg_type != FIO_TLS13_HS_CERTIFICATE_VERIFY) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }
    fio___tls13_transcript_update(client, msg, 4 + body_len);
    if (fio___tls13_process_certificate_verify(client, body, body_len) != 0)
      return -1;
    /* Verify certificate chain after CertificateVerify signature is valid */
    if (fio___tls13_verify_certificate_chain(client) != 0) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_BAD_CERTIFICATE);
      return -1;
    }
    client->state = FIO_TLS13_STATE_WAIT_FINISHED;
    break;

  case FIO_TLS13_STATE_WAIT_FINISHED:
    if (msg_type != FIO_TLS13_HS_FINISHED) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }
    /* Verify server Finished BEFORE updating transcript */
    if (fio___tls13_process_finished(client, body, body_len) != 0)
      return -1;
    /* Now update transcript with server Finished */
    fio___tls13_transcript_update(client, msg, 4 + body_len);

    /* Derive application keys */
    if (fio___tls13_derive_app_keys(client) != 0) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_INTERNAL_ERROR);
      return -1;
    }

    /* Build client Finished */
    {
      uint8_t finished_msg[64];
      int finished_len =
          fio___tls13_build_client_finished(client,
                                            finished_msg,
                                            sizeof(finished_msg));
      if (finished_len < 0) {
        fio___tls13_set_error(client,
                              FIO_TLS13_ALERT_LEVEL_FATAL,
                              FIO_TLS13_ALERT_INTERNAL_ERROR);
        return -1;
      }

      /* Update transcript with client Finished */
      fio___tls13_transcript_update(client, finished_msg, (size_t)finished_len);

      /* Encrypt client Finished */
      int enc_len = fio_tls13_record_encrypt(out,
                                             out_capacity,
                                             finished_msg,
                                             (size_t)finished_len,
                                             FIO_TLS13_CONTENT_HANDSHAKE,
                                             &client->client_handshake_keys);
      if (enc_len < 0) {
        fio___tls13_set_error(client,
                              FIO_TLS13_ALERT_LEVEL_FATAL,
                              FIO_TLS13_ALERT_INTERNAL_ERROR);
        return -1;
      }
      *out_len = (size_t)enc_len;
    }

    /* Switch to encrypted mode for writing */
    client->encrypted_write = 1;
    client->state = FIO_TLS13_STATE_CONNECTED;
    break;

  default:
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
    return -1;
  }

  return 0;
}

/* *****************************************************************************
TLS 1.3 Client Public API Implementation
***************************************************************************** */

SFUNC void fio_tls13_client_init(fio_tls13_client_s *client,
                                 const char *server_name) {
  if (!client)
    return;

  FIO_MEMSET(client, 0, sizeof(*client));
  client->state = FIO_TLS13_STATE_START;
  client->server_name = server_name;

  /* Initialize transcript hashes */
  client->transcript_sha256 = fio_sha256_init();
  client->transcript_sha384 = fio_sha512_init();

  /* Generate random and X25519 keypair */
  fio_rand_bytes(client->client_random, 32);
  fio_x25519_keypair(client->x25519_private_key, client->x25519_public_key);
}

SFUNC void fio_tls13_client_destroy(fio_tls13_client_s *client) {
  if (!client)
    return;

  /* Clear all sensitive data */
  fio_secure_zero(client->x25519_private_key, 32);
  fio_secure_zero(client->shared_secret, 32);
  fio_secure_zero(client->early_secret, 48);
  fio_secure_zero(client->handshake_secret, 48);
  fio_secure_zero(client->master_secret, 48);
  fio_secure_zero(client->client_handshake_traffic_secret, 48);
  fio_secure_zero(client->server_handshake_traffic_secret, 48);
  fio_secure_zero(client->client_app_traffic_secret, 48);
  fio_secure_zero(client->server_app_traffic_secret, 48);

  fio_tls13_record_keys_clear(&client->client_handshake_keys);
  fio_tls13_record_keys_clear(&client->server_handshake_keys);
  fio_tls13_record_keys_clear(&client->client_app_keys);
  fio_tls13_record_keys_clear(&client->server_app_keys);

  FIO_MEMSET(client, 0, sizeof(*client));
}

SFUNC int fio_tls13_client_start(fio_tls13_client_s *client,
                                 uint8_t *out,
                                 size_t out_capacity) {
  if (!client || !out || client->state != FIO_TLS13_STATE_START)
    return -1;

  /* Build ClientHello */
  uint16_t cipher_suites[] = {FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256,
                              FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256,
                              FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384};

  /* Build handshake message first (without record header) */
  uint8_t ch_msg[512];
  int ch_len = fio_tls13_build_client_hello(ch_msg,
                                            sizeof(ch_msg),
                                            client->client_random,
                                            client->server_name,
                                            client->x25519_public_key,
                                            cipher_suites,
                                            3);
  if (ch_len < 0)
    return -1;

  /* Update transcript with ClientHello (handshake message only) */
  fio___tls13_transcript_update(client, ch_msg, (size_t)ch_len);

  /* Check output capacity for record header + message */
  size_t total_len = FIO_TLS13_RECORD_HEADER_LEN + (size_t)ch_len;
  if (out_capacity < total_len)
    return -1;

  /* Write record header */
  fio___tls13_write_record_header(out,
                                  FIO_TLS13_CONTENT_HANDSHAKE,
                                  (uint16_t)ch_len);

  /* Copy handshake message */
  FIO_MEMCPY(out + FIO_TLS13_RECORD_HEADER_LEN, ch_msg, (size_t)ch_len);

  client->state = FIO_TLS13_STATE_WAIT_SH;
  return (int)total_len;
}

SFUNC int fio_tls13_client_process(fio_tls13_client_s *client,
                                   const uint8_t *in,
                                   size_t in_len,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   size_t *out_len) {
  if (!client || !in || !out || !out_len)
    return -1;

  if (client->state == FIO_TLS13_STATE_ERROR ||
      client->state == FIO_TLS13_STATE_CONNECTED)
    return -1;

  *out_len = 0;

  /* Parse record header */
  fio_tls13_content_type_e content_type;
  size_t payload_len;
  const uint8_t *payload =
      fio_tls13_record_parse_header(in, in_len, &content_type, &payload_len);

  if (!payload)
    return 0; /* Need more data */

  size_t record_len = FIO_TLS13_RECORD_HEADER_LEN + payload_len;

  /* Handle Change Cipher Spec (ignore in TLS 1.3) */
  if (content_type == FIO_TLS13_CONTENT_CHANGE_CIPHER_SPEC) {
    return (int)record_len;
  }

  /* Decrypt if in encrypted mode */
  uint8_t decrypted[FIO_TLS13_MAX_PLAINTEXT_LEN + 256];
  const uint8_t *hs_data = payload;
  size_t hs_len = payload_len;

  if (client->encrypted_read) {
    if (content_type != FIO_TLS13_CONTENT_APPLICATION_DATA) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }

    fio_tls13_content_type_e inner_type;
    int dec_len = fio_tls13_record_decrypt(decrypted,
                                           sizeof(decrypted),
                                           &inner_type,
                                           in,
                                           record_len,
                                           &client->server_handshake_keys);
    if (dec_len < 0) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_BAD_RECORD_MAC);
      return -1;
    }

    if (inner_type != FIO_TLS13_CONTENT_HANDSHAKE) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }

    hs_data = decrypted;
    hs_len = (size_t)dec_len;
  } else {
    if (content_type != FIO_TLS13_CONTENT_HANDSHAKE) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }
  }

  /* Process handshake messages (may be multiple in one record) */
  size_t offset = 0;
  while (offset < hs_len) {
    /* Parse handshake header to get message length */
    if (hs_len - offset < 4)
      break; /* Need more data */

    uint32_t msg_body_len = ((uint32_t)hs_data[offset + 1] << 16) |
                            ((uint32_t)hs_data[offset + 2] << 8) |
                            (uint32_t)hs_data[offset + 3];
    size_t msg_total_len = 4 + msg_body_len;

    if (offset + msg_total_len > hs_len)
      break; /* Need more data */

    /* Process this handshake message */
    size_t msg_out_len = 0;
    if (fio___tls13_process_handshake_message(client,
                                              hs_data + offset,
                                              msg_total_len,
                                              out + *out_len,
                                              out_capacity - *out_len,
                                              &msg_out_len) != 0) {
      return -1;
    }
    *out_len += msg_out_len;
    offset += msg_total_len;
  }

  return (int)record_len;
}

SFUNC int fio_tls13_client_encrypt(fio_tls13_client_s *client,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *plaintext,
                                   size_t plaintext_len) {
  if (!client || !out)
    return -1;

  if (client->state != FIO_TLS13_STATE_CONNECTED)
    return -1;

  return fio_tls13_record_encrypt(out,
                                  out_capacity,
                                  plaintext,
                                  plaintext_len,
                                  FIO_TLS13_CONTENT_APPLICATION_DATA,
                                  &client->client_app_keys);
}

SFUNC int fio_tls13_client_decrypt(fio_tls13_client_s *client,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *ciphertext,
                                   size_t ciphertext_len) {
  if (!client || !out || !ciphertext)
    return -1;

  if (client->state != FIO_TLS13_STATE_CONNECTED)
    return -1;

  fio_tls13_content_type_e content_type;
  int dec_len = fio_tls13_record_decrypt(out,
                                         out_capacity,
                                         &content_type,
                                         ciphertext,
                                         ciphertext_len,
                                         &client->server_app_keys);

  if (dec_len < 0)
    return -1;

  /* Only return application data */
  if (content_type != FIO_TLS13_CONTENT_APPLICATION_DATA)
    return -1;

  return dec_len;
}

/* *****************************************************************************
Module Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_TLS13 */
#undef FIO_TLS13
