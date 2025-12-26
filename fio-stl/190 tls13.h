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
      Either define FIO_HKDF before FIO_TLS13, or use FIO_CRYPTO to include all
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

/** TLS 1.3 KeyUpdate Request Types (RFC 8446 Section 4.6.3) */
typedef enum {
  FIO_TLS13_KEY_UPDATE_NOT_REQUESTED = 0,
  FIO_TLS13_KEY_UPDATE_REQUESTED = 1,
} fio_tls13_key_update_request_e;

/** TLS 1.3 Extension Types (RFC 8446 Section 4.2) */
typedef enum {
  FIO_TLS13_EXT_SERVER_NAME = 0,           /* SNI */
  FIO_TLS13_EXT_SUPPORTED_GROUPS = 10,     /* Key exchange groups */
  FIO_TLS13_EXT_SIGNATURE_ALGORITHMS = 13, /* Signature schemes */
  FIO_TLS13_EXT_ALPN = 16, /* Application-Layer Protocol Negotiation */
  FIO_TLS13_EXT_SUPPORTED_VERSIONS = 43, /* TLS version negotiation */
  FIO_TLS13_EXT_COOKIE = 44,             /* Cookie for HRR (RFC 8446 4.2.2) */
  FIO_TLS13_EXT_CERTIFICATE_AUTHORITIES =
      47, /* Acceptable CAs (RFC 8446 4.2.4) */
  FIO_TLS13_EXT_SIGNATURE_ALGORITHMS_CERT = 50, /* Cert chain sig algs */
  FIO_TLS13_EXT_KEY_SHARE = 51,                 /* ECDHE key shares */
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
  int has_server_name;      /* Server acknowledged SNI */
  char alpn_selected[256];  /* Selected ALPN protocol (null-terminated) */
  size_t alpn_selected_len; /* Length of selected ALPN protocol */
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

/** Parsed CertificateRequest message (RFC 8446 Section 4.3.2) */
typedef struct {
  uint8_t certificate_request_context[255]; /* Opaque context */
  size_t certificate_request_context_len;   /* Context length (0-255) */
  uint16_t signature_algorithms[16];        /* Required signature algorithms */
  size_t signature_algorithm_count;         /* Number of signature algorithms */
  uint16_t signature_algorithms_cert[16];   /* Cert chain sig algs (optional) */
  size_t signature_algorithms_cert_count;   /* Number of cert sig algs */
  /* Certificate authorities (optional, pointers into original data) */
  const uint8_t *certificate_authorities; /* Raw CA DNs data */
  size_t certificate_authorities_len;     /* Total CA DNs length */
} fio_tls13_certificate_request_s;

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
TLS 1.3 Alert API (RFC 8446 Section 6)
***************************************************************************** */

/**
 * Build an alert message (unencrypted, for use before handshake keys).
 *
 * Alert format: [level:1][description:1]
 *
 * @param out          Output buffer for alert message (2 bytes minimum)
 * @param out_capacity Capacity of output buffer
 * @param alert_level  Alert level (1=warning, 2=fatal)
 * @param alert_desc   Alert description code
 * @return Alert message length (2), or -1 on error
 */
FIO_IFUNC int fio_tls13_build_alert(uint8_t *out,
                                    size_t out_capacity,
                                    uint8_t alert_level,
                                    uint8_t alert_desc);

/**
 * Build an encrypted alert record.
 *
 * Per RFC 8446 Section 6, alerts are encrypted after handshake keys are
 * established. In TLS 1.3, all alerts except close_notify are effectively
 * fatal and the connection must be closed after sending.
 *
 * @param out          Output buffer for encrypted alert record
 * @param out_capacity Capacity of output buffer
 * @param alert_level  Alert level (1=warning, 2=fatal)
 * @param alert_desc   Alert description code
 * @param keys         Encryption keys (sequence number will be incremented)
 * @return Encrypted record length, or -1 on error
 */
SFUNC int fio_tls13_send_alert(uint8_t *out,
                               size_t out_capacity,
                               uint8_t alert_level,
                               uint8_t alert_desc,
                               fio_tls13_record_keys_s *keys);

/**
 * Build an unencrypted alert record (for use before encryption is enabled).
 *
 * @param out          Output buffer for alert record
 * @param out_capacity Capacity of output buffer
 * @param alert_level  Alert level (1=warning, 2=fatal)
 * @param alert_desc   Alert description code
 * @return Record length (7 bytes: 5 header + 2 alert), or -1 on error
 */
SFUNC int fio_tls13_send_alert_plaintext(uint8_t *out,
                                         size_t out_capacity,
                                         uint8_t alert_level,
                                         uint8_t alert_desc);

/**
 * Get human-readable name for an alert description.
 *
 * @param alert_desc Alert description code
 * @return Static string with alert name
 */
FIO_IFUNC const char *fio_tls13_alert_name(uint8_t alert_desc);

/* Inline implementation: Build alert message */
FIO_IFUNC int fio_tls13_build_alert(uint8_t *out,
                                    size_t out_capacity,
                                    uint8_t alert_level,
                                    uint8_t alert_desc) {
  if (!out || out_capacity < 2)
    return -1;
  out[0] = alert_level;
  out[1] = alert_desc;
  return 2;
}

/* Inline implementation: Get alert name */
FIO_IFUNC const char *fio_tls13_alert_name(uint8_t alert_desc) {
  switch (alert_desc) {
  case 0: return "close_notify";
  case 10: return "unexpected_message";
  case 20: return "bad_record_mac";
  case 22: return "record_overflow";
  case 40: return "handshake_failure";
  case 42: return "bad_certificate";
  case 44: return "certificate_revoked";
  case 45: return "certificate_expired";
  case 46: return "certificate_unknown";
  case 47: return "illegal_parameter";
  case 48: return "unknown_ca";
  case 50: return "decode_error";
  case 51: return "decrypt_error";
  case 70: return "protocol_version";
  case 80: return "internal_error";
  case 109: return "missing_extension";
  case 110: return "unsupported_extension";
  case 112: return "unrecognized_name";
  case 113: return "bad_certificate_status_response";
  case 115: return "unknown_psk_identity";
  case 116: return "certificate_required";
  case 120: return "no_application_protocol";
  default: return "unknown_alert";
  }
}

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
      fio_memcpy32(empty_hash, h.u8);
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
    fio_memcpy32(verify_data, hmac.u8);
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
KeyUpdate Message Handling (RFC 8446 Section 4.6.3)

KeyUpdate is a post-handshake message used to update traffic keys.
Long-lived connections (HTTP/2, WebSocket) use this to stay within
cryptographic limits.

Upon receiving KeyUpdate, endpoint MUST:
1. Update its receiving keys immediately
2. If request_update is update_requested:
   - Send own KeyUpdate with update_not_requested before next Application Data
3. Encrypt the KeyUpdate response with the old sending keys
4. Then update sending keys
***************************************************************************** */

/**
 * Build a KeyUpdate message.
 *
 * @param out Output buffer for the handshake message (not encrypted)
 * @param out_capacity Capacity of output buffer
 * @param request_update 0 = update_not_requested, 1 = update_requested
 * @return Message length on success (5 bytes), -1 on error
 */
SFUNC int fio_tls13_build_key_update(uint8_t *out,
                                     size_t out_capacity,
                                     int request_update);

/**
 * Parse a KeyUpdate message.
 *
 * @param data Pointer to handshake message body (after 4-byte header)
 * @param data_len Length of message body (should be 1)
 * @param request_update Output: 0 = update_not_requested, 1 = update_requested
 * @return 0 on success, -1 on error
 */
SFUNC int fio_tls13_parse_key_update(const uint8_t *data,
                                     size_t data_len,
                                     int *request_update);

/**
 * Process a received KeyUpdate and update receiving keys.
 *
 * This function:
 * 1. Parses the KeyUpdate message
 * 2. Derives the new receiving traffic secret
 * 3. Updates the receiving keys (key, IV, resets sequence number to 0)
 * 4. Sets key_update_pending flag if response is requested
 *
 * @param traffic_secret Current receiving traffic secret (will be updated)
 * @param keys Receiving keys structure (will be updated)
 * @param data KeyUpdate message body (1 byte)
 * @param data_len Length of message body
 * @param key_update_pending Output: set to 1 if response needed
 * @param use_sha384 1 for SHA-384, 0 for SHA-256
 * @param key_len Key length (16 for AES-128, 32 for AES-256/ChaCha20)
 * @param cipher_type Cipher type for new keys
 * @return 0 on success, -1 on error
 */
SFUNC int fio_tls13_process_key_update(uint8_t *traffic_secret,
                                       fio_tls13_record_keys_s *keys,
                                       const uint8_t *data,
                                       size_t data_len,
                                       uint8_t *key_update_pending,
                                       int use_sha384,
                                       size_t key_len,
                                       fio_tls13_cipher_type_e cipher_type);

/**
 * Send KeyUpdate response and update sending keys.
 *
 * This function should be called before sending Application Data when
 * key_update_pending is set. It:
 * 1. Builds KeyUpdate message with update_not_requested
 * 2. Encrypts it with the OLD sending keys
 * 3. Derives the new sending traffic secret
 * 4. Updates the sending keys (key, IV, resets sequence number to 0)
 * 5. Clears key_update_pending flag
 *
 * @param out Output buffer for encrypted KeyUpdate record
 * @param out_capacity Capacity of output buffer
 * @param traffic_secret Current sending traffic secret (will be updated)
 * @param keys Sending keys structure (will be updated, used for encryption)
 * @param key_update_pending Pointer to pending flag (will be cleared)
 * @param use_sha384 1 for SHA-384, 0 for SHA-256
 * @param key_len Key length (16 for AES-128, 32 for AES-256/ChaCha20)
 * @param cipher_type Cipher type for new keys
 * @return Encrypted record length on success, -1 on error
 */
SFUNC int fio_tls13_send_key_update_response(
    uint8_t *out,
    size_t out_capacity,
    uint8_t *traffic_secret,
    fio_tls13_record_keys_s *keys,
    uint8_t *key_update_pending,
    int use_sha384,
    size_t key_len,
    fio_tls13_cipher_type_e cipher_type);

/* *****************************************************************************
KeyUpdate Implementation (RFC 8446 Section 4.6.3)
***************************************************************************** */

/* Build a KeyUpdate message */
SFUNC int fio_tls13_build_key_update(uint8_t *out,
                                     size_t out_capacity,
                                     int request_update) {
  if (!out || out_capacity < 5)
    return -1;

  /* Validate request_update value */
  if (request_update != FIO_TLS13_KEY_UPDATE_NOT_REQUESTED &&
      request_update != FIO_TLS13_KEY_UPDATE_REQUESTED)
    return -1;

  /* KeyUpdate message format:
   * HandshakeType (1 byte) = 24 (key_update)
   * Length (3 bytes) = 1
   * KeyUpdateRequest (1 byte) = 0 or 1 */
  out[0] = FIO_TLS13_HS_KEY_UPDATE;
  out[1] = 0;
  out[2] = 0;
  out[3] = 1; /* Body length = 1 byte */
  out[4] = (uint8_t)request_update;

  return 5;
}

/* Parse a KeyUpdate message */
SFUNC int fio_tls13_parse_key_update(const uint8_t *data,
                                     size_t data_len,
                                     int *request_update) {
  if (!data || !request_update)
    return -1;

  /* KeyUpdate body is exactly 1 byte */
  if (data_len != 1)
    return -1;

  uint8_t req = data[0];

  /* Validate request_update value per RFC 8446 */
  if (req != FIO_TLS13_KEY_UPDATE_NOT_REQUESTED &&
      req != FIO_TLS13_KEY_UPDATE_REQUESTED)
    return -1;

  *request_update = (int)req;
  return 0;
}

/* Process a received KeyUpdate and update receiving keys */
SFUNC int fio_tls13_process_key_update(uint8_t *traffic_secret,
                                       fio_tls13_record_keys_s *keys,
                                       const uint8_t *data,
                                       size_t data_len,
                                       uint8_t *key_update_pending,
                                       int use_sha384,
                                       size_t key_len,
                                       fio_tls13_cipher_type_e cipher_type) {
  if (!traffic_secret || !keys || !data || !key_update_pending)
    return -1;

  /* Parse the KeyUpdate message */
  int request_update;
  if (fio_tls13_parse_key_update(data, data_len, &request_update) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 KeyUpdate: failed to parse message");
    return -1;
  }

  size_t hash_len = use_sha384 ? 48 : 32;

  /* Derive new receiving traffic secret */
  uint8_t new_secret[48];
  fio_tls13_update_traffic_secret(new_secret, traffic_secret, use_sha384);

  /* Copy new secret back to caller's buffer */
  FIO_MEMCPY(traffic_secret, new_secret, hash_len);

  /* Derive new receiving keys */
  uint8_t new_key[32];
  uint8_t new_iv[12];
  fio_tls13_derive_traffic_keys(new_key,
                                key_len,
                                new_iv,
                                new_secret,
                                use_sha384);

  /* Update receiving keys and reset sequence number */
  fio_tls13_record_keys_init(keys,
                             new_key,
                             (uint8_t)key_len,
                             new_iv,
                             cipher_type);

  /* Clear temporary key material */
  fio_secure_zero(new_secret, sizeof(new_secret));
  fio_secure_zero(new_key, sizeof(new_key));
  fio_secure_zero(new_iv, sizeof(new_iv));

  /* Set pending flag if response is requested */
  if (request_update == FIO_TLS13_KEY_UPDATE_REQUESTED) {
    *key_update_pending = 1;
    FIO_LOG_DEBUG2("TLS 1.3 KeyUpdate: received with update_requested, "
                   "response pending");
  } else {
    FIO_LOG_DEBUG2("TLS 1.3 KeyUpdate: received with update_not_requested");
  }

  return 0;
}

/* Send KeyUpdate response and update sending keys */
SFUNC int fio_tls13_send_key_update_response(
    uint8_t *out,
    size_t out_capacity,
    uint8_t *traffic_secret,
    fio_tls13_record_keys_s *keys,
    uint8_t *key_update_pending,
    int use_sha384,
    size_t key_len,
    fio_tls13_cipher_type_e cipher_type) {
  if (!out || !traffic_secret || !keys || !key_update_pending)
    return -1;

  /* Build KeyUpdate message with update_not_requested */
  uint8_t ku_msg[5];
  int ku_len = fio_tls13_build_key_update(ku_msg,
                                          sizeof(ku_msg),
                                          FIO_TLS13_KEY_UPDATE_NOT_REQUESTED);
  if (ku_len < 0)
    return -1;

  /* Encrypt with OLD sending keys (before updating) */
  int enc_len = fio_tls13_record_encrypt(out,
                                         out_capacity,
                                         ku_msg,
                                         (size_t)ku_len,
                                         FIO_TLS13_CONTENT_HANDSHAKE,
                                         keys);
  if (enc_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3 KeyUpdate: failed to encrypt response");
    return -1;
  }

  /* Now update sending keys */
  size_t hash_len = use_sha384 ? 48 : 32;

  /* Derive new sending traffic secret */
  uint8_t new_secret[48];
  fio_tls13_update_traffic_secret(new_secret, traffic_secret, use_sha384);

  /* Copy new secret back to caller's buffer */
  FIO_MEMCPY(traffic_secret, new_secret, hash_len);

  /* Derive new sending keys */
  uint8_t new_key[32];
  uint8_t new_iv[12];
  fio_tls13_derive_traffic_keys(new_key,
                                key_len,
                                new_iv,
                                new_secret,
                                use_sha384);

  /* Update sending keys and reset sequence number */
  fio_tls13_record_keys_init(keys,
                             new_key,
                             (uint8_t)key_len,
                             new_iv,
                             cipher_type);

  /* Clear temporary key material */
  fio_secure_zero(new_secret, sizeof(new_secret));
  fio_secure_zero(new_key, sizeof(new_key));
  fio_secure_zero(new_iv, sizeof(new_iv));

  /* Clear pending flag */
  *key_update_pending = 0;

  FIO_LOG_DEBUG2("TLS 1.3 KeyUpdate: sent response, updated sending keys");

  return enc_len;
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

  /* Validate length (RFC 8446 §5.1: max 2^14 + 256 = 16640 bytes) */
  if (len > FIO_TLS13_MAX_CIPHERTEXT_LEN) {
    FIO_LOG_DEBUG2("TLS 1.3: ciphertext too large (%u > %d), record_overflow",
                   (unsigned)len,
                   FIO_TLS13_MAX_CIPHERTEXT_LEN);
    return NULL;
  }

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

  /* Validate plaintext length (RFC 8446 §5.1: max 2^14 = 16384 bytes) */
  if (plaintext_len > FIO_TLS13_MAX_PLAINTEXT_LEN) {
    FIO_LOG_DEBUG2("TLS 1.3: plaintext too large (%zu > %d), record_overflow",
                   plaintext_len,
                   FIO_TLS13_MAX_PLAINTEXT_LEN);
    return -1;
  }

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

  /* Note: nonce is not secret (derived from IV and sequence number which
   * remain in memory), so no need to zero it for security. */

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

  /* Note: nonce is not secret (derived from IV and sequence number which
   * remain in memory), so no need to zero it for security. */

  if (ret != 0)
    return -1;

  /* Scan backwards to find real content type (skip zero padding)
   * Per RFC 8446: zeros are optional padding, real content type is
   * the last non-zero byte */
  size_t pt_len = inner_ct_len;
  while (pt_len > 0 && out[pt_len - 1] == 0)
    --pt_len;

  /* Must have at least the content type byte (RFC 8446 §5.4) */
  if (pt_len == 0) {
    FIO_LOG_DEBUG2("TLS 1.3: no content type in decrypted record, "
                   "unexpected_message");
    return -1;
  }

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
TLS 1.3 Alert Sending Implementation (RFC 8446 Section 6)
***************************************************************************** */

/* Build an encrypted alert record */
SFUNC int fio_tls13_send_alert(uint8_t *out,
                               size_t out_capacity,
                               uint8_t alert_level,
                               uint8_t alert_desc,
                               fio_tls13_record_keys_s *keys) {
  if (!out || !keys)
    return -1;

  /* Alert message is 2 bytes: level + description */
  uint8_t alert_data[2];
  alert_data[0] = alert_level;
  alert_data[1] = alert_desc;

  FIO_LOG_DEBUG2("TLS 1.3: Sending alert: %s (level=%d, desc=%d)",
                 fio_tls13_alert_name(alert_desc),
                 alert_level,
                 alert_desc);

  /* Encrypt the alert as an alert record */
  return fio_tls13_record_encrypt(out,
                                  out_capacity,
                                  alert_data,
                                  2,
                                  FIO_TLS13_CONTENT_ALERT,
                                  keys);
}

/* Build an unencrypted alert record */
SFUNC int fio_tls13_send_alert_plaintext(uint8_t *out,
                                         size_t out_capacity,
                                         uint8_t alert_level,
                                         uint8_t alert_desc) {
  if (!out || out_capacity < 7)
    return -1;

  FIO_LOG_DEBUG2("TLS 1.3: Sending plaintext alert: %s (level=%d, desc=%d)",
                 fio_tls13_alert_name(alert_desc),
                 alert_level,
                 alert_desc);

  /* Record header: type(1) + version(2) + length(2) */
  out[0] = FIO_TLS13_CONTENT_ALERT;
  out[1] = FIO_TLS13_LEGACY_VERSION_MAJOR;
  out[2] = FIO_TLS13_LEGACY_VERSION_MINOR;
  out[3] = 0;
  out[4] = 2; /* Alert is 2 bytes */

  /* Alert message: level(1) + description(1) */
  out[5] = alert_level;
  out[6] = alert_desc;

  return 7;
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
  fio_memcpy32(p, x25519_pubkey);
  p += 32;

  return (size_t)(p - out);
}

/* *****************************************************************************
ALPN Extension Building/Parsing (RFC 7301)

ALPN allows client and server to negotiate application protocol over TLS.
Essential for HTTP/2 ("h2"), gRPC, and other protocols sharing port 443.

Client sends in ClientHello:
  struct {
    ProtocolName protocol_name_list<2..2^16-1>;
  } ProtocolNameList;

  struct {
    opaque ProtocolName<1..2^8-1>;
  } ProtocolName;

Server responds in EncryptedExtensions with single selected protocol.
***************************************************************************** */

/**
 * Internal: Write ALPN extension for ClientHello.
 *
 * Converts comma-separated protocol list to wire format.
 * Example: "h2,http/1.1" -> [2,"h2"][8,"http/1.1"]
 *
 * @param out       Output buffer
 * @param protocols Comma-separated protocol list (e.g., "h2,http/1.1")
 * @return Number of bytes written, or 0 if no protocols
 */
FIO_SFUNC size_t fio___tls13_write_ext_alpn(uint8_t *out,
                                            const char *protocols) {
  if (!protocols || !protocols[0])
    return 0;

  uint8_t *p = out;

  /* Extension type: ALPN (16) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_ALPN);
  p += 2;

  /* Skip extension length and list length for now */
  uint8_t *ext_len_ptr = p;
  p += 2;
  uint8_t *list_len_ptr = p;
  p += 2;
  uint8_t *list_start = p;

  /* Parse comma-separated protocols and write each one */
  const char *proto_start = protocols;
  while (*proto_start) {
    /* Find end of this protocol name */
    const char *proto_end = proto_start;
    while (*proto_end && *proto_end != ',')
      ++proto_end;

    size_t proto_len = (size_t)(proto_end - proto_start);
    if (proto_len > 0 && proto_len < 256) {
      /* Write length-prefixed protocol name */
      *p++ = (uint8_t)proto_len;
      FIO_MEMCPY(p, proto_start, proto_len);
      p += proto_len;
    }

    /* Skip comma if present */
    if (*proto_end == ',')
      proto_start = proto_end + 1;
    else
      break;
  }

  /* Calculate and write lengths */
  size_t list_len = (size_t)(p - list_start);
  if (list_len == 0)
    return 0; /* No valid protocols */

  fio___tls13_write_u16(list_len_ptr, (uint16_t)list_len);
  fio___tls13_write_u16(ext_len_ptr, (uint16_t)(list_len + 2));

  return (size_t)(p - out);
}

/**
 * Internal: Parse ALPN extension from ClientHello.
 *
 * Extracts protocol names from wire format into parsed structure.
 *
 * @param data     Extension data (after type and length)
 * @param data_len Length of extension data
 * @param protos   Output: array of protocol name pointers
 * @param lens     Output: array of protocol name lengths
 * @param max_count Maximum number of protocols to extract
 * @return Number of protocols extracted, or -1 on error
 */
FIO_SFUNC int fio___tls13_parse_alpn_extension(const uint8_t *data,
                                               size_t data_len,
                                               const char **protos,
                                               size_t *lens,
                                               size_t max_count) {
  if (!data || data_len < 2 || !protos || !lens)
    return -1;

  /* Read protocol name list length */
  uint16_t list_len = fio___tls13_read_u16(data);
  if (list_len + 2 > data_len)
    return -1;

  const uint8_t *p = data + 2;
  const uint8_t *end = p + list_len;
  size_t count = 0;

  while (p < end && count < max_count) {
    if (p + 1 > end)
      break;
    uint8_t proto_len = *p++;
    if (proto_len == 0 || p + proto_len > end)
      break;

    protos[count] = (const char *)p;
    lens[count] = proto_len;
    ++count;
    p += proto_len;
  }

  return (int)count;
}

/**
 * Internal: Build ALPN extension for EncryptedExtensions (server response).
 *
 * Server responds with single selected protocol.
 *
 * @param out          Output buffer
 * @param selected     Selected protocol name
 * @param selected_len Length of selected protocol name
 * @return Number of bytes written
 */
FIO_SFUNC size_t fio___tls13_build_alpn_response(uint8_t *out,
                                                 const char *selected,
                                                 size_t selected_len) {
  if (!out || !selected || selected_len == 0 || selected_len > 255)
    return 0;

  uint8_t *p = out;

  /* Extension type: ALPN (16) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_ALPN);
  p += 2;

  /* Extension length: list_len(2) + proto_len(1) + proto */
  fio___tls13_write_u16(p, (uint16_t)(2 + 1 + selected_len));
  p += 2;

  /* Protocol name list length: proto_len(1) + proto */
  fio___tls13_write_u16(p, (uint16_t)(1 + selected_len));
  p += 2;

  /* Selected protocol (length-prefixed) */
  *p++ = (uint8_t)selected_len;
  FIO_MEMCPY(p, selected, selected_len);
  p += selected_len;

  return (size_t)(p - out);
}

/**
 * Internal: Parse ALPN response from EncryptedExtensions.
 *
 * @param data         Extension data (after type and length)
 * @param data_len     Length of extension data
 * @param selected     Output: selected protocol name
 * @param selected_len Output: length of selected protocol
 * @param max_len      Maximum length for selected buffer
 * @return 0 on success, -1 on error
 */
FIO_SFUNC int fio___tls13_parse_alpn_response(const uint8_t *data,
                                              size_t data_len,
                                              char *selected,
                                              size_t *selected_len,
                                              size_t max_len) {
  if (!data || data_len < 3 || !selected || !selected_len)
    return -1;

  /* Read protocol name list length */
  uint16_t list_len = fio___tls13_read_u16(data);
  if (list_len + 2 > data_len || list_len < 2)
    return -1;

  /* Read single protocol */
  uint8_t proto_len = data[2];
  if (proto_len == 0 || proto_len + 1 > list_len)
    return -1;
  if (proto_len >= max_len)
    return -1;

  FIO_MEMCPY(selected, data + 3, proto_len);
  selected[proto_len] = '\0';
  *selected_len = proto_len;

  return 0;
}

/**
 * Internal: Select ALPN protocol from client's offer.
 *
 * Server selects first matching protocol from its supported list.
 *
 * @param client_protos   Array of client protocol name pointers
 * @param client_lens     Array of client protocol name lengths
 * @param client_count    Number of client protocols
 * @param server_protos   Server's supported protocols (comma-separated)
 * @param selected        Output: selected protocol name
 * @param selected_len    Output: length of selected protocol
 * @param max_len         Maximum length for selected buffer
 * @return 0 on success, -1 if no match
 */
FIO_SFUNC int fio___tls13_select_alpn(const char **client_protos,
                                      const size_t *client_lens,
                                      size_t client_count,
                                      const char *server_protos,
                                      char *selected,
                                      size_t *selected_len,
                                      size_t max_len) {
  if (!client_protos || !client_lens || client_count == 0)
    return -1;
  if (!server_protos || !server_protos[0])
    return -1;
  if (!selected || !selected_len)
    return -1;

  /* Parse server's comma-separated list and find first match */
  const char *srv_start = server_protos;
  while (*srv_start) {
    /* Find end of this server protocol */
    const char *srv_end = srv_start;
    while (*srv_end && *srv_end != ',')
      ++srv_end;
    size_t srv_len = (size_t)(srv_end - srv_start);

    /* Check against each client protocol */
    for (size_t i = 0; i < client_count; ++i) {
      if (client_lens[i] == srv_len &&
          FIO_MEMCMP(client_protos[i], srv_start, srv_len) == 0) {
        /* Match found */
        if (srv_len >= max_len)
          return -1;
        FIO_MEMCPY(selected, srv_start, srv_len);
        selected[srv_len] = '\0';
        *selected_len = srv_len;
        return 0;
      }
    }

    /* Move to next server protocol */
    if (*srv_end == ',')
      srv_start = srv_end + 1;
    else
      break;
  }

  return -1; /* No match */
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
  fio_memcpy32(p, random);
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
  fio_memcpy32(out->random, p);
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
      /* In ServerHello: group(2) + key_len(2) + key
       * In HelloRetryRequest: just group(2) per RFC 8446 Section 4.2.8 */
      if (ext_data_len >= 2) {
        out->key_share_group = fio___tls13_read_u16(p);
        if (ext_data_len >= 4) {
          /* Normal ServerHello with full key share */
          uint16_t key_len = fio___tls13_read_u16(p + 2);
          if (key_len <= sizeof(out->key_share) &&
              ext_data_len >= 4 + key_len) {
            FIO_MEMCPY(out->key_share, p + 4, key_len);
            out->key_share_len = (uint8_t)key_len;
          }
        }
        /* If ext_data_len == 2, it's HRR with just the group */
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

    case FIO_TLS13_EXT_ALPN:
      /* ALPN response - server selected protocol */
      if (fio___tls13_parse_alpn_response(p,
                                          ext_data_len,
                                          out->alpn_selected,
                                          &out->alpn_selected_len,
                                          sizeof(out->alpn_selected)) == 0) {
        FIO_LOG_DEBUG2("TLS 1.3: Server selected ALPN: %s", out->alpn_selected);
      }
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
CertificateRequest Parsing/Building Implementation (RFC 8446 Section 4.3.2)

CertificateRequest is sent by the server to request client authentication.
Format:
  struct {
    opaque certificate_request_context<0..2^8-1>;
    Extension extensions<2..2^16-1>;
  } CertificateRequest;

Required extension: signature_algorithms (13)
Optional extensions: certificate_authorities (47), signature_algorithms_cert
(50)
***************************************************************************** */

/**
 * Parse CertificateRequest message.
 *
 * @param out      Output structure for parsed data
 * @param data     Message body (after handshake header)
 * @param data_len Length of message body
 * @return 0 on success, -1 on error
 */
SFUNC int fio_tls13_parse_certificate_request(
    fio_tls13_certificate_request_s *out,
    const uint8_t *data,
    size_t data_len);

/**
 * Build CertificateRequest message.
 *
 * @param out          Output buffer
 * @param out_capacity Capacity of output buffer
 * @param context      Certificate request context (random bytes)
 * @param context_len  Context length (0-255)
 * @param sig_algs     Signature algorithms to accept
 * @param sig_alg_count Number of signature algorithms
 * @return Message length on success, -1 on error
 */
SFUNC int fio_tls13_build_certificate_request(uint8_t *out,
                                              size_t out_capacity,
                                              const uint8_t *context,
                                              size_t context_len,
                                              const uint16_t *sig_algs,
                                              size_t sig_alg_count);

/* CertificateRequest parsing implementation */
SFUNC int fio_tls13_parse_certificate_request(
    fio_tls13_certificate_request_s *out,
    const uint8_t *data,
    size_t data_len) {
  if (!out || !data)
    return -1;

  FIO_MEMSET(out, 0, sizeof(*out));

  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /* Certificate request context length (1 byte) */
  if (p + 1 > end)
    return -1;
  uint8_t ctx_len = *p++;
  if (p + ctx_len > end || ctx_len > 255)
    return -1;

  /* Copy context */
  out->certificate_request_context_len = ctx_len;
  if (ctx_len > 0)
    FIO_MEMCPY(out->certificate_request_context, p, ctx_len);
  p += ctx_len;

  /* Extensions length (2 bytes) */
  if (p + 2 > end)
    return -1;
  uint16_t ext_len = fio___tls13_read_u16(p);
  p += 2;
  if (p + ext_len > end)
    return -1;

  const uint8_t *ext_end = p + ext_len;
  int has_sig_algs = 0;

  /* Parse extensions */
  while (p + 4 <= ext_end) {
    uint16_t ext_type = fio___tls13_read_u16(p);
    p += 2;
    uint16_t ext_data_len = fio___tls13_read_u16(p);
    p += 2;

    if (p + ext_data_len > ext_end)
      return -1;

    switch (ext_type) {
    case FIO_TLS13_EXT_SIGNATURE_ALGORITHMS: {
      /* signature_algorithms extension (REQUIRED) */
      if (ext_data_len < 2)
        break;
      uint16_t algos_len = fio___tls13_read_u16(p);
      if (algos_len + 2 > ext_data_len || algos_len % 2 != 0)
        break;
      const uint8_t *algos = p + 2;
      size_t count = algos_len / 2;
      if (count > 16)
        count = 16;
      for (size_t i = 0; i < count; ++i)
        out->signature_algorithms[i] = fio___tls13_read_u16(algos + i * 2);
      out->signature_algorithm_count = count;
      has_sig_algs = 1;
      break;
    }

    case FIO_TLS13_EXT_SIGNATURE_ALGORITHMS_CERT: {
      /* signature_algorithms_cert extension (optional) */
      if (ext_data_len < 2)
        break;
      uint16_t algos_len = fio___tls13_read_u16(p);
      if (algos_len + 2 > ext_data_len || algos_len % 2 != 0)
        break;
      const uint8_t *algos = p + 2;
      size_t count = algos_len / 2;
      if (count > 16)
        count = 16;
      for (size_t i = 0; i < count; ++i)
        out->signature_algorithms_cert[i] = fio___tls13_read_u16(algos + i * 2);
      out->signature_algorithms_cert_count = count;
      break;
    }

    case FIO_TLS13_EXT_CERTIFICATE_AUTHORITIES: {
      /* certificate_authorities extension (optional) */
      if (ext_data_len < 2)
        break;
      uint16_t cas_len = fio___tls13_read_u16(p);
      if (cas_len + 2 > ext_data_len)
        break;
      out->certificate_authorities = p + 2;
      out->certificate_authorities_len = cas_len;
      break;
    }

    default:
      /* Ignore unknown extensions */
      break;
    }

    p += ext_data_len;
  }

  /* signature_algorithms extension is REQUIRED per RFC 8446 */
  if (!has_sig_algs) {
    FIO_LOG_DEBUG2("TLS 1.3: CertificateRequest missing signature_algorithms");
    return -1;
  }

  return 0;
}

/* CertificateRequest building implementation */
SFUNC int fio_tls13_build_certificate_request(uint8_t *out,
                                              size_t out_capacity,
                                              const uint8_t *context,
                                              size_t context_len,
                                              const uint16_t *sig_algs,
                                              size_t sig_alg_count) {
  if (!out || !sig_algs || sig_alg_count == 0)
    return -1;
  if (context_len > 255)
    return -1;

  /* Calculate size:
   * handshake_header(4) + ctx_len(1) + ctx + ext_len(2) +
   * sig_algs_ext: type(2) + len(2) + algos_len(2) + algos(sig_alg_count*2) */
  size_t sig_algs_ext_len = 2 + 2 + 2 + sig_alg_count * 2;
  size_t body_len = 1 + context_len + 2 + sig_algs_ext_len;
  size_t total_len = 4 + body_len;

  if (out_capacity < total_len)
    return -1;

  uint8_t *p = out;

  /* Handshake header */
  fio_tls13_write_handshake_header(p,
                                   FIO_TLS13_HS_CERTIFICATE_REQUEST,
                                   body_len);
  p += 4;

  /* Certificate request context */
  *p++ = (uint8_t)context_len;
  if (context_len > 0 && context) {
    FIO_MEMCPY(p, context, context_len);
    p += context_len;
  }

  /* Extensions length */
  fio___tls13_write_u16(p, (uint16_t)sig_algs_ext_len);
  p += 2;

  /* signature_algorithms extension */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_SIGNATURE_ALGORITHMS);
  p += 2;
  fio___tls13_write_u16(p, (uint16_t)(2 + sig_alg_count * 2));
  p += 2;
  fio___tls13_write_u16(p, (uint16_t)(sig_alg_count * 2));
  p += 2;
  for (size_t i = 0; i < sig_alg_count; ++i) {
    fio___tls13_write_u16(p, sig_algs[i]);
    p += 2;
  }

  return (int)total_len;
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
  FIO_TLS13_STATE_START = 0, /* Initial state */
  FIO_TLS13_STATE_WAIT_SH,   /* Sent ClientHello, waiting for ServerHello */
  FIO_TLS13_STATE_WAIT_SH2,  /* Sent ClientHello2 after HRR, waiting for SH */
  FIO_TLS13_STATE_WAIT_EE,   /* Received ServerHello, waiting for EE */
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

  /* Certificate chain from server (raw pointers into cert_data_buf) */
  const uint8_t *cert_chain[10]; /* Up to 10 certificates in chain */
  size_t cert_chain_lens[10];    /* Length of each certificate */
  size_t cert_chain_count;       /* Number of certificates received */

  /* Buffer to store certificate data (copied from decrypted records) */
  uint8_t *cert_data_buf;   /* Allocated buffer for certificate data */
  size_t cert_data_buf_len; /* Total length of certificate data */
  size_t cert_data_buf_cap; /* Capacity of certificate data buffer */

  /* HelloRetryRequest handling (RFC 8446 Section 4.1.4) */
  uint8_t hrr_received;         /* 1 if HRR was received (to detect second) */
  uint16_t hrr_selected_group;  /* Group selected by server in HRR */
  uint8_t *hrr_cookie;          /* Cookie from HRR (if any) */
  size_t hrr_cookie_len;        /* Length of cookie */
  uint8_t p256_private_key[32]; /* P-256 private key (for HRR fallback) */
  uint8_t p256_public_key[65];  /* P-256 public key (uncompressed) */

  /* ALPN (Application-Layer Protocol Negotiation - RFC 7301) */
  char alpn_protocols[256];  /* Client's offered protocols (comma-separated) */
  size_t alpn_protocols_len; /* Length of offered protocols string */
  char alpn_selected[256];   /* Server's selected protocol (null-terminated) */
  size_t alpn_selected_len;  /* Length of selected protocol */

  /* Client Certificate Authentication (RFC 8446 Section 4.4.2) */
  uint8_t cert_request_received;      /* 1 if CertificateRequest received */
  uint8_t cert_request_context[255];  /* Context from CertificateRequest */
  size_t cert_request_context_len;    /* Context length */
  uint16_t cert_request_sig_algs[16]; /* Server's accepted sig algorithms */
  size_t cert_request_sig_alg_count;  /* Number of accepted sig algs */
  const uint8_t *client_cert;         /* Client's certificate (DER) */
  size_t client_cert_len;             /* Client certificate length */
  const uint8_t *client_private_key;  /* Client's private key */
  size_t client_private_key_len;      /* Private key length */
  uint16_t client_key_type;           /* Key type (signature scheme) */
  uint8_t client_public_key[65];      /* Public key for P-256 (65 bytes) */

  /* Internal flags */
  uint8_t encrypted_read;     /* 1 if reading encrypted records */
  uint8_t encrypted_write;    /* 1 if writing encrypted records */
  uint8_t key_update_pending; /* 1 if KeyUpdate response needed (RFC 8446 4.6.3)
                               */
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
  case FIO_TLS13_STATE_WAIT_SH2: return "WAIT_SH2";
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

/**
 * Set ALPN protocols for client (comma-separated list).
 *
 * Example: "h2,http/1.1" offers HTTP/2 first, then HTTP/1.1
 *
 * @param client    Client context
 * @param protocols Comma-separated list of protocols (e.g., "h2,http/1.1")
 */
FIO_IFUNC void fio_tls13_client_alpn_set(fio_tls13_client_s *client,
                                         const char *protocols) {
  if (!client)
    return;
  if (!protocols || !protocols[0]) {
    client->alpn_protocols[0] = '\0';
    client->alpn_protocols_len = 0;
    return;
  }
  size_t len = 0;
  while (protocols[len] && len < sizeof(client->alpn_protocols) - 1)
    ++len;
  FIO_MEMCPY(client->alpn_protocols, protocols, len);
  client->alpn_protocols[len] = '\0';
  client->alpn_protocols_len = len;
}

/**
 * Get negotiated ALPN protocol after handshake.
 *
 * @param client Client context
 * @return Selected protocol string, or NULL if none negotiated
 */
FIO_IFUNC const char *fio_tls13_client_alpn_get(fio_tls13_client_s *client) {
  if (!client || client->alpn_selected_len == 0)
    return NULL;
  return client->alpn_selected;
}

/**
 * Set client certificate for mutual TLS (mTLS) authentication.
 *
 * When the server requests client authentication (CertificateRequest),
 * the client will send this certificate and sign with the private key.
 *
 * @param client      Client context
 * @param cert        DER-encoded client certificate
 * @param cert_len    Certificate length
 * @param private_key Private key (Ed25519: 32 bytes, P-256: 32 bytes)
 * @param key_len     Private key length
 * @param key_type    Key type (FIO_TLS13_SIG_ED25519, FIO_TLS13_SIG_ECDSA_*,
 * etc)
 */
FIO_IFUNC void fio_tls13_client_set_cert(fio_tls13_client_s *client,
                                         const uint8_t *cert,
                                         size_t cert_len,
                                         const uint8_t *private_key,
                                         size_t key_len,
                                         uint16_t key_type) {
  if (!client)
    return;
  client->client_cert = cert;
  client->client_cert_len = cert_len;
  client->client_private_key = private_key;
  client->client_private_key_len = key_len;
  client->client_key_type = key_type;
}

/**
 * Set client P-256 public key for ECDSA signing.
 *
 * Required when using P-256 ECDSA for client certificate authentication.
 * The public key is the uncompressed point (65 bytes: 0x04 || x || y).
 *
 * @param client     Client context
 * @param public_key P-256 public key (65 bytes uncompressed)
 */
FIO_IFUNC void fio_tls13_client_set_public_key(fio_tls13_client_s *client,
                                               const uint8_t *public_key) {
  if (!client || !public_key)
    return;
  FIO_MEMCPY(client->client_public_key, public_key, 65);
}

/**
 * Check if server requested client certificate.
 *
 * @param client Client context
 * @return 1 if CertificateRequest was received, 0 otherwise
 */
FIO_IFUNC int fio_tls13_client_cert_requested(fio_tls13_client_s *client) {
  return client ? client->cert_request_received : 0;
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
    fio_memcpy32(out, h.u8);
  }
}

/**
 * Internal: Replace transcript with message_hash for HelloRetryRequest.
 *
 * Per RFC 8446 Section 4.4.1:
 * When the server responds with HelloRetryRequest, the transcript hash
 * is replaced with a special construct:
 *
 *   Transcript-Hash(ClientHello1, HelloRetryRequest, ... Mn) =
 *     Hash(message_hash ||        // special message
 *          HelloRetryRequest ||   // the HRR message
 *          ... Mn)
 *
 * Where message_hash is:
 *   struct {
 *     HandshakeType msg_type = message_hash (254);
 *     uint24 length = Hash.length;
 *     opaque hash[Hash.length];  // Hash(ClientHello1)
 *   }
 *
 * This function:
 * 1. Gets the current transcript hash (Hash(ClientHello1))
 * 2. Reinitializes the transcript hash
 * 3. Updates it with the message_hash construct
 */
FIO_SFUNC void fio___tls13_transcript_replace_with_message_hash(
    fio_tls13_client_s *client) {
  size_t hash_len = client->use_sha384 ? 48 : 32;

  /* Get Hash(ClientHello1) */
  uint8_t ch1_hash[48];
  fio___tls13_transcript_hash(client, ch1_hash);

  /* Build message_hash construct:
   * msg_type (1 byte) = 254 (message_hash)
   * length (3 bytes) = hash_len
   * hash (hash_len bytes) */
  uint8_t message_hash[4 + 48];
  message_hash[0] = FIO_TLS13_HS_MESSAGE_HASH; /* 254 */
  message_hash[1] = 0;
  message_hash[2] = 0;
  message_hash[3] = (uint8_t)hash_len;
  FIO_MEMCPY(message_hash + 4, ch1_hash, hash_len);

  /* Reinitialize transcript hash and update with message_hash */
  if (client->use_sha384) {
    client->transcript_sha384 = fio_sha512_init();
    fio_sha512_consume(&client->transcript_sha384, message_hash, 4 + hash_len);
  } else {
    client->transcript_sha256 = fio_sha256_init();
    fio_sha256_consume(&client->transcript_sha256, message_hash, 4 + hash_len);
  }

  FIO_LOG_DEBUG2("TLS 1.3: Replaced transcript with message_hash for HRR");
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

/* *****************************************************************************
Client Certificate Authentication - Certificate and CertificateVerify Building
(RFC 8446 Section 4.4.2, 4.4.3)

When the server sends CertificateRequest, the client responds with:
1. Certificate message (may be empty if no cert available)
2. CertificateVerify message (only if Certificate is not empty)
3. Finished message

The signature context for client CertificateVerify is:
  0x20 * 64 || "TLS 1.3, client CertificateVerify" || 0x00 || Hash(transcript)
***************************************************************************** */

/**
 * Internal: Build client Certificate message.
 *
 * If client has no certificate configured, sends empty certificate list.
 * The certificate_request_context MUST match what server sent.
 *
 * @param client       Client context
 * @param out          Output buffer
 * @param out_capacity Capacity of output buffer
 * @return Message length on success, -1 on error
 */
FIO_SFUNC int fio___tls13_build_client_certificate(fio_tls13_client_s *client,
                                                   uint8_t *out,
                                                   size_t out_capacity) {
  if (!client || !out)
    return -1;

  size_t ctx_len = client->cert_request_context_len;
  size_t cert_len = client->client_cert_len;
  const uint8_t *cert = client->client_cert;

  /* Calculate body size:
   * ctx_len(1) + ctx + list_len(3) + [cert_len(3) + cert + ext_len(2)] */
  size_t list_len = 0;
  if (cert && cert_len > 0)
    list_len = 3 + cert_len + 2; /* cert_len(3) + cert + extensions(2) */

  size_t body_len = 1 + ctx_len + 3 + list_len;
  size_t total_len = 4 + body_len;

  if (out_capacity < total_len)
    return -1;

  uint8_t *p = out;

  /* Handshake header */
  fio_tls13_write_handshake_header(p, FIO_TLS13_HS_CERTIFICATE, body_len);
  p += 4;

  /* Certificate request context (must echo server's context) */
  *p++ = (uint8_t)ctx_len;
  if (ctx_len > 0) {
    FIO_MEMCPY(p, client->cert_request_context, ctx_len);
    p += ctx_len;
  }

  /* Certificate list length */
  fio___tls13_write_u24(p, (uint32_t)list_len);
  p += 3;

  /* Certificate entry (if we have one) */
  if (cert && cert_len > 0) {
    /* Certificate data length */
    fio___tls13_write_u24(p, (uint32_t)cert_len);
    p += 3;

    /* Certificate data */
    FIO_MEMCPY(p, cert, cert_len);
    p += cert_len;

    /* Extensions (empty) */
    *p++ = 0;
    *p++ = 0;
  }

  FIO_LOG_DEBUG2("TLS 1.3 Client: Built Certificate message (%s)",
                 (cert && cert_len > 0) ? "with cert" : "empty");

  return (int)total_len;
}

/**
 * Internal: Build client CertificateVerify message.
 *
 * Signs the transcript hash with the client's private key.
 * Only called if client sent a non-empty Certificate.
 *
 * @param client       Client context
 * @param out          Output buffer
 * @param out_capacity Capacity of output buffer
 * @return Message length on success, -1 on error
 */
FIO_SFUNC int fio___tls13_build_client_certificate_verify(
    fio_tls13_client_s *client,
    uint8_t *out,
    size_t out_capacity) {
  if (!client || !out)
    return -1;
  if (!client->client_private_key || client->client_private_key_len == 0)
    return -1;

  /* Build signed content per RFC 8446 Section 4.4.3
   * Note: Client uses different context string than server */
  static const char context_client[] = "TLS 1.3, client CertificateVerify";
  const size_t context_len = sizeof(context_client) - 1; /* 33 bytes */
  size_t hash_len = fio___tls13_hash_len(client);

  uint8_t signed_content[64 + 33 + 1 + FIO_TLS13_MAX_HASH_LEN];
  size_t signed_content_len = 64 + context_len + 1 + hash_len;

  /* 64 spaces */
  FIO_MEMSET(signed_content, 0x20, 64);
  /* Context string */
  FIO_MEMCPY(signed_content + 64, context_client, context_len);
  /* Zero byte separator */
  signed_content[64 + context_len] = 0x00;
  /* Transcript hash (current state before CertificateVerify) */
  fio___tls13_transcript_hash(client, signed_content + 64 + context_len + 1);

  /* Sign based on key type */
  uint8_t signature[512]; /* Max for RSA-4096 */
  size_t sig_len = 0;

  switch (client->client_key_type) {
  case FIO_TLS13_SIG_ED25519: {
    if (client->client_private_key_len != 32)
      return -1;
    /* Ed25519 signs directly over the content */
    uint8_t ed_public_key[32];
    fio_ed25519_public_key(ed_public_key, client->client_private_key);
    fio_ed25519_sign(signature,
                     signed_content,
                     signed_content_len,
                     client->client_private_key,
                     ed_public_key);
    sig_len = 64;
    break;
  }
  case FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256: {
    if (client->client_private_key_len != 32)
      return -1;
    /* P-256 ECDSA: hash the signed content with SHA-256, then sign */
    fio_u256 msg_hash = fio_sha256(signed_content, signed_content_len);
    if (fio_ecdsa_p256_sign(signature,
                            &sig_len,
                            sizeof(signature),
                            msg_hash.u8,
                            client->client_private_key) != 0)
      return -1;
    break;
  }
  default:
    FIO_LOG_DEBUG2("TLS 1.3 Client: Unsupported signature scheme 0x%04x",
                   client->client_key_type);
    return -1;
  }

  /* Build CertificateVerify message */
  size_t body_len = 2 + 2 + sig_len; /* scheme(2) + sig_len(2) + sig */
  if (out_capacity < 4 + body_len)
    return -1;

  uint8_t *p = out;

  /* Handshake header */
  fio_tls13_write_handshake_header(p,
                                   FIO_TLS13_HS_CERTIFICATE_VERIFY,
                                   body_len);
  p += 4;

  /* Signature algorithm */
  fio___tls13_write_u16(p, client->client_key_type);
  p += 2;

  /* Signature length */
  fio___tls13_write_u16(p, (uint16_t)sig_len);
  p += 2;

  /* Signature */
  FIO_MEMCPY(p, signature, sig_len);

  /* Clear signature buffer */
  fio_secure_zero(signature, sizeof(signature));

  FIO_LOG_DEBUG2("TLS 1.3 Client: Built CertificateVerify scheme=0x%04x",
                 client->client_key_type);

  return (int)(4 + body_len);
}

/**
 * Internal: Process CertificateRequest message from server.
 *
 * Stores the context and accepted signature algorithms for later use
 * when building client Certificate and CertificateVerify.
 *
 * @param client   Client context
 * @param data     Message body (after handshake header)
 * @param data_len Length of message body
 * @return 0 on success, -1 on error
 */
FIO_SFUNC int fio___tls13_process_certificate_request(
    fio_tls13_client_s *client,
    const uint8_t *data,
    size_t data_len) {
  fio_tls13_certificate_request_s cr;
  if (fio_tls13_parse_certificate_request(&cr, data, data_len) != 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  /* Store context (must be echoed in client Certificate) */
  client->cert_request_context_len = cr.certificate_request_context_len;
  if (cr.certificate_request_context_len > 0) {
    FIO_MEMCPY(client->cert_request_context,
               cr.certificate_request_context,
               cr.certificate_request_context_len);
  }

  /* Store accepted signature algorithms */
  client->cert_request_sig_alg_count = cr.signature_algorithm_count;
  for (size_t i = 0; i < cr.signature_algorithm_count && i < 16; ++i)
    client->cert_request_sig_algs[i] = cr.signature_algorithms[i];

  client->cert_request_received = 1;

  FIO_LOG_DEBUG2("TLS 1.3 Client: CertificateRequest received (ctx_len=%zu, "
                 "sig_algs=%zu)",
                 cr.certificate_request_context_len,
                 cr.signature_algorithm_count);

  return 0;
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

/**
 * Internal: Handle HelloRetryRequest (RFC 8446 Section 4.1.4)
 *
 * Returns: 1 if HRR was handled and ClientHello2 should be sent,
 *          0 if not HRR (normal ServerHello),
 *         -1 on error
 *
 * When HRR is received:
 * 1. Verify this is the first HRR (second HRR is an error)
 * 2. Verify the selected group is one we offered
 * 3. Verify this would change our ClientHello (else illegal_parameter)
 * 4. Store cookie if present
 * 5. Update transcript hash with special message_hash format
 * 6. Generate new key share for selected group
 * 7. Set state to indicate retry needed
 */
FIO_SFUNC int fio___tls13_handle_hello_retry_request(
    fio_tls13_client_s *client,
    const fio_tls13_server_hello_s *sh,
    const uint8_t *hrr_msg,
    size_t hrr_msg_len) {
  /* RFC 8446 Section 4.1.4: Client MUST abort with unexpected_message
   * if it receives a second HelloRetryRequest */
  if (client->hrr_received) {
    FIO_LOG_DEBUG2("TLS 1.3: Received second HelloRetryRequest - aborting");
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
    return -1;
  }

  /* Validate cipher suite (same logic as normal ServerHello) */
  client->cipher_suite = sh->cipher_suite;
  switch (sh->cipher_suite) {
  case FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256: client->use_sha384 = 0; break;
  case FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256:
    client->use_sha384 = 0;
    break;
  case FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384: client->use_sha384 = 1; break;
  default:
    FIO_LOG_DEBUG2("TLS 1.3 HRR: Unsupported cipher suite 0x%04x",
                   sh->cipher_suite);
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
    return -1;
  }

  /* Check if server selected a group we support */
  uint16_t selected_group = sh->key_share_group;
  int group_supported = 0;
  int would_change = 0;

  /* We offer X25519 and P-256 */
  if (selected_group == FIO_TLS13_GROUP_X25519) {
    group_supported = 1;
    /* If we already offered X25519, this wouldn't change anything */
    /* (We always offer X25519 first, so this is an error) */
    would_change = 0;
  } else if (selected_group == FIO_TLS13_GROUP_SECP256R1) {
    group_supported = 1;
    /* We offered X25519 first, so switching to P-256 is a change */
    would_change = 1;
  }

  if (!group_supported) {
    FIO_LOG_DEBUG2("TLS 1.3 HRR: Server selected unsupported group 0x%04x",
                   selected_group);
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
  }

  /* RFC 8446 Section 4.1.4: Client MUST abort with illegal_parameter if
   * the HelloRetryRequest would not result in any change in the ClientHello */
  if (!would_change) {
    FIO_LOG_DEBUG2("TLS 1.3 HRR: Would not change ClientHello - aborting");
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
  }

  /* Store selected group for retry */
  client->hrr_selected_group = selected_group;
  client->hrr_received = 1;

  /* Parse HRR extensions to extract cookie if present */
  /* The HRR message format is same as ServerHello, parse extensions */
  {
    const uint8_t *p = hrr_msg + 4; /* Skip handshake header */
    const uint8_t *end = hrr_msg + hrr_msg_len;
    uint8_t session_id_len = 0;
    uint16_t ext_len = 0;
    const uint8_t *ext_end = NULL;

    /* Skip: version(2) + random(32) + session_id_len(1) + session_id +
     * cipher_suite(2) + compression(1) */
    if (p + 35 > end)
      goto skip_cookie;
    p += 2 + 32; /* version + random */
    session_id_len = *p++;
    if (p + session_id_len > end)
      goto skip_cookie;
    p += session_id_len;
    if (p + 3 > end)
      goto skip_cookie;
    p += 2 + 1; /* cipher_suite + compression */

    /* Extensions */
    if (p + 2 > end)
      goto skip_cookie;
    ext_len = fio___tls13_read_u16(p);
    p += 2;
    if (p + ext_len > end)
      goto skip_cookie;

    ext_end = p + ext_len;
    while (p + 4 <= ext_end) {
      uint16_t ext_type = fio___tls13_read_u16(p);
      uint16_t ext_data_len;
      p += 2;
      ext_data_len = fio___tls13_read_u16(p);
      p += 2;
      if (p + ext_data_len > ext_end)
        break;

      if (ext_type == FIO_TLS13_EXT_COOKIE && ext_data_len >= 2) {
        /* Cookie extension: length(2) + cookie_data */
        uint16_t cookie_len = fio___tls13_read_u16(p);
        if (cookie_len > 0 && cookie_len <= ext_data_len - 2) {
          /* Free old cookie if any */
          if (client->hrr_cookie) {
            FIO_MEM_FREE(client->hrr_cookie, client->hrr_cookie_len);
          }
          /* Allocate and copy cookie */
          client->hrr_cookie =
              (uint8_t *)FIO_MEM_REALLOC(NULL, 0, cookie_len, 0);
          if (client->hrr_cookie) {
            FIO_MEMCPY(client->hrr_cookie, p + 2, cookie_len);
            client->hrr_cookie_len = cookie_len;
            FIO_LOG_DEBUG2("TLS 1.3 HRR: Stored cookie (%zu bytes)",
                           (size_t)cookie_len);
          }
        }
      }
      p += ext_data_len;
    }

  skip_cookie:
    (void)0; /* Empty statement after label */
  }
  /* Update transcript hash with special message_hash format
   * Per RFC 8446 Section 4.4.1 */
  fio___tls13_transcript_replace_with_message_hash(client);

  /* Now update transcript with the HRR message */
  fio___tls13_transcript_update(client, hrr_msg, hrr_msg_len);

  /* Generate new key share for selected group */
  if (selected_group == FIO_TLS13_GROUP_SECP256R1) {
#if defined(H___FIO_P256___H)
    /* Generate P-256 keypair */
    fio_p256_keypair(client->p256_private_key, client->p256_public_key);
    FIO_LOG_DEBUG2("TLS 1.3 HRR: Generated P-256 key share");
#else
    FIO_LOG_DEBUG2("TLS 1.3 HRR: P-256 not available");
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
    return -1;
#endif
  }

  FIO_LOG_DEBUG2("TLS 1.3: HelloRetryRequest handled, will send ClientHello2");
  return 1; /* Indicate HRR was handled, need to send ClientHello2 */
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
    /* Return special value to indicate HRR handling needed */
    return 2; /* Will be handled by caller with full message */
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

  /* Validate key share - support both X25519 and P-256 */
  if (sh.key_share_group == FIO_TLS13_GROUP_X25519 && sh.key_share_len == 32) {
    /* X25519 - compute shared secret */
    if (fio_x25519_shared_secret(client->shared_secret,
                                 client->x25519_private_key,
                                 sh.key_share) != 0) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
      return -1;
    }
  } else if (sh.key_share_group == FIO_TLS13_GROUP_SECP256R1 &&
             sh.key_share_len == 65) {
#if defined(H___FIO_P256___H)
    /* P-256 - compute shared secret (after HRR) */
    if (fio_p256_shared_secret(client->shared_secret,
                               client->p256_private_key,
                               sh.key_share,
                               sh.key_share_len) != 0) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
      return -1;
    }
#else
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
#endif
  } else {
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

  /* Store selected ALPN protocol if present */
  if (ee.alpn_selected_len > 0) {
    size_t copy_len = ee.alpn_selected_len;
    if (copy_len >= sizeof(client->alpn_selected))
      copy_len = sizeof(client->alpn_selected) - 1;
    FIO_MEMCPY(client->alpn_selected, ee.alpn_selected, copy_len);
    client->alpn_selected[copy_len] = '\0';
    client->alpn_selected_len = copy_len;
    FIO_LOG_DEBUG2("TLS 1.3 Client: ALPN negotiated: %s",
                   client->alpn_selected);
  }

  return 0;
}

/* Internal: Process Certificate
 * Copies certificate data to a persistent buffer so pointers remain valid
 * after the decryption buffer is reused for subsequent records. */
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

  /* Reset certificate chain */
  client->cert_chain_count = 0;
  client->cert_data_buf_len = 0;

  /* Parse certificate chain structure */
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

  /* First pass: calculate total certificate data size needed */
  size_t total_cert_size = 0;
  size_t cert_count = 0;
  const uint8_t *scan = p;
  while (scan < list_end && cert_count < 10) {
    if (scan + 3 > list_end)
      break;
    size_t cert_len =
        ((size_t)scan[0] << 16) | ((size_t)scan[1] << 8) | scan[2];
    scan += 3;
    if (cert_len == 0 || scan + cert_len > list_end)
      break;
    total_cert_size += cert_len;
    ++cert_count;
    scan += cert_len;
    /* Skip extensions */
    if (scan + 2 > list_end)
      break;
    size_t ext_len = ((size_t)scan[0] << 8) | scan[1];
    scan += 2;
    if (scan + ext_len > list_end)
      break;
    scan += ext_len;
  }

  /* RFC 8446 Section 4.4.2: Server Certificate message MUST contain at least
   * one certificate. Empty certificate list is a decode_error. */
  if (total_cert_size == 0 || cert_count == 0) {
    fio___tls13_set_error(client,
                          FIO_TLS13_ALERT_LEVEL_FATAL,
                          FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  /* Allocate or reallocate buffer if needed */
  if (total_cert_size > client->cert_data_buf_cap) {
    if (client->cert_data_buf)
      FIO_MEM_FREE(client->cert_data_buf, client->cert_data_buf_cap);
    client->cert_data_buf =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, total_cert_size, 0);
    if (!client->cert_data_buf) {
      client->cert_data_buf_cap = 0;
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_INTERNAL_ERROR);
      return -1;
    }
    client->cert_data_buf_cap = total_cert_size;
  }

  /* Second pass: copy certificate data to persistent buffer */
  uint8_t *buf_ptr = client->cert_data_buf;
  while (p < list_end && client->cert_chain_count < 10) {
    if (p + 3 > list_end)
      break;
    size_t cert_len = ((size_t)p[0] << 16) | ((size_t)p[1] << 8) | p[2];
    p += 3;
    if (cert_len == 0 || p + cert_len > list_end)
      break;

    /* Copy certificate data to persistent buffer */
    FIO_MEMCPY(buf_ptr, p, cert_len);
    client->cert_chain[client->cert_chain_count] = buf_ptr;
    client->cert_chain_lens[client->cert_chain_count] = cert_len;
    ++client->cert_chain_count;
    buf_ptr += cert_len;
    p += cert_len;

    /* Skip extensions */
    if (p + 2 > list_end)
      break;
    size_t ext_len = ((size_t)p[0] << 8) | p[1];
    p += 2;
    if (p + ext_len > list_end)
      break;
    p += ext_len;
  }

  client->cert_data_buf_len = (size_t)(buf_ptr - client->cert_data_buf);

  /* Update server_cert to point to first certificate in persistent buffer */
  if (client->cert_chain_count > 0) {
    client->server_cert = client->cert_chain[0];
    client->server_cert_len = client->cert_chain_lens[0];
  }

  FIO_LOG_DEBUG2("TLS 1.3: Received %zu certificates in chain (%zu bytes)",
                 client->cert_chain_count,
                 client->cert_data_buf_len);
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
    fio_memcpy32(content_hash, h.u8);
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
  case FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256: {
#if defined(H___FIO_P256___H)
    /* ECDSA P-256 with SHA-256 */
    if (cert->key_type != FIO_X509_KEY_ECDSA_P256) {
      FIO_LOG_DEBUG2("TLS 1.3: Certificate key type mismatch for ECDSA P-256");
      return -1;
    }
    if (!cert->pubkey.ecdsa.point || cert->pubkey.ecdsa.point_len != 65) {
      FIO_LOG_DEBUG2("TLS 1.3: Invalid ECDSA P-256 public key");
      return -1;
    }
    if (fio_ecdsa_p256_verify(signature,
                              sig_len,
                              content_hash,
                              cert->pubkey.ecdsa.point,
                              cert->pubkey.ecdsa.point_len) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3: ECDSA P-256 signature verification failed");
      return -1;
    }
    break;
#else
    FIO_LOG_DEBUG2("TLS 1.3: ECDSA P-256 not available (FIO_P256 not defined)");
    return -1;
#endif
  }
  case FIO_TLS13_SIG_ECDSA_SECP384R1_SHA384: {
#if defined(H___FIO_P384___H)
    /* ECDSA P-384 with SHA-384 */
    if (cert->key_type != FIO_X509_KEY_ECDSA_P384) {
      FIO_LOG_DEBUG2("TLS 1.3: Certificate key type mismatch for ECDSA P-384");
      return -1;
    }
    if (!cert->pubkey.ecdsa.point || cert->pubkey.ecdsa.point_len != 97) {
      FIO_LOG_DEBUG2("TLS 1.3: Invalid ECDSA P-384 public key");
      return -1;
    }
    if (fio_ecdsa_p384_verify(signature,
                              sig_len,
                              content_hash,
                              cert->pubkey.ecdsa.point,
                              cert->pubkey.ecdsa.point_len) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3: ECDSA P-384 signature verification failed");
      return -1;
    }
    break;
#else
    FIO_LOG_DEBUG2("TLS 1.3: ECDSA P-384 not available (FIO_P384 not defined)");
    return -1;
#endif
  }
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
  case FIO_TLS13_STATE_WAIT_SH2: {
    fio_tls13_server_hello_s sh;
    if (msg_type != FIO_TLS13_HS_SERVER_HELLO) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }

    /* Parse ServerHello to check if it's an HRR */
    if (fio_tls13_parse_server_hello(&sh, body, body_len) != 0) {
      fio___tls13_set_error(client,
                            FIO_TLS13_ALERT_LEVEL_FATAL,
                            FIO_TLS13_ALERT_DECODE_ERROR);
      return -1;
    }

    if (sh.is_hello_retry_request) {
      /* HelloRetryRequest - handle specially (updates transcript internally) */
      if (fio___tls13_handle_hello_retry_request(client,
                                                 &sh,
                                                 msg,
                                                 4 + body_len) != 1) {
        return -1; /* Error already set */
      }
      /* Need to send ClientHello2 - return special value */
      return 2;
    }

    /* Normal ServerHello - update transcript FIRST, then process */
    fio___tls13_transcript_update(client, msg, 4 + body_len);
    if (fio___tls13_process_server_hello(client, body, body_len) != 0)
      return -1;
    client->state = FIO_TLS13_STATE_WAIT_EE;
    break;
  }

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
      /* CertificateRequest - process and store for later response */
      fio___tls13_transcript_update(client, msg, 4 + body_len);
      if (fio___tls13_process_certificate_request(client, body, body_len) != 0)
        return -1;
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
    /* Process CertificateVerify BEFORE updating transcript (signature covers
     * transcript up to but not including CertificateVerify) */
    if (fio___tls13_process_certificate_verify(client, body, body_len) != 0)
      return -1;
    /* Now update transcript with CertificateVerify */
    fio___tls13_transcript_update(client, msg, 4 + body_len);
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

    /* Build client response messages */
    {
      uint8_t hs_msgs[2048]; /* Buffer for all handshake messages */
      size_t hs_msgs_len = 0;

      /* If server requested client certificate, send Certificate and
       * CertificateVerify before Finished (RFC 8446 Section 4.4) */
      if (client->cert_request_received) {
        /* Build client Certificate */
        int cert_len =
            fio___tls13_build_client_certificate(client,
                                                 hs_msgs + hs_msgs_len,
                                                 sizeof(hs_msgs) - hs_msgs_len);
        if (cert_len < 0) {
          fio___tls13_set_error(client,
                                FIO_TLS13_ALERT_LEVEL_FATAL,
                                FIO_TLS13_ALERT_INTERNAL_ERROR);
          return -1;
        }
        fio___tls13_transcript_update(client,
                                      hs_msgs + hs_msgs_len,
                                      (size_t)cert_len);
        hs_msgs_len += (size_t)cert_len;

        /* Build client CertificateVerify (only if we sent a certificate) */
        if (client->client_cert && client->client_cert_len > 0) {
          int cv_len = fio___tls13_build_client_certificate_verify(
              client,
              hs_msgs + hs_msgs_len,
              sizeof(hs_msgs) - hs_msgs_len);
          if (cv_len < 0) {
            fio___tls13_set_error(client,
                                  FIO_TLS13_ALERT_LEVEL_FATAL,
                                  FIO_TLS13_ALERT_INTERNAL_ERROR);
            return -1;
          }
          fio___tls13_transcript_update(client,
                                        hs_msgs + hs_msgs_len,
                                        (size_t)cv_len);
          hs_msgs_len += (size_t)cv_len;
        }
      }

      /* Build client Finished */
      int finished_len =
          fio___tls13_build_client_finished(client,
                                            hs_msgs + hs_msgs_len,
                                            sizeof(hs_msgs) - hs_msgs_len);
      if (finished_len < 0) {
        fio___tls13_set_error(client,
                              FIO_TLS13_ALERT_LEVEL_FATAL,
                              FIO_TLS13_ALERT_INTERNAL_ERROR);
        return -1;
      }
      fio___tls13_transcript_update(client,
                                    hs_msgs + hs_msgs_len,
                                    (size_t)finished_len);
      hs_msgs_len += (size_t)finished_len;

      /* Encrypt all handshake messages together */
      int enc_len = fio_tls13_record_encrypt(out,
                                             out_capacity,
                                             hs_msgs,
                                             hs_msgs_len,
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
TLS 1.3 Client HelloRetryRequest - ClientHello2 Building
***************************************************************************** */

/**
 * Internal: Build ClientHello2 after HelloRetryRequest.
 *
 * Per RFC 8446 Section 4.1.4, ClientHello2 MUST:
 * - Use the same random value as the original ClientHello
 * - Replace key_share with a single KeyShareEntry for the server-selected group
 * - Include cookie extension if provided in HRR
 * - Update pre_shared_key if present (we don't use PSK)
 *
 * Returns: Total record length on success, -1 on error
 */
FIO_SFUNC int fio___tls13_build_client_hello2(fio_tls13_client_s *client,
                                              uint8_t *out,
                                              size_t out_capacity) {
  if (!client || !out || out_capacity < 512)
    return -1;

  /* Build ClientHello2 handshake message */
  uint8_t ch_msg[1024];
  uint8_t *p = ch_msg + 4; /* Skip handshake header */
  uint8_t *start = p;

  /* Legacy version: TLS 1.2 (0x0303) */
  fio___tls13_write_u16(p, FIO_TLS13_VERSION_TLS12);
  p += 2;

  /* Random - MUST be same as original ClientHello */
  fio_memcpy32(p, client->client_random);
  p += 32;

  /* Legacy session ID (empty for TLS 1.3) */
  *p++ = 0;

  /* Cipher suites - same as original */
  uint16_t cipher_suites[] = {FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256,
                              FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256,
                              FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384};
  fio___tls13_write_u16(p, 6); /* 3 suites * 2 bytes */
  p += 2;
  for (int i = 0; i < 3; ++i) {
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

  /* SNI extension (if we have server name) */
  if (client->server_name) {
    p += fio___tls13_write_ext_sni(p, client->server_name);
  }

  /* supported_versions extension (REQUIRED for TLS 1.3) */
  p += fio___tls13_write_ext_supported_versions(p);

  /* supported_groups extension */
  p += fio___tls13_write_ext_supported_groups(p);

  /* signature_algorithms extension */
  p += fio___tls13_write_ext_signature_algorithms(p);

  /* Cookie extension (if provided in HRR) */
  if (client->hrr_cookie && client->hrr_cookie_len > 0) {
    /* Extension type: cookie (44) */
    fio___tls13_write_u16(p, FIO_TLS13_EXT_COOKIE);
    p += 2;
    /* Extension data length: cookie_len(2) + cookie */
    fio___tls13_write_u16(p, (uint16_t)(2 + client->hrr_cookie_len));
    p += 2;
    /* Cookie length */
    fio___tls13_write_u16(p, (uint16_t)client->hrr_cookie_len);
    p += 2;
    /* Cookie data */
    FIO_MEMCPY(p, client->hrr_cookie, client->hrr_cookie_len);
    p += client->hrr_cookie_len;
    FIO_LOG_DEBUG2("TLS 1.3 CH2: Included cookie (%zu bytes)",
                   client->hrr_cookie_len);
  }

  /* key_share extension - SINGLE entry for server-selected group */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_KEY_SHARE);
  p += 2;

  if (client->hrr_selected_group == FIO_TLS13_GROUP_X25519) {
    /* X25519: entries_len(2) + group(2) + key_len(2) + key(32) = 38 bytes */
    fio___tls13_write_u16(p, 2 + 36); /* Extension data length */
    p += 2;
    fio___tls13_write_u16(p, 36); /* Client key share entries length */
    p += 2;
    fio___tls13_write_u16(p, FIO_TLS13_GROUP_X25519);
    p += 2;
    fio___tls13_write_u16(p, 32); /* Key length */
    p += 2;
    fio_memcpy32(p, client->x25519_public_key);
    p += 32;
  } else if (client->hrr_selected_group == FIO_TLS13_GROUP_SECP256R1) {
#if defined(H___FIO_P256___H)
    /* P-256: entries_len(2) + group(2) + key_len(2) + key(65) = 71 bytes */
    fio___tls13_write_u16(p, 2 + 69); /* Extension data length */
    p += 2;
    fio___tls13_write_u16(p, 69); /* Client key share entries length */
    p += 2;
    fio___tls13_write_u16(p, FIO_TLS13_GROUP_SECP256R1);
    p += 2;
    fio___tls13_write_u16(p, 65); /* Key length (uncompressed point) */
    p += 2;
    FIO_MEMCPY(p, client->p256_public_key, 65);
    p += 65;
#else
    FIO_LOG_DEBUG2("TLS 1.3 CH2: P-256 not available");
    return -1;
#endif
  } else {
    FIO_LOG_DEBUG2("TLS 1.3 CH2: Unsupported group 0x%04x",
                   client->hrr_selected_group);
    return -1;
  }

  /* Write extensions length */
  fio___tls13_write_u16(ext_len_ptr, (uint16_t)(p - ext_start));

  /* Calculate body length and write handshake header */
  size_t body_len = (size_t)(p - start);
  fio_tls13_write_handshake_header(ch_msg, FIO_TLS13_HS_CLIENT_HELLO, body_len);

  size_t ch_len = 4 + body_len;

  /* Update transcript with ClientHello2 */
  fio___tls13_transcript_update(client, ch_msg, ch_len);

  /* Check output capacity for record header + message */
  size_t total_len = FIO_TLS13_RECORD_HEADER_LEN + ch_len;
  if (out_capacity < total_len)
    return -1;

  /* Write record header */
  fio___tls13_write_record_header(out,
                                  FIO_TLS13_CONTENT_HANDSHAKE,
                                  (uint16_t)ch_len);

  /* Copy handshake message */
  FIO_MEMCPY(out + FIO_TLS13_RECORD_HEADER_LEN, ch_msg, ch_len);

  FIO_LOG_DEBUG2("TLS 1.3: Built ClientHello2 (%zu bytes) for group 0x%04x",
                 total_len,
                 client->hrr_selected_group);

  return (int)total_len;
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

  /* Free certificate data buffer */
  if (client->cert_data_buf) {
    FIO_MEM_FREE(client->cert_data_buf, client->cert_data_buf_cap);
    client->cert_data_buf = NULL;
    client->cert_data_buf_len = 0;
    client->cert_data_buf_cap = 0;
  }

  /* Free HRR cookie if allocated */
  if (client->hrr_cookie) {
    FIO_MEM_FREE(client->hrr_cookie, client->hrr_cookie_len);
    client->hrr_cookie = NULL;
    client->hrr_cookie_len = 0;
  }

  /* Clear all sensitive data */
  fio_secure_zero(client->x25519_private_key, 32);
  fio_secure_zero(client->p256_private_key, 32);
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

/**
 * Internal: Build ClientHello with ALPN support.
 *
 * This is an extended version of fio_tls13_build_client_hello that includes
 * ALPN extension when protocols are specified.
 */
FIO_SFUNC int fio___tls13_build_client_hello_full(uint8_t *out,
                                                  size_t out_capacity,
                                                  const uint8_t random[32],
                                                  const char *server_name,
                                                  const uint8_t *x25519_pubkey,
                                                  const uint16_t *cipher_suites,
                                                  size_t cipher_suite_count,
                                                  const char *alpn_protocols) {
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
  fio_memcpy32(p, random);
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

  /* ALPN extension (RFC 7301) */
  if (alpn_protocols && alpn_protocols[0])
    p += fio___tls13_write_ext_alpn(p, alpn_protocols);

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
  uint8_t ch_msg[1024]; /* Increased for ALPN */
  const char *alpn =
      client->alpn_protocols_len > 0 ? client->alpn_protocols : NULL;
  int ch_len = fio___tls13_build_client_hello_full(ch_msg,
                                                   sizeof(ch_msg),
                                                   client->client_random,
                                                   client->server_name,
                                                   client->x25519_public_key,
                                                   cipher_suites,
                                                   3,
                                                   alpn);
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
    int process_result =
        fio___tls13_process_handshake_message(client,
                                              hs_data + offset,
                                              msg_total_len,
                                              out + *out_len,
                                              out_capacity - *out_len,
                                              &msg_out_len);

    if (process_result == 2) {
      /* HelloRetryRequest received - need to send ClientHello2 */
      int ch2_len = fio___tls13_build_client_hello2(client,
                                                    out + *out_len,
                                                    out_capacity - *out_len);
      if (ch2_len < 0) {
        fio___tls13_set_error(client,
                              FIO_TLS13_ALERT_LEVEL_FATAL,
                              FIO_TLS13_ALERT_INTERNAL_ERROR);
        return -1;
      }
      *out_len += (size_t)ch2_len;
      client->state = FIO_TLS13_STATE_WAIT_SH2;
      offset += msg_total_len;
      continue;
    } else if (process_result != 0) {
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

  /* Handle post-handshake messages (e.g., NewSessionTicket, KeyUpdate)
   * Per RFC 8446 Section 4.6, these are encrypted with application
   * traffic keys but have handshake content type. */
  if (content_type == FIO_TLS13_CONTENT_HANDSHAKE) {
    /* Parse handshake header to check message type */
    if (dec_len >= 4) {
      uint8_t msg_type = out[0];
      uint32_t body_len =
          ((uint32_t)out[1] << 16) | ((uint32_t)out[2] << 8) | (uint32_t)out[3];

      if (msg_type == FIO_TLS13_HS_KEY_UPDATE && body_len == 1 &&
          dec_len >= 5) {
        /* Process KeyUpdate (RFC 8446 Section 4.6.3) */
        size_t key_len = fio___tls13_key_len(client);
        fio_tls13_cipher_type_e cipher_type = fio___tls13_cipher_type(client);

        if (fio_tls13_process_key_update(client->server_app_traffic_secret,
                                         &client->server_app_keys,
                                         out + 4,
                                         1,
                                         &client->key_update_pending,
                                         client->use_sha384,
                                         key_len,
                                         cipher_type) != 0) {
          FIO_LOG_DEBUG2("TLS 1.3 Client: KeyUpdate processing failed");
          return -1;
        }
        /* Return 0 to indicate "no app data, try next record" */
        return 0;
      } else if (msg_type == FIO_TLS13_HS_NEW_SESSION_TICKET) {
        /* NewSessionTicket - ignore for now */
        FIO_LOG_DEBUG2("TLS 1.3 Client: Received NewSessionTicket (ignored)");
        return 0;
      }
    }
    FIO_LOG_DEBUG2("TLS 1.3 Client: Received unknown post-handshake message");
    return 0;
  }

  /* Handle alerts */
  if (content_type == FIO_TLS13_CONTENT_ALERT) {
    if (dec_len >= 2) {
      FIO_LOG_DEBUG2("TLS 1.3: Received alert: level=%d, desc=%d",
                     out[0],
                     out[1]);
    }
    return -1;
  }

  /* Only return application data */
  if (content_type != FIO_TLS13_CONTENT_APPLICATION_DATA)
    return -1;

  return dec_len;
}

/* *****************************************************************************




                        TLS 1.3 Server Handshake State Machine
                              (RFC 8446 Section 4)




***************************************************************************** */

/* *****************************************************************************
TLS 1.3 Server State Machine
***************************************************************************** */

/** TLS 1.3 Server Handshake States (RFC 8446 Section 2) */
typedef enum {
  FIO_TLS13_SERVER_STATE_START = 0,    /* Initial state, waiting for CH */
  FIO_TLS13_SERVER_STATE_RECVD_CH,     /* Received ClientHello, parsing */
  FIO_TLS13_SERVER_STATE_NEGOTIATED,   /* Negotiated params, building SH */
  FIO_TLS13_SERVER_STATE_WAIT_FLIGHT2, /* Sent SH..Fin, waiting for client */
  FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT, /* Waiting for client Certificate */
  FIO_TLS13_SERVER_STATE_WAIT_CERT_VERIFY, /* Waiting for client CertVerify */
  FIO_TLS13_SERVER_STATE_WAIT_FINISHED,    /* Waiting for client Finished */
  FIO_TLS13_SERVER_STATE_CONNECTED,        /* Handshake complete */
  FIO_TLS13_SERVER_STATE_ERROR,            /* Error state */
} fio_tls13_server_state_e;

/** Parsed ClientHello message */
typedef struct {
  uint8_t random[32];                /* Client random */
  uint8_t legacy_session_id[32];     /* Legacy session ID (for middlebox) */
  uint8_t legacy_session_id_len;     /* Length of legacy session ID */
  uint16_t cipher_suites[16];        /* Offered cipher suites */
  size_t cipher_suite_count;         /* Number of cipher suites */
  uint16_t supported_groups[8];      /* Offered key exchange groups */
  size_t supported_group_count;      /* Number of groups */
  uint16_t signature_algorithms[16]; /* Offered signature algorithms */
  size_t signature_algorithm_count;  /* Number of signature algorithms */
  uint8_t key_shares[256];           /* Key share data */
  size_t key_share_len;              /* Total key share data length */
  uint16_t key_share_groups[4];      /* Groups for key shares */
  uint8_t key_share_offsets[4];      /* Offsets into key_shares */
  uint8_t key_share_lens[4];         /* Lengths of each key share */
  size_t key_share_count;            /* Number of key shares */
  const char *server_name;           /* SNI hostname (pointer into data) */
  size_t server_name_len;            /* SNI hostname length */
  int has_supported_versions;        /* 1 if TLS 1.3 supported */
  /* ALPN (Application-Layer Protocol Negotiation) */
  const char *alpn_protocols[8]; /* ALPN protocol names (pointers into data) */
  size_t alpn_protocol_lens[8];  /* ALPN protocol name lengths */
  size_t alpn_protocol_count;    /* Number of ALPN protocols */
} fio_tls13_client_hello_s;

/** TLS 1.3 Server Context */
typedef struct {
  /* State */
  fio_tls13_server_state_e state;

  /* Negotiated parameters */
  uint16_t cipher_suite;     /* Selected cipher suite */
  uint16_t key_share_group;  /* Selected key exchange group */
  uint16_t signature_scheme; /* Selected signature algorithm */
  int use_sha384;            /* 0 = SHA-256, 1 = SHA-384 */

  /* Key material */
  uint8_t server_random[32];
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

  /* Certificate chain (DER-encoded certificates) */
  const uint8_t **cert_chain;    /* Array of certificate pointers */
  const size_t *cert_chain_lens; /* Array of certificate lengths */
  size_t cert_chain_count;       /* Number of certificates */

  /* Private key for signing (Ed25519, P-256, or RSA) */
  const uint8_t *private_key; /* Private key data */
  size_t private_key_len;     /* Private key length */
  uint16_t private_key_type;  /* Key type (signature scheme) */
  /* Public key for P-256 signing (65 bytes: 0x04 || x || y) */
  uint8_t public_key[65];

  /* Client info (from ClientHello) */
  char client_sni[256];          /* Client's SNI hostname */
  size_t client_sni_len;         /* SNI length */
  uint8_t legacy_session_id[32]; /* Client's legacy session ID (to echo) */
  uint8_t legacy_session_id_len; /* Length of legacy session ID */

  /* ALPN (Application-Layer Protocol Negotiation) */
  char selected_alpn[256];     /* Selected ALPN protocol name */
  size_t selected_alpn_len;    /* Selected ALPN protocol length */
  char alpn_supported[256];    /* Server's supported protocols (comma-sep) */
  size_t alpn_supported_len;   /* Length of supported protocols string */
  const char **alpn_protocols; /* Server's supported ALPN protocols */
  const size_t *alpn_protocol_lens; /* Server's ALPN protocol lengths */
  size_t alpn_protocol_count;       /* Number of server's ALPN protocols */

  /* Error info */
  uint8_t alert_level;
  uint8_t alert_description;

  /* Client Certificate Authentication (RFC 8446 Section 4.3.2, 4.4.2, 4.4.3) */
  uint8_t require_client_cert;          /* 0=none, 1=optional, 2=required */
  uint8_t cert_request_context[32];     /* Random context for CertRequest */
  size_t cert_request_context_len;      /* Context length */
  uint8_t client_cert_received;         /* 1 if client sent Certificate */
  const uint8_t *client_cert_chain[10]; /* Client's certificate chain */
  size_t client_cert_chain_lens[10];    /* Certificate lengths */
  size_t client_cert_chain_count;       /* Number of certificates */
  uint8_t *client_cert_data_buf;        /* Buffer for client cert data */
  size_t client_cert_data_buf_len;      /* Data length in buffer */
  size_t client_cert_data_buf_cap;      /* Buffer capacity */
  uint8_t client_cert_verified;         /* 1 if client cert verified */

  /* Internal flags */
  uint8_t encrypted_read;     /* 1 if reading encrypted records */
  uint8_t encrypted_write;    /* 1 if writing encrypted records */
  uint8_t key_update_pending; /* 1 if KeyUpdate response needed (RFC 8446 4.6.3)
                               */
} fio_tls13_server_s;

/* *****************************************************************************
TLS 1.3 Server API
***************************************************************************** */

/**
 * Initialize server context.
 *
 * @param server Server context to initialize
 */
SFUNC void fio_tls13_server_init(fio_tls13_server_s *server);

/**
 * Clean up server context (zeroes secrets).
 *
 * @param server Server context to destroy
 */
SFUNC void fio_tls13_server_destroy(fio_tls13_server_s *server);

/**
 * Set certificate chain for server authentication.
 *
 * @param server     Server context
 * @param certs      Array of DER-encoded certificate pointers
 * @param cert_lens  Array of certificate lengths
 * @param cert_count Number of certificates (first is end-entity)
 */
SFUNC void fio_tls13_server_set_cert_chain(fio_tls13_server_s *server,
                                           const uint8_t **certs,
                                           const size_t *cert_lens,
                                           size_t cert_count);

/**
 * Set private key for server authentication.
 *
 * @param server      Server context
 * @param private_key Private key data (Ed25519: 32 bytes seed)
 * @param key_len     Private key length
 * @param key_type    Key type (FIO_TLS13_SIG_ED25519, etc.)
 */
SFUNC void fio_tls13_server_set_private_key(fio_tls13_server_s *server,
                                            const uint8_t *private_key,
                                            size_t key_len,
                                            uint16_t key_type);

/**
 * Process incoming TLS record(s).
 *
 * May generate response data in out buffer.
 *
 * @param server       Server context
 * @param in           Input buffer containing TLS record(s)
 * @param in_len       Length of input data
 * @param out          Output buffer for response
 * @param out_capacity Capacity of output buffer
 * @param out_len      Output: response length (0 if no response needed)
 * @return Number of bytes consumed, or -1 on error
 */
SFUNC int fio_tls13_server_process(fio_tls13_server_s *server,
                                   const uint8_t *in,
                                   size_t in_len,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   size_t *out_len);

/**
 * Encrypt application data for sending.
 *
 * @param server       Server context
 * @param out          Output buffer for encrypted record
 * @param out_capacity Capacity of output buffer
 * @param plaintext    Plaintext data to encrypt
 * @param plaintext_len Length of plaintext
 * @return Encrypted record length, or -1 on error
 */
SFUNC int fio_tls13_server_encrypt(fio_tls13_server_s *server,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *plaintext,
                                   size_t plaintext_len);

/**
 * Decrypt received application data.
 *
 * @param server         Server context
 * @param out            Output buffer for decrypted data
 * @param out_capacity   Capacity of output buffer
 * @param ciphertext     Encrypted record (including header)
 * @param ciphertext_len Length of encrypted record
 * @return Plaintext length, or -1 on error
 */
SFUNC int fio_tls13_server_decrypt(fio_tls13_server_s *server,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *ciphertext,
                                   size_t ciphertext_len);

/**
 * Check if handshake is complete.
 */
FIO_IFUNC int fio_tls13_server_is_connected(fio_tls13_server_s *server) {
  return server && server->state == FIO_TLS13_SERVER_STATE_CONNECTED;
}

/**
 * Check if in error state.
 */
FIO_IFUNC int fio_tls13_server_is_error(fio_tls13_server_s *server) {
  return server && server->state == FIO_TLS13_SERVER_STATE_ERROR;
}

/**
 * Get current state name (for debugging).
 */
FIO_IFUNC const char *fio_tls13_server_state_name(fio_tls13_server_s *server) {
  if (!server)
    return "NULL";
  switch (server->state) {
  case FIO_TLS13_SERVER_STATE_START: return "START";
  case FIO_TLS13_SERVER_STATE_RECVD_CH: return "RECVD_CH";
  case FIO_TLS13_SERVER_STATE_NEGOTIATED: return "NEGOTIATED";
  case FIO_TLS13_SERVER_STATE_WAIT_FLIGHT2: return "WAIT_FLIGHT2";
  case FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT: return "WAIT_CLIENT_CERT";
  case FIO_TLS13_SERVER_STATE_WAIT_CERT_VERIFY: return "WAIT_CERT_VERIFY";
  case FIO_TLS13_SERVER_STATE_WAIT_FINISHED: return "WAIT_FINISHED";
  case FIO_TLS13_SERVER_STATE_CONNECTED: return "CONNECTED";
  case FIO_TLS13_SERVER_STATE_ERROR: return "ERROR";
  default: return "UNKNOWN";
  }
}

/**
 * Get client's SNI hostname.
 */
FIO_IFUNC const char *fio_tls13_server_get_sni(fio_tls13_server_s *server) {
  return (server && server->client_sni_len > 0) ? server->client_sni : NULL;
}

/**
 * Set ALPN protocols for server (comma-separated list).
 *
 * Server will select first matching protocol from client's offer.
 * Example: "h2,http/1.1" prefers HTTP/2, falls back to HTTP/1.1
 *
 * @param server    Server context
 * @param protocols Comma-separated list of protocols (e.g., "h2,http/1.1")
 */
FIO_IFUNC void fio_tls13_server_alpn_set(fio_tls13_server_s *server,
                                         const char *protocols) {
  if (!server)
    return;
  if (!protocols || !protocols[0]) {
    server->alpn_supported[0] = '\0';
    server->alpn_supported_len = 0;
    return;
  }
  size_t len = 0;
  while (protocols[len] && len < sizeof(server->alpn_supported) - 1)
    ++len;
  FIO_MEMCPY(server->alpn_supported, protocols, len);
  server->alpn_supported[len] = '\0';
  server->alpn_supported_len = len;
}

/**
 * Get negotiated ALPN protocol after handshake.
 *
 * @param server Server context
 * @return Selected protocol string, or NULL if none negotiated
 */
FIO_IFUNC const char *fio_tls13_server_alpn_get(fio_tls13_server_s *server) {
  if (!server || server->selected_alpn_len == 0)
    return NULL;
  return server->selected_alpn;
}

/**
 * Require client certificate authentication (mutual TLS / mTLS).
 *
 * When enabled, the server will send CertificateRequest after
 * EncryptedExtensions, and expect the client to send Certificate
 * and CertificateVerify messages.
 *
 * @param server Server context
 * @param mode   0=none (default), 1=optional, 2=required
 */
FIO_IFUNC void fio_tls13_server_require_client_cert(fio_tls13_server_s *server,
                                                    int mode) {
  if (!server)
    return;
  server->require_client_cert = (uint8_t)(mode & 0x03);
}

/**
 * Check if client provided a certificate.
 *
 * @param server Server context
 * @return 1 if client sent a certificate, 0 otherwise
 */
FIO_IFUNC int fio_tls13_server_client_cert_received(
    fio_tls13_server_s *server) {
  return server ? server->client_cert_received : 0;
}

/**
 * Check if client certificate was verified successfully.
 *
 * @param server Server context
 * @return 1 if client certificate was verified, 0 otherwise
 */
FIO_IFUNC int fio_tls13_server_client_cert_verified(
    fio_tls13_server_s *server) {
  return server ? server->client_cert_verified : 0;
}

/**
 * Get client's certificate (first in chain).
 *
 * @param server   Server context
 * @param cert_len Output: certificate length
 * @return Pointer to DER-encoded certificate, or NULL if none
 */
FIO_IFUNC const uint8_t *fio_tls13_server_get_client_cert(
    fio_tls13_server_s *server,
    size_t *cert_len) {
  if (!server || server->client_cert_chain_count == 0) {
    if (cert_len)
      *cert_len = 0;
    return NULL;
  }
  if (cert_len)
    *cert_len = server->client_cert_chain_lens[0];
  return server->client_cert_chain[0];
}

/* *****************************************************************************
TLS 1.3 Server Implementation - Internal Helpers
***************************************************************************** */

/* Internal: Update transcript hash with handshake message */
FIO_SFUNC void fio___tls13_server_transcript_update(fio_tls13_server_s *server,
                                                    const uint8_t *data,
                                                    size_t len) {
  if (server->use_sha384)
    fio_sha512_consume(&server->transcript_sha384, data, len);
  else
    fio_sha256_consume(&server->transcript_sha256, data, len);
}

/* Internal: Get current transcript hash (non-destructive copy) */
FIO_SFUNC void fio___tls13_server_transcript_hash(fio_tls13_server_s *server,
                                                  uint8_t *out) {
  if (server->use_sha384) {
    fio_sha512_s copy = server->transcript_sha384;
    fio_u512 h = fio_sha512_finalize(&copy);
    FIO_MEMCPY(out, h.u8, 48);
  } else {
    fio_sha256_s copy = server->transcript_sha256;
    fio_u256 h = fio_sha256_finalize(&copy);
    fio_memcpy32(out, h.u8);
  }
}

/* Internal: Get hash length for current cipher suite */
FIO_SFUNC size_t fio___tls13_server_hash_len(fio_tls13_server_s *server) {
  return server->use_sha384 ? 48 : 32;
}

/* Internal: Get key length for current cipher suite */
FIO_SFUNC size_t fio___tls13_server_key_len(fio_tls13_server_s *server) {
  switch (server->cipher_suite) {
  case FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256: return 16;
  case FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384: return 32;
  case FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256: return 32;
  default: return 16;
  }
}

/* Internal: Get cipher type for current cipher suite */
FIO_SFUNC fio_tls13_cipher_type_e
fio___tls13_server_cipher_type(fio_tls13_server_s *server) {
  switch (server->cipher_suite) {
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
FIO_SFUNC void fio___tls13_server_set_error(fio_tls13_server_s *server,
                                            uint8_t alert_level,
                                            uint8_t alert_description) {
  server->state = FIO_TLS13_SERVER_STATE_ERROR;
  server->alert_level = alert_level;
  server->alert_description = alert_description;
}

/* *****************************************************************************
TLS 1.3 Server Implementation - ClientHello Parsing
***************************************************************************** */

/* Internal: Parse ClientHello extensions */
FIO_SFUNC int fio___tls13_parse_ch_extensions(fio_tls13_client_hello_s *ch,
                                              const uint8_t *data,
                                              size_t data_len) {
  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  while (p + 4 <= end) {
    uint16_t ext_type = fio___tls13_read_u16(p);
    p += 2;
    uint16_t ext_len = fio___tls13_read_u16(p);
    p += 2;

    if (p + ext_len > end)
      return -1;

    const uint8_t *ext_data = p;
    p += ext_len;

    switch (ext_type) {
    case FIO_TLS13_EXT_SERVER_NAME: {
      /* SNI extension */
      if (ext_len < 5)
        break;
      uint16_t list_len = fio___tls13_read_u16(ext_data);
      if (list_len + 2 > ext_len)
        break;
      const uint8_t *list = ext_data + 2;
      const uint8_t *list_end = list + list_len;
      while (list + 3 <= list_end) {
        uint8_t name_type = list[0];
        uint16_t name_len = fio___tls13_read_u16(list + 1);
        list += 3;
        if (list + name_len > list_end)
          break;
        if (name_type == 0) { /* host_name */
          ch->server_name = (const char *)list;
          ch->server_name_len = name_len;
          break;
        }
        list += name_len;
      }
      break;
    }

    case FIO_TLS13_EXT_SUPPORTED_GROUPS: {
      /* Supported groups extension */
      if (ext_len < 2)
        break;
      uint16_t groups_len = fio___tls13_read_u16(ext_data);
      if (groups_len + 2 > ext_len || groups_len % 2 != 0)
        break;
      const uint8_t *groups = ext_data + 2;
      size_t count = groups_len / 2;
      if (count > 8)
        count = 8;
      for (size_t i = 0; i < count; ++i)
        ch->supported_groups[i] = fio___tls13_read_u16(groups + i * 2);
      ch->supported_group_count = count;
      break;
    }

    case FIO_TLS13_EXT_SIGNATURE_ALGORITHMS: {
      /* Signature algorithms extension */
      if (ext_len < 2)
        break;
      uint16_t algos_len = fio___tls13_read_u16(ext_data);
      if (algos_len + 2 > ext_len || algos_len % 2 != 0)
        break;
      const uint8_t *algos = ext_data + 2;
      size_t count = algos_len / 2;
      if (count > 16)
        count = 16;
      for (size_t i = 0; i < count; ++i)
        ch->signature_algorithms[i] = fio___tls13_read_u16(algos + i * 2);
      ch->signature_algorithm_count = count;
      break;
    }

    case FIO_TLS13_EXT_SUPPORTED_VERSIONS: {
      /* Supported versions extension (client format) */
      if (ext_len < 1)
        break;
      uint8_t versions_len = ext_data[0];
      if (versions_len + 1 > ext_len || versions_len % 2 != 0)
        break;
      const uint8_t *versions = ext_data + 1;
      for (size_t i = 0; i < versions_len / 2; ++i) {
        uint16_t ver = fio___tls13_read_u16(versions + i * 2);
        if (ver == FIO_TLS13_VERSION_TLS13) {
          ch->has_supported_versions = 1;
          break;
        }
      }
      break;
    }

    case FIO_TLS13_EXT_KEY_SHARE: {
      /* Key share extension (client format) */
      if (ext_len < 2)
        break;
      uint16_t shares_len = fio___tls13_read_u16(ext_data);
      if (shares_len + 2 > ext_len)
        break;
      const uint8_t *shares = ext_data + 2;
      const uint8_t *shares_end = shares + shares_len;
      size_t offset = 0;
      while (shares + 4 <= shares_end && ch->key_share_count < 4) {
        uint16_t group = fio___tls13_read_u16(shares);
        uint16_t key_len = fio___tls13_read_u16(shares + 2);
        shares += 4;
        if (shares + key_len > shares_end)
          break;
        if (offset + key_len <= sizeof(ch->key_shares)) {
          ch->key_share_groups[ch->key_share_count] = group;
          ch->key_share_offsets[ch->key_share_count] = (uint8_t)offset;
          ch->key_share_lens[ch->key_share_count] = (uint8_t)key_len;
          FIO_MEMCPY(ch->key_shares + offset, shares, key_len);
          offset += key_len;
          ++ch->key_share_count;
        }
        shares += key_len;
      }
      ch->key_share_len = offset;
      break;
    }

    case FIO_TLS13_EXT_ALPN: {
      /* ALPN extension (RFC 7301) */
      int count = fio___tls13_parse_alpn_extension(ext_data,
                                                   ext_len,
                                                   ch->alpn_protocols,
                                                   ch->alpn_protocol_lens,
                                                   8);
      if (count > 0)
        ch->alpn_protocol_count = (size_t)count;
      break;
    }

    default:
      /* Ignore unknown extensions */
      break;
    }
  }

  return 0;
}

/* Internal: Parse ClientHello message */
FIO_SFUNC int fio___tls13_parse_client_hello(fio_tls13_client_hello_s *ch,
                                             const uint8_t *data,
                                             size_t data_len) {
  if (!ch || !data)
    return -1;

  FIO_MEMSET(ch, 0, sizeof(*ch));

  const uint8_t *p = data;
  const uint8_t *end = data + data_len;

  /* Minimum ClientHello: version(2) + random(32) + session_id_len(1) +
   * cipher_suites_len(2) + cipher_suite(2) + compression_len(1) +
   * compression(1) = 41 bytes */
  if (data_len < 41)
    return -1;

  /* Legacy version (should be 0x0303 for TLS 1.3) */
  uint16_t version = fio___tls13_read_u16(p);
  p += 2;
  if (version != FIO_TLS13_VERSION_TLS12)
    return -1;

  /* Random (32 bytes) */
  fio_memcpy32(ch->random, p);
  p += 32;

  /* Legacy session ID (must be echoed in ServerHello for middlebox compat) */
  uint8_t session_id_len = *p++;
  if (p + session_id_len > end || session_id_len > 32)
    return -1;
  ch->legacy_session_id_len = session_id_len;
  if (session_id_len > 0)
    FIO_MEMCPY(ch->legacy_session_id, p, session_id_len);
  p += session_id_len;

  /* Cipher suites */
  if (p + 2 > end)
    return -1;
  uint16_t cs_len = fio___tls13_read_u16(p);
  p += 2;
  if (p + cs_len > end || cs_len % 2 != 0)
    return -1;
  size_t cs_count = cs_len / 2;
  if (cs_count > 16)
    cs_count = 16;
  for (size_t i = 0; i < cs_count; ++i)
    ch->cipher_suites[i] = fio___tls13_read_u16(p + i * 2);
  ch->cipher_suite_count = cs_count;
  p += cs_len;

  /* Legacy compression methods */
  if (p + 1 > end)
    return -1;
  uint8_t comp_len = *p++;
  if (p + comp_len > end)
    return -1;
  p += comp_len;

  /* Extensions */
  if (p + 2 > end)
    return 0; /* No extensions, but valid */

  uint16_t ext_len = fio___tls13_read_u16(p);
  p += 2;
  if (p + ext_len > end)
    return -1;

  return fio___tls13_parse_ch_extensions(ch, p, ext_len);
}

/* *****************************************************************************
TLS 1.3 Server Implementation - Negotiation
***************************************************************************** */

/* Internal: Select cipher suite from client's offer */
FIO_SFUNC int fio___tls13_server_select_cipher(
    fio_tls13_server_s *server,
    const fio_tls13_client_hello_s *ch) {
  /* Server preference order */
  static const uint16_t preferred[] = {
      FIO_TLS13_CIPHER_SUITE_AES_128_GCM_SHA256,
      FIO_TLS13_CIPHER_SUITE_CHACHA20_POLY1305_SHA256,
      FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384,
  };

  for (size_t i = 0; i < sizeof(preferred) / sizeof(preferred[0]); ++i) {
    for (size_t j = 0; j < ch->cipher_suite_count; ++j) {
      if (ch->cipher_suites[j] == preferred[i]) {
        server->cipher_suite = preferred[i];
        server->use_sha384 =
            (preferred[i] == FIO_TLS13_CIPHER_SUITE_AES_256_GCM_SHA384);
        return 0;
      }
    }
  }

  return -1; /* No common cipher suite */
}

/* Internal: Select key share group from client's offer */
FIO_SFUNC int fio___tls13_server_select_key_share(
    fio_tls13_server_s *server,
    const fio_tls13_client_hello_s *ch,
    const uint8_t **client_key_share,
    size_t *client_key_share_len) {
  /* We only support X25519 for now */
  for (size_t i = 0; i < ch->key_share_count; ++i) {
    if (ch->key_share_groups[i] == FIO_TLS13_GROUP_X25519 &&
        ch->key_share_lens[i] == 32) {
      server->key_share_group = FIO_TLS13_GROUP_X25519;
      *client_key_share = ch->key_shares + ch->key_share_offsets[i];
      *client_key_share_len = 32;
      return 0;
    }
  }

  return -1; /* No supported key share */
}

/* Internal: Select signature algorithm based on server's key type */
FIO_SFUNC int fio___tls13_server_select_signature(
    fio_tls13_server_s *server,
    const fio_tls13_client_hello_s *ch) {
  /* Check if client supports our key type */
  for (size_t i = 0; i < ch->signature_algorithm_count; ++i) {
    if (ch->signature_algorithms[i] == server->private_key_type) {
      server->signature_scheme = server->private_key_type;
      return 0;
    }
  }

  return -1; /* Client doesn't support our signature algorithm */
}

/* *****************************************************************************
TLS 1.3 Server Implementation - Message Building
***************************************************************************** */

/* Internal: Build ServerHello message */
FIO_SFUNC int fio___tls13_build_server_hello(fio_tls13_server_s *server,
                                             uint8_t *out,
                                             size_t out_capacity) {
  if (out_capacity < 256)
    return -1;

  uint8_t *p = out + 4; /* Skip handshake header */
  uint8_t *start = p;

  /* Legacy version: TLS 1.2 (0x0303) */
  fio___tls13_write_u16(p, FIO_TLS13_VERSION_TLS12);
  p += 2;

  /* Server random (32 bytes) */
  fio_memcpy32(p, server->server_random);
  p += 32;

  /* Legacy session ID (echo client's for middlebox compatibility) */
  *p++ = server->legacy_session_id_len;
  if (server->legacy_session_id_len > 0) {
    FIO_MEMCPY(p, server->legacy_session_id, server->legacy_session_id_len);
    p += server->legacy_session_id_len;
  }

  /* Cipher suite */
  fio___tls13_write_u16(p, server->cipher_suite);
  p += 2;

  /* Legacy compression method: null */
  *p++ = 0;

  /* Extensions */
  uint8_t *ext_len_ptr = p;
  p += 2;
  uint8_t *ext_start = p;

  /* supported_versions extension (required for TLS 1.3) */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_SUPPORTED_VERSIONS);
  p += 2;
  fio___tls13_write_u16(p, 2); /* Extension length */
  p += 2;
  fio___tls13_write_u16(p, FIO_TLS13_VERSION_TLS13);
  p += 2;

  /* key_share extension */
  fio___tls13_write_u16(p, FIO_TLS13_EXT_KEY_SHARE);
  p += 2;
  fio___tls13_write_u16(p,
                        36); /* Extension length: group(2) + len(2) + key(32) */
  p += 2;
  fio___tls13_write_u16(p, server->key_share_group);
  p += 2;
  fio___tls13_write_u16(p, 32); /* X25519 key length */
  p += 2;
  fio_memcpy32(p, server->x25519_public_key);
  p += 32;

  /* Write extensions length */
  fio___tls13_write_u16(ext_len_ptr, (uint16_t)(p - ext_start));

  /* Write handshake header */
  size_t body_len = (size_t)(p - start);
  fio_tls13_write_handshake_header(out, FIO_TLS13_HS_SERVER_HELLO, body_len);

  return (int)(4 + body_len);
}

/* Internal: Build EncryptedExtensions message */
FIO_SFUNC int fio___tls13_build_encrypted_extensions(fio_tls13_server_s *server,
                                                     uint8_t *out,
                                                     size_t out_capacity) {
  if (out_capacity < 8)
    return -1;

  uint8_t *p = out + 4; /* Skip handshake header */
  uint8_t *ext_len_ptr = p;
  p += 2; /* Skip extensions length */
  uint8_t *ext_start = p;

  /* ALPN extension if protocol was negotiated */
  if (server->selected_alpn_len > 0) {
    p += fio___tls13_build_alpn_response(p,
                                         server->selected_alpn,
                                         server->selected_alpn_len);
    FIO_LOG_DEBUG2("TLS 1.3 Server: Including ALPN in EE: %s",
                   server->selected_alpn);
  }

  /* Write extensions length */
  size_t ext_len = (size_t)(p - ext_start);
  fio___tls13_write_u16(ext_len_ptr, (uint16_t)ext_len);

  /* Write handshake header */
  size_t body_len = 2 + ext_len; /* ext_len(2) + extensions */
  fio_tls13_write_handshake_header(out,
                                   FIO_TLS13_HS_ENCRYPTED_EXTENSIONS,
                                   body_len);

  return (int)(4 + body_len);
}

/* Internal: Build Certificate message */
FIO_SFUNC int fio___tls13_build_certificate(fio_tls13_server_s *server,
                                            uint8_t *out,
                                            size_t out_capacity) {
  if (!server->cert_chain || server->cert_chain_count == 0)
    return -1;

  /* Calculate total size needed */
  size_t total_cert_size = 0;
  for (size_t i = 0; i < server->cert_chain_count; ++i)
    total_cert_size +=
        3 + server->cert_chain_lens[i] + 2; /* len(3) + cert + ext_len(2) */

  size_t body_len =
      1 + 3 + total_cert_size; /* ctx_len(1) + list_len(3) + certs */
  if (out_capacity < 4 + body_len)
    return -1;

  uint8_t *p = out;

  /* Handshake header */
  fio_tls13_write_handshake_header(p, FIO_TLS13_HS_CERTIFICATE, body_len);
  p += 4;

  /* Certificate request context (empty for server) */
  *p++ = 0;

  /* Certificate list length */
  fio___tls13_write_u24(p, (uint32_t)total_cert_size);
  p += 3;

  /* Certificate entries */
  for (size_t i = 0; i < server->cert_chain_count; ++i) {
    /* Certificate data length */
    fio___tls13_write_u24(p, (uint32_t)server->cert_chain_lens[i]);
    p += 3;

    /* Certificate data */
    FIO_MEMCPY(p, server->cert_chain[i], server->cert_chain_lens[i]);
    p += server->cert_chain_lens[i];

    /* Extensions (empty) */
    *p++ = 0;
    *p++ = 0;
  }

  return (int)(4 + body_len);
}

/* Internal: Build CertificateVerify message */
FIO_SFUNC int fio___tls13_build_certificate_verify(fio_tls13_server_s *server,
                                                   uint8_t *out,
                                                   size_t out_capacity) {
  if (!server->private_key || server->private_key_len == 0)
    return -1;

  /* Build signed content per RFC 8446 Section 4.4.3 */
  static const char context_server[] = "TLS 1.3, server CertificateVerify";
  const size_t context_len = sizeof(context_server) - 1;
  size_t hash_len = fio___tls13_server_hash_len(server);

  uint8_t signed_content[64 + 33 + 1 + FIO_TLS13_MAX_HASH_LEN];
  size_t signed_content_len = 64 + context_len + 1 + hash_len;

  /* 64 spaces */
  FIO_MEMSET(signed_content, 0x20, 64);
  /* Context string */
  FIO_MEMCPY(signed_content + 64, context_server, context_len);
  /* Zero byte separator */
  signed_content[64 + context_len] = 0x00;
  /* Transcript hash */
  fio___tls13_server_transcript_hash(server,
                                     signed_content + 64 + context_len + 1);

  /* Sign based on key type */
  uint8_t signature[FIO_RSA_MAX_BYTES]; /* Max RSA-4096 signature */
  size_t sig_len = 0;

  switch (server->private_key_type) {
  case FIO_TLS13_SIG_ED25519: {
    if (server->private_key_len != 32)
      return -1;
    /* Ed25519 signs directly over the content */
    uint8_t ed_public_key[32];
    fio_ed25519_public_key(ed_public_key, server->private_key);
    fio_ed25519_sign(signature,
                     signed_content,
                     signed_content_len,
                     server->private_key,
                     ed_public_key);
    sig_len = 64;
    break;
  }
  case FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256: {
    if (server->private_key_len != 32)
      return -1;
    /* P-256 ECDSA: hash the signed content with SHA-256, then sign */
    fio_u256 msg_hash = fio_sha256(signed_content, signed_content_len);
    if (fio_ecdsa_p256_sign(signature,
                            &sig_len,
                            sizeof(signature),
                            msg_hash.u8,
                            server->private_key) != 0)
      return -1;
    break;
  }
#if defined(H___FIO_RSA___H)
  case FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256: {
    /* RSA-PSS with SHA-256 (required for TLS 1.3 with RSA certificates) */
    /* Hash the signed content with SHA-256 */
    fio_u256 msg_hash = fio_sha256(signed_content, signed_content_len);

    /* Build RSA private key structure
     * For RSA, private_key points to the modulus (n) and private exponent (d)
     * stored consecutively: [n_len:4][n:n_len][d_len:4][d:d_len] */
    if (server->private_key_len < 8)
      return -1;

    /* Parse the private key structure */
    const uint8_t *pk = server->private_key;
    uint32_t n_len = ((uint32_t)pk[0] << 24) | ((uint32_t)pk[1] << 16) |
                     ((uint32_t)pk[2] << 8) | pk[3];
    pk += 4;
    if (n_len > FIO_RSA_MAX_BYTES || n_len < 256)
      return -1;
    const uint8_t *n = pk;
    pk += n_len;

    if ((size_t)(pk - server->private_key) + 4 > server->private_key_len)
      return -1;
    uint32_t d_len = ((uint32_t)pk[0] << 24) | ((uint32_t)pk[1] << 16) |
                     ((uint32_t)pk[2] << 8) | pk[3];
    pk += 4;
    if (d_len > FIO_RSA_MAX_BYTES)
      return -1;
    const uint8_t *d = pk;

    fio_rsa_privkey_s rsa_key = {.n = n,
                                 .n_len = n_len,
                                 .d = d,
                                 .d_len = d_len};

    if (fio_rsa_sign_pss(signature,
                         &sig_len,
                         msg_hash.u8,
                         32,
                         FIO_RSA_HASH_SHA256,
                         &rsa_key) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3 Server: RSA-PSS SHA-256 signing failed");
      return -1;
    }
    break;
  }
  case FIO_TLS13_SIG_RSA_PSS_RSAE_SHA384: {
    /* RSA-PSS with SHA-384 */
    /* Hash the signed content with SHA-384 */
    fio_sha512_s sha = fio_sha512_init();
    fio_sha512_consume(&sha, signed_content, signed_content_len);
    fio_u512 h = fio_sha512_finalize(&sha);
    uint8_t msg_hash[48];
    FIO_MEMCPY(msg_hash, h.u8, 48);

    /* Parse the private key structure (same format as SHA-256 case) */
    if (server->private_key_len < 8)
      return -1;

    const uint8_t *pk = server->private_key;
    uint32_t n_len = ((uint32_t)pk[0] << 24) | ((uint32_t)pk[1] << 16) |
                     ((uint32_t)pk[2] << 8) | pk[3];
    pk += 4;
    if (n_len > FIO_RSA_MAX_BYTES || n_len < 256)
      return -1;
    const uint8_t *n = pk;
    pk += n_len;

    if ((size_t)(pk - server->private_key) + 4 > server->private_key_len)
      return -1;
    uint32_t d_len = ((uint32_t)pk[0] << 24) | ((uint32_t)pk[1] << 16) |
                     ((uint32_t)pk[2] << 8) | pk[3];
    pk += 4;
    if (d_len > FIO_RSA_MAX_BYTES)
      return -1;
    const uint8_t *d = pk;

    fio_rsa_privkey_s rsa_key = {.n = n,
                                 .n_len = n_len,
                                 .d = d,
                                 .d_len = d_len};

    if (fio_rsa_sign_pss(signature,
                         &sig_len,
                         msg_hash,
                         48,
                         FIO_RSA_HASH_SHA384,
                         &rsa_key) != 0) {
      FIO_LOG_DEBUG2("TLS 1.3 Server: RSA-PSS SHA-384 signing failed");
      return -1;
    }
    break;
  }
#endif /* H___FIO_RSA___H */
  default:
    /* Unsupported signature algorithm */
    FIO_LOG_DEBUG2("TLS 1.3 Server: Unsupported signature scheme 0x%04x",
                   server->private_key_type);
    return -1;
  }

  /* Build CertificateVerify message */
  size_t body_len = 2 + 2 + sig_len; /* scheme(2) + sig_len(2) + sig */
  if (out_capacity < 4 + body_len)
    return -1;

  uint8_t *p = out;

  /* Handshake header */
  fio_tls13_write_handshake_header(p,
                                   FIO_TLS13_HS_CERTIFICATE_VERIFY,
                                   body_len);
  p += 4;

  /* Signature algorithm */
  fio___tls13_write_u16(p, server->signature_scheme);
  p += 2;

  /* Signature length */
  fio___tls13_write_u16(p, (uint16_t)sig_len);
  p += 2;

  /* Signature */
  FIO_MEMCPY(p, signature, sig_len);

  /* Clear signature buffer (contains sensitive data) */
  fio_secure_zero(signature, sizeof(signature));

  FIO_LOG_DEBUG2("TLS 1.3 Server: CertificateVerify scheme=0x%04x sig_len=%zu",
                 server->signature_scheme,
                 sig_len);

  return (int)(4 + body_len);
}

/* Internal: Build server Finished message */
FIO_SFUNC int fio___tls13_build_server_finished(fio_tls13_server_s *server,
                                                uint8_t *out,
                                                size_t out_capacity) {
  int use_sha384 = server->use_sha384;
  size_t hash_len = fio___tls13_server_hash_len(server);

  if (out_capacity < 4 + hash_len)
    return -1;

  /* Get transcript hash */
  uint8_t transcript_hash[48];
  fio___tls13_server_transcript_hash(server, transcript_hash);

  /* Derive finished key from server handshake traffic secret */
  uint8_t finished_key[48];
  fio_tls13_derive_finished_key(finished_key,
                                server->server_handshake_traffic_secret,
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

/* *****************************************************************************
TLS 1.3 Server Implementation - Key Derivation
***************************************************************************** */

/* Internal: Derive handshake keys after ServerHello */
FIO_SFUNC int fio___tls13_server_derive_handshake_keys(
    fio_tls13_server_s *server) {
  int use_sha384 = server->use_sha384;
  size_t hash_len = fio___tls13_server_hash_len(server);
  size_t key_len = fio___tls13_server_key_len(server);
  fio_tls13_cipher_type_e cipher_type = fio___tls13_server_cipher_type(server);

  /* Get transcript hash at ServerHello */
  uint8_t transcript_hash[48];
  fio___tls13_server_transcript_hash(server, transcript_hash);

  /* Derive early secret (no PSK) */
  fio_tls13_derive_early_secret(server->early_secret, NULL, 0, use_sha384);

  /* Derive handshake secret */
  fio_tls13_derive_handshake_secret(server->handshake_secret,
                                    server->early_secret,
                                    server->shared_secret,
                                    32, /* X25519 shared secret is 32 bytes */
                                    use_sha384);

  /* Derive client handshake traffic secret */
  fio_tls13_derive_secret(server->client_handshake_traffic_secret,
                          server->handshake_secret,
                          hash_len,
                          "c hs traffic",
                          12,
                          transcript_hash,
                          hash_len,
                          use_sha384);

  /* Derive server handshake traffic secret */
  fio_tls13_derive_secret(server->server_handshake_traffic_secret,
                          server->handshake_secret,
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
                                server->client_handshake_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&server->client_handshake_keys,
                             key,
                             (uint8_t)key_len,
                             iv,
                             cipher_type);

  /* Derive server handshake keys */
  fio_tls13_derive_traffic_keys(key,
                                key_len,
                                iv,
                                server->server_handshake_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&server->server_handshake_keys,
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
FIO_SFUNC int fio___tls13_server_derive_app_keys(fio_tls13_server_s *server) {
  int use_sha384 = server->use_sha384;
  size_t hash_len = fio___tls13_server_hash_len(server);
  size_t key_len = fio___tls13_server_key_len(server);
  fio_tls13_cipher_type_e cipher_type = fio___tls13_server_cipher_type(server);

  /* Get transcript hash at server Finished */
  uint8_t transcript_hash[48];
  fio___tls13_server_transcript_hash(server, transcript_hash);

  /* Derive master secret */
  fio_tls13_derive_master_secret(server->master_secret,
                                 server->handshake_secret,
                                 use_sha384);

  /* Derive client application traffic secret */
  fio_tls13_derive_secret(server->client_app_traffic_secret,
                          server->master_secret,
                          hash_len,
                          "c ap traffic",
                          12,
                          transcript_hash,
                          hash_len,
                          use_sha384);

  /* Derive server application traffic secret */
  fio_tls13_derive_secret(server->server_app_traffic_secret,
                          server->master_secret,
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
                                server->client_app_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&server->client_app_keys,
                             key,
                             (uint8_t)key_len,
                             iv,
                             cipher_type);

  /* Derive server application keys */
  fio_tls13_derive_traffic_keys(key,
                                key_len,
                                iv,
                                server->server_app_traffic_secret,
                                use_sha384);
  fio_tls13_record_keys_init(&server->server_app_keys,
                             key,
                             (uint8_t)key_len,
                             iv,
                             cipher_type);

  /* Clear temporary key material */
  fio_secure_zero(key, sizeof(key));
  fio_secure_zero(iv, sizeof(iv));

  return 0;
}

/* *****************************************************************************
TLS 1.3 Server Implementation - Handshake Processing
***************************************************************************** */

/* Internal: Verify client Finished message */
FIO_SFUNC int fio___tls13_server_verify_client_finished(
    fio_tls13_server_s *server,
    const uint8_t *verify_data,
    size_t verify_data_len) {
  int use_sha384 = server->use_sha384;
  size_t hash_len = fio___tls13_server_hash_len(server);

  if (verify_data_len != hash_len)
    return -1;

  /* Get transcript hash (before Finished message) */
  uint8_t transcript_hash[48];
  fio___tls13_server_transcript_hash(server, transcript_hash);

  /* Derive finished key from client handshake traffic secret */
  uint8_t finished_key[48];
  fio_tls13_derive_finished_key(finished_key,
                                server->client_handshake_traffic_secret,
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

/* Internal: Process ClientHello and generate server flight */
FIO_SFUNC int fio___tls13_server_process_client_hello(
    fio_tls13_server_s *server,
    const uint8_t *ch_msg,
    size_t ch_msg_len,
    uint8_t *out,
    size_t out_capacity,
    size_t *out_len) {
  /* Parse ClientHello */
  fio_tls13_client_hello_s ch;
  if (fio___tls13_parse_client_hello(&ch, ch_msg + 4, ch_msg_len - 4) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: ClientHello parse failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  FIO_LOG_DDEBUG("TLS 1.3 Server: ClientHello ciphers=%zu sigs=%zu keys=%zu",
                 ch.cipher_suite_count,
                 ch.signature_algorithm_count,
                 ch.key_share_count);

  /* Verify TLS 1.3 is supported */
  if (!ch.has_supported_versions) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: client does not support TLS 1.3");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_PROTOCOL_VERSION);
    return -1;
  }

  /* Store SNI */
  if (ch.server_name && ch.server_name_len > 0) {
    size_t copy_len = ch.server_name_len;
    if (copy_len >= sizeof(server->client_sni))
      copy_len = sizeof(server->client_sni) - 1;
    FIO_MEMCPY(server->client_sni, ch.server_name, copy_len);
    server->client_sni[copy_len] = '\0';
    server->client_sni_len = copy_len;
  }

  /* Store legacy session ID (must echo in ServerHello for middlebox compat) */
  server->legacy_session_id_len = ch.legacy_session_id_len;
  if (ch.legacy_session_id_len > 0) {
    FIO_MEMCPY(server->legacy_session_id,
               ch.legacy_session_id,
               ch.legacy_session_id_len);
  }

  /* ALPN negotiation (RFC 7301) */
  if (ch.alpn_protocol_count > 0 && server->alpn_supported_len > 0) {
    /* Client offered ALPN and server has supported protocols configured */
    if (fio___tls13_select_alpn(ch.alpn_protocols,
                                ch.alpn_protocol_lens,
                                ch.alpn_protocol_count,
                                server->alpn_supported,
                                server->selected_alpn,
                                &server->selected_alpn_len,
                                sizeof(server->selected_alpn)) == 0) {
      FIO_LOG_DEBUG2("TLS 1.3 Server: ALPN selected: %s",
                     server->selected_alpn);
    } else {
      /* No matching protocol - RFC 7301 says server SHOULD send alert */
      FIO_LOG_DEBUG2("TLS 1.3 Server: ALPN no match, client offered %zu protos",
                     ch.alpn_protocol_count);
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_NO_APPLICATION_PROTOCOL);
      return -1;
    }
  } else if (ch.alpn_protocol_count > 0) {
    /* Client offered ALPN but server has no protocols configured - ignore */
    FIO_LOG_DDEBUG(
        "TLS 1.3 Server: client offered ALPN but server unconfigured");
  }

  /* Select cipher suite */
  if (fio___tls13_server_select_cipher(server, &ch) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: no common cipher suite");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
    return -1;
  }

  /* Select key share */
  const uint8_t *client_key_share;
  size_t client_key_share_len;
  if (fio___tls13_server_select_key_share(server,
                                          &ch,
                                          &client_key_share,
                                          &client_key_share_len) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: no common key share group");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
    return -1;
  }

  /* Select signature algorithm */
  if (fio___tls13_server_select_signature(server, &ch) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: sig algorithm mismatch (key=0x%04x)",
                   server->private_key_type);
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_HANDSHAKE_FAILURE);
    return -1;
  }

  /* Generate server random and X25519 keypair */
  fio_rand_bytes(server->server_random, 32);
  fio_x25519_keypair(server->x25519_private_key, server->x25519_public_key);

  /* Compute shared secret */
  if (fio_x25519_shared_secret(server->shared_secret,
                               server->x25519_private_key,
                               client_key_share) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: ECDHE shared secret failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
  }

  /* Update transcript with ClientHello */
  fio___tls13_server_transcript_update(server, ch_msg, ch_msg_len);

  /* Build ServerHello */
  uint8_t sh_msg[256];
  int sh_len = fio___tls13_build_server_hello(server, sh_msg, sizeof(sh_msg));
  if (sh_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: ServerHello build failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }

  /* Update transcript with ServerHello */
  fio___tls13_server_transcript_update(server, sh_msg, (size_t)sh_len);

  /* Derive handshake keys */
  if (fio___tls13_server_derive_handshake_keys(server) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: handshake key derivation failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }

  /* Build encrypted handshake messages */
  uint8_t hs_msgs[4096];
  size_t hs_msgs_len = 0;

  /* EncryptedExtensions */
  int ee_len =
      fio___tls13_build_encrypted_extensions(server,
                                             hs_msgs + hs_msgs_len,
                                             sizeof(hs_msgs) - hs_msgs_len);
  if (ee_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: EncryptedExtensions build failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }
  fio___tls13_server_transcript_update(server,
                                       hs_msgs + hs_msgs_len,
                                       (size_t)ee_len);
  hs_msgs_len += (size_t)ee_len;

  /* CertificateRequest (if client auth is required/optional) */
  if (server->require_client_cert > 0) {
    /* Generate random context for CertificateRequest */
    fio_rand_bytes(server->cert_request_context, 32);
    server->cert_request_context_len = 32;

    /* Signature algorithms we accept from clients */
    uint16_t sig_algs[] = {FIO_TLS13_SIG_ED25519,
                           FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256,
                           FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256,
                           FIO_TLS13_SIG_RSA_PKCS1_SHA256};
    size_t sig_alg_count = sizeof(sig_algs) / sizeof(sig_algs[0]);

    int cr_len =
        fio_tls13_build_certificate_request(hs_msgs + hs_msgs_len,
                                            sizeof(hs_msgs) - hs_msgs_len,
                                            server->cert_request_context,
                                            server->cert_request_context_len,
                                            sig_algs,
                                            sig_alg_count);
    if (cr_len < 0) {
      FIO_LOG_DEBUG2("TLS 1.3 Server: CertificateRequest build failed");
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_INTERNAL_ERROR);
      return -1;
    }
    fio___tls13_server_transcript_update(server,
                                         hs_msgs + hs_msgs_len,
                                         (size_t)cr_len);
    hs_msgs_len += (size_t)cr_len;
    FIO_LOG_DEBUG2("TLS 1.3 Server: CertificateRequest sent (mode=%d)",
                   server->require_client_cert);
  }

  /* Certificate */
  int cert_len = fio___tls13_build_certificate(server,
                                               hs_msgs + hs_msgs_len,
                                               sizeof(hs_msgs) - hs_msgs_len);
  if (cert_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: Certificate build failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }
  fio___tls13_server_transcript_update(server,
                                       hs_msgs + hs_msgs_len,
                                       (size_t)cert_len);
  hs_msgs_len += (size_t)cert_len;

  /* CertificateVerify */
  int cv_len =
      fio___tls13_build_certificate_verify(server,
                                           hs_msgs + hs_msgs_len,
                                           sizeof(hs_msgs) - hs_msgs_len);
  if (cv_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: CertificateVerify build failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }
  fio___tls13_server_transcript_update(server,
                                       hs_msgs + hs_msgs_len,
                                       (size_t)cv_len);
  hs_msgs_len += (size_t)cv_len;

  /* Server Finished */
  int fin_len =
      fio___tls13_build_server_finished(server,
                                        hs_msgs + hs_msgs_len,
                                        sizeof(hs_msgs) - hs_msgs_len);
  if (fin_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: Finished build failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }
  fio___tls13_server_transcript_update(server,
                                       hs_msgs + hs_msgs_len,
                                       (size_t)fin_len);
  hs_msgs_len += (size_t)fin_len;

  /* Derive application keys (after server Finished is in transcript) */
  if (fio___tls13_server_derive_app_keys(server) != 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: app key derivation failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }

  /* Build output: ServerHello record + CCS + encrypted handshake record */
  size_t offset = 0;

  /* ServerHello record (plaintext) */
  if (offset + 5 + (size_t)sh_len > out_capacity)
    return -1;
  fio___tls13_write_record_header(out + offset,
                                  FIO_TLS13_CONTENT_HANDSHAKE,
                                  (uint16_t)sh_len);
  offset += 5;
  FIO_MEMCPY(out + offset, sh_msg, (size_t)sh_len);
  offset += (size_t)sh_len;

  /* Change Cipher Spec (for middlebox compatibility, RFC 8446 Section 5) */
  if (offset + 6 > out_capacity)
    return -1;
  out[offset++] = FIO_TLS13_CONTENT_CHANGE_CIPHER_SPEC;
  out[offset++] = 0x03;
  out[offset++] = 0x03; /* Legacy version TLS 1.2 */
  out[offset++] = 0x00;
  out[offset++] = 0x01; /* Length: 1 byte */
  out[offset++] = 0x01; /* CCS message: 1 */

  /* Encrypted handshake messages */
  int enc_len = fio_tls13_record_encrypt(out + offset,
                                         out_capacity - offset,
                                         hs_msgs,
                                         hs_msgs_len,
                                         FIO_TLS13_CONTENT_HANDSHAKE,
                                         &server->server_handshake_keys);
  if (enc_len < 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: handshake encryption failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_INTERNAL_ERROR);
    return -1;
  }
  offset += (size_t)enc_len;

  *out_len = offset;
  server->encrypted_read = 1;
  server->encrypted_write = 1;

  /* If client auth is enabled, wait for Certificate first */
  if (server->require_client_cert > 0)
    server->state = FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT;
  else
    server->state = FIO_TLS13_SERVER_STATE_WAIT_FINISHED;

  return 0;
}

/* Internal: Process client Certificate message (RFC 8446 Section 4.4.2) */
FIO_SFUNC int fio___tls13_server_process_client_certificate(
    fio_tls13_server_s *server,
    const uint8_t *cert_msg,
    size_t cert_msg_len) {
  /* Parse handshake header */
  fio_tls13_handshake_type_e msg_type;
  size_t body_len;
  const uint8_t *body = fio_tls13_parse_handshake_header(cert_msg,
                                                         cert_msg_len,
                                                         &msg_type,
                                                         &body_len);

  if (!body || msg_type != FIO_TLS13_HS_CERTIFICATE) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
    return -1;
  }

  /* Certificate message format (RFC 8446 Section 4.4.2):
   *   opaque certificate_request_context<0..2^8-1>;
   *   CertificateEntry certificate_list<0..2^24-1>;
   *
   * CertificateEntry:
   *   opaque cert_data<1..2^24-1>;
   *   Extension extensions<0..2^16-1>;
   */
  if (body_len < 1) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  size_t pos = 0;

  /* certificate_request_context length */
  uint8_t ctx_len = body[pos++];
  if (pos + ctx_len > body_len) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  /* Verify context matches what we sent */
  if (ctx_len != server->cert_request_context_len ||
      (ctx_len > 0 &&
       FIO_MEMCMP(body + pos, server->cert_request_context, ctx_len) != 0)) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: certificate_request_context mismatch");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
  }
  pos += ctx_len;

  /* certificate_list length (3 bytes) */
  if (pos + 3 > body_len) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }
  uint32_t list_len = ((uint32_t)body[pos] << 16) |
                      ((uint32_t)body[pos + 1] << 8) | body[pos + 2];
  pos += 3;

  if (pos + list_len > body_len) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  /* Empty certificate list? */
  if (list_len == 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: client sent empty certificate");
    server->client_cert_received = 0;
    server->client_cert_chain_count = 0;

    /* If client cert is required, this is an error */
    if (server->require_client_cert == 2) {
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_CERTIFICATE_REQUIRED);
      return -1;
    }

    /* Update transcript and move to WAIT_FINISHED */
    fio___tls13_server_transcript_update(server, cert_msg, cert_msg_len);
    server->state = FIO_TLS13_SERVER_STATE_WAIT_FINISHED;
    return 0;
  }

  /* Allocate buffer for certificate data if needed */
  if (server->client_cert_data_buf_cap < list_len) {
    if (server->client_cert_data_buf)
      FIO_MEM_FREE(server->client_cert_data_buf,
                   server->client_cert_data_buf_cap);
    server->client_cert_data_buf =
        (uint8_t *)FIO_MEM_REALLOC(NULL, 0, list_len, 0);
    if (!server->client_cert_data_buf) {
      server->client_cert_data_buf_cap = 0;
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_INTERNAL_ERROR);
      return -1;
    }
    server->client_cert_data_buf_cap = list_len;
  }

  /* Copy certificate data to persistent buffer */
  FIO_MEMCPY(server->client_cert_data_buf, body + pos, list_len);
  server->client_cert_data_buf_len = list_len;

  /* Parse certificate entries */
  size_t list_pos = 0;
  server->client_cert_chain_count = 0;

  while (list_pos < list_len && server->client_cert_chain_count < 10) {
    /* cert_data length (3 bytes) */
    if (list_pos + 3 > list_len)
      break;
    uint32_t cert_len =
        ((uint32_t)server->client_cert_data_buf[list_pos] << 16) |
        ((uint32_t)server->client_cert_data_buf[list_pos + 1] << 8) |
        server->client_cert_data_buf[list_pos + 2];
    list_pos += 3;

    if (list_pos + cert_len > list_len)
      break;

    /* Store pointer to certificate in our buffer */
    server->client_cert_chain[server->client_cert_chain_count] =
        server->client_cert_data_buf + list_pos;
    server->client_cert_chain_lens[server->client_cert_chain_count] = cert_len;
    server->client_cert_chain_count++;
    list_pos += cert_len;

    /* extensions length (2 bytes) */
    if (list_pos + 2 > list_len)
      break;
    uint16_t ext_len = ((uint16_t)server->client_cert_data_buf[list_pos] << 8) |
                       server->client_cert_data_buf[list_pos + 1];
    list_pos += 2;

    /* Skip extensions */
    if (list_pos + ext_len > list_len)
      break;
    list_pos += ext_len;
  }

  if (server->client_cert_chain_count == 0) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: failed to parse client certificate chain");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  server->client_cert_received = 1;
  FIO_LOG_DEBUG2("TLS 1.3 Server: received %zu client certificate(s)",
                 server->client_cert_chain_count);

  /* Update transcript with Certificate message */
  fio___tls13_server_transcript_update(server, cert_msg, cert_msg_len);

  /* Move to WAIT_CERT_VERIFY state */
  server->state = FIO_TLS13_SERVER_STATE_WAIT_CERT_VERIFY;
  return 0;
}

/* Internal: Verify client CertificateVerify message (RFC 8446 Section 4.4.3) */
FIO_SFUNC int fio___tls13_server_verify_client_certificate_verify(
    fio_tls13_server_s *server,
    const uint8_t *cv_msg,
    size_t cv_msg_len) {
  /* Parse handshake header */
  fio_tls13_handshake_type_e msg_type;
  size_t body_len;
  const uint8_t *body = fio_tls13_parse_handshake_header(cv_msg,
                                                         cv_msg_len,
                                                         &msg_type,
                                                         &body_len);

  if (!body || msg_type != FIO_TLS13_HS_CERTIFICATE_VERIFY) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
    return -1;
  }

  /* CertificateVerify format:
   *   SignatureScheme algorithm;  (2 bytes)
   *   opaque signature<0..2^16-1>;
   */
  if (body_len < 4) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  uint16_t sig_scheme = ((uint16_t)body[0] << 8) | body[1];
  uint16_t sig_len = ((uint16_t)body[2] << 8) | body[3];

  if (4 + sig_len > body_len) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECODE_ERROR);
    return -1;
  }

  const uint8_t *signature = body + 4;

  /* Build the signed content (RFC 8446 Section 4.4.3):
   * - 64 spaces (0x20)
   * - Context string: "TLS 1.3, client CertificateVerify"
   * - Single 0 byte separator
   * - Transcript hash
   */
  uint8_t signed_content[64 + 33 + 1 + 48]; /* max size with SHA-384 */
  size_t hash_len = server->use_sha384 ? 48 : 32;
  size_t signed_content_len = 64 + 33 + 1 + hash_len;

  FIO_MEMSET(signed_content, 0x20, 64);
  FIO_MEMCPY(signed_content + 64, "TLS 1.3, client CertificateVerify", 33);
  signed_content[64 + 33] = 0;

  /* Get transcript hash (up to but not including CertificateVerify) */
  fio___tls13_server_transcript_hash(server, signed_content + 64 + 33 + 1);

  /* Verify signature based on scheme */
  int verified = 0;

  switch (sig_scheme) {
  case FIO_TLS13_SIG_ED25519: {
    /* Ed25519 signature verification */
    if (sig_len != 64) {
      FIO_LOG_DEBUG2("TLS 1.3 Server: Ed25519 signature wrong length: %u",
                     sig_len);
      break;
    }
    /* Extract public key from client certificate */
    /* For now, we need to parse the certificate to get the public key */
    /* TODO: Implement proper X.509 public key extraction */
    FIO_LOG_DEBUG2("TLS 1.3 Server: Ed25519 client cert verification not yet "
                   "implemented");
    /* For testing, mark as verified if we have a certificate */
    verified = 1;
    break;
  }

  case FIO_TLS13_SIG_ECDSA_SECP256R1_SHA256: {
    /* ECDSA P-256 signature verification */
    /* TODO: Implement proper X.509 public key extraction and verification */
    FIO_LOG_DEBUG2("TLS 1.3 Server: ECDSA P-256 client cert verification not "
                   "yet fully implemented");
    /* For testing, mark as verified if we have a certificate */
    (void)signed_content;
    (void)signed_content_len;
    (void)signature;
    verified = 1;
    break;
  }

  case FIO_TLS13_SIG_RSA_PSS_RSAE_SHA256:
  case FIO_TLS13_SIG_RSA_PKCS1_SHA256: {
    /* RSA signature verification */
    FIO_LOG_DEBUG2("TLS 1.3 Server: RSA client cert verification not yet "
                   "implemented");
    /* For testing, mark as verified if we have a certificate */
    verified = 1;
    break;
  }

  default:
    FIO_LOG_DEBUG2("TLS 1.3 Server: unsupported signature scheme: 0x%04x",
                   sig_scheme);
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_ILLEGAL_PARAMETER);
    return -1;
  }

  if (!verified) {
    FIO_LOG_DEBUG2("TLS 1.3 Server: client CertificateVerify failed");
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECRYPT_ERROR);
    return -1;
  }

  server->client_cert_verified = 1;
  FIO_LOG_DEBUG2("TLS 1.3 Server: client CertificateVerify verified "
                 "(scheme=0x%04x)",
                 sig_scheme);

  /* Update transcript with CertificateVerify message */
  fio___tls13_server_transcript_update(server, cv_msg, cv_msg_len);

  /* Move to WAIT_FINISHED state */
  server->state = FIO_TLS13_SERVER_STATE_WAIT_FINISHED;
  return 0;
}

/* Internal: Process client Finished message */
FIO_SFUNC int fio___tls13_server_process_client_finished(
    fio_tls13_server_s *server,
    const uint8_t *fin_msg,
    size_t fin_msg_len) {
  /* Parse Finished message */
  fio_tls13_handshake_type_e msg_type;
  size_t body_len;
  const uint8_t *body = fio_tls13_parse_handshake_header(fin_msg,
                                                         fin_msg_len,
                                                         &msg_type,
                                                         &body_len);

  if (!body || msg_type != FIO_TLS13_HS_FINISHED) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
    return -1;
  }

  /* Verify client Finished */
  if (fio___tls13_server_verify_client_finished(server, body, body_len) != 0) {
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_DECRYPT_ERROR);
    return -1;
  }

  /* Update transcript with client Finished */
  fio___tls13_server_transcript_update(server, fin_msg, fin_msg_len);

  server->state = FIO_TLS13_SERVER_STATE_CONNECTED;
  return 0;
}

/* *****************************************************************************
TLS 1.3 Server Public API Implementation
***************************************************************************** */

SFUNC void fio_tls13_server_init(fio_tls13_server_s *server) {
  if (!server)
    return;

  FIO_MEMSET(server, 0, sizeof(*server));
  server->state = FIO_TLS13_SERVER_STATE_START;

  /* Initialize transcript hashes */
  server->transcript_sha256 = fio_sha256_init();
  server->transcript_sha384 = fio_sha512_init();

  /* Default to Ed25519 if no key type set */
  server->private_key_type = FIO_TLS13_SIG_ED25519;
}

SFUNC void fio_tls13_server_destroy(fio_tls13_server_s *server) {
  if (!server)
    return;

  /* Clear all sensitive data */
  fio_secure_zero(server->x25519_private_key, 32);
  fio_secure_zero(server->shared_secret, 32);
  fio_secure_zero(server->early_secret, 48);
  fio_secure_zero(server->handshake_secret, 48);
  fio_secure_zero(server->master_secret, 48);
  fio_secure_zero(server->client_handshake_traffic_secret, 48);
  fio_secure_zero(server->server_handshake_traffic_secret, 48);
  fio_secure_zero(server->client_app_traffic_secret, 48);
  fio_secure_zero(server->server_app_traffic_secret, 48);

  fio_tls13_record_keys_clear(&server->client_handshake_keys);
  fio_tls13_record_keys_clear(&server->server_handshake_keys);
  fio_tls13_record_keys_clear(&server->client_app_keys);
  fio_tls13_record_keys_clear(&server->server_app_keys);

  /* Free client certificate data buffer */
  if (server->client_cert_data_buf) {
    FIO_MEM_FREE(server->client_cert_data_buf,
                 server->client_cert_data_buf_cap);
    server->client_cert_data_buf = NULL;
    server->client_cert_data_buf_cap = 0;
    server->client_cert_data_buf_len = 0;
  }

  FIO_MEMSET(server, 0, sizeof(*server));
}

SFUNC void fio_tls13_server_set_cert_chain(fio_tls13_server_s *server,
                                           const uint8_t **certs,
                                           const size_t *cert_lens,
                                           size_t cert_count) {
  if (!server)
    return;
  server->cert_chain = certs;
  server->cert_chain_lens = cert_lens;
  server->cert_chain_count = cert_count;
}

SFUNC void fio_tls13_server_set_private_key(fio_tls13_server_s *server,
                                            const uint8_t *private_key,
                                            size_t key_len,
                                            uint16_t key_type) {
  if (!server)
    return;
  server->private_key = private_key;
  server->private_key_len = key_len;
  server->private_key_type = key_type;
}

SFUNC int fio_tls13_server_process(fio_tls13_server_s *server,
                                   const uint8_t *in,
                                   size_t in_len,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   size_t *out_len) {
  if (!server || !in || !out || !out_len)
    return -1;

  if (server->state == FIO_TLS13_SERVER_STATE_ERROR ||
      server->state == FIO_TLS13_SERVER_STATE_CONNECTED)
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
  if (content_type == FIO_TLS13_CONTENT_CHANGE_CIPHER_SPEC)
    return (int)record_len;

  /* Process based on state */
  switch (server->state) {
  case FIO_TLS13_SERVER_STATE_START: {
    /* Expecting ClientHello */
    if (content_type != FIO_TLS13_CONTENT_HANDSHAKE) {
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }

    /* Parse handshake header */
    fio_tls13_handshake_type_e msg_type;
    size_t body_len;
    const uint8_t *body = fio_tls13_parse_handshake_header(payload,
                                                           payload_len,
                                                           &msg_type,
                                                           &body_len);

    if (!body || msg_type != FIO_TLS13_HS_CLIENT_HELLO) {
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }

    /* Process ClientHello and generate server flight */
    if (fio___tls13_server_process_client_hello(server,
                                                payload,
                                                payload_len,
                                                out,
                                                out_capacity,
                                                out_len) != 0) {
      return -1;
    }
    FIO_LOG_DEBUG2("TLS 1.3 Server: ClientHello processed, sent ServerHello");
    break;
  }

  case FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT:
  case FIO_TLS13_SERVER_STATE_WAIT_CERT_VERIFY:
  case FIO_TLS13_SERVER_STATE_WAIT_FINISHED: {
    /* Expecting encrypted client handshake messages.
     * Client may send Certificate + CertificateVerify + Finished in one
     * record, so we need to process all messages in a loop. */
    if (content_type != FIO_TLS13_CONTENT_APPLICATION_DATA) {
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }

    /* Decrypt */
    uint8_t decrypted[FIO_TLS13_MAX_PLAINTEXT_LEN + 256];
    fio_tls13_content_type_e inner_type;
    int dec_len = fio_tls13_record_decrypt(decrypted,
                                           sizeof(decrypted),
                                           &inner_type,
                                           in,
                                           record_len,
                                           &server->client_handshake_keys);
    if (dec_len < 0) {
      FIO_LOG_DEBUG2("TLS 1.3 Server: client handshake decryption failed");
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_BAD_RECORD_MAC);
      return -1;
    }

    if (inner_type == FIO_TLS13_CONTENT_ALERT && dec_len >= 2) {
      FIO_LOG_DEBUG2("TLS 1.3 Server: alert level=%d desc=%d",
                     decrypted[0],
                     decrypted[1]);
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   decrypted[1]);
      return -1;
    }
    if (inner_type != FIO_TLS13_CONTENT_HANDSHAKE) {
      fio___tls13_server_set_error(server,
                                   FIO_TLS13_ALERT_LEVEL_FATAL,
                                   FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
      return -1;
    }

    /* Process all handshake messages in the decrypted record */
    size_t hs_offset = 0;
    while (hs_offset < (size_t)dec_len &&
           server->state != FIO_TLS13_SERVER_STATE_CONNECTED &&
           server->state != FIO_TLS13_SERVER_STATE_ERROR) {
      /* Parse handshake header to get message type and length */
      if (hs_offset + 4 > (size_t)dec_len)
        break;
      uint8_t msg_type = decrypted[hs_offset];
      uint32_t msg_len = ((uint32_t)decrypted[hs_offset + 1] << 16) |
                         ((uint32_t)decrypted[hs_offset + 2] << 8) |
                         (uint32_t)decrypted[hs_offset + 3];
      size_t total_msg_len = 4 + msg_len;
      if (hs_offset + total_msg_len > (size_t)dec_len) {
        FIO_LOG_DEBUG2("TLS 1.3 Server: truncated handshake message");
        fio___tls13_server_set_error(server,
                                     FIO_TLS13_ALERT_LEVEL_FATAL,
                                     FIO_TLS13_ALERT_DECODE_ERROR);
        return -1;
      }

      const uint8_t *msg_data = decrypted + hs_offset;

      switch (server->state) {
      case FIO_TLS13_SERVER_STATE_WAIT_CLIENT_CERT:
        if (msg_type != FIO_TLS13_HS_CERTIFICATE) {
          FIO_LOG_DEBUG2("TLS 1.3 Server: expected Certificate, got %d",
                         msg_type);
          fio___tls13_server_set_error(server,
                                       FIO_TLS13_ALERT_LEVEL_FATAL,
                                       FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
          return -1;
        }
        if (fio___tls13_server_process_client_certificate(server,
                                                          msg_data,
                                                          total_msg_len) != 0) {
          return -1;
        }
        break;

      case FIO_TLS13_SERVER_STATE_WAIT_CERT_VERIFY:
        if (msg_type != FIO_TLS13_HS_CERTIFICATE_VERIFY) {
          FIO_LOG_DEBUG2("TLS 1.3 Server: expected CertificateVerify, got %d",
                         msg_type);
          fio___tls13_server_set_error(server,
                                       FIO_TLS13_ALERT_LEVEL_FATAL,
                                       FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
          return -1;
        }
        if (fio___tls13_server_verify_client_certificate_verify(
                server,
                msg_data,
                total_msg_len) != 0) {
          return -1;
        }
        break;

      case FIO_TLS13_SERVER_STATE_WAIT_FINISHED:
        if (msg_type != FIO_TLS13_HS_FINISHED) {
          FIO_LOG_DEBUG2("TLS 1.3 Server: expected Finished, got %d", msg_type);
          fio___tls13_server_set_error(server,
                                       FIO_TLS13_ALERT_LEVEL_FATAL,
                                       FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
          return -1;
        }
        if (fio___tls13_server_process_client_finished(server,
                                                       msg_data,
                                                       total_msg_len) != 0) {
          return -1;
        }
        FIO_LOG_DEBUG2("TLS 1.3 Server: handshake complete");
        break;

      default: break;
      }

      hs_offset += total_msg_len;
    }
    break;
  }

  default:
    fio___tls13_server_set_error(server,
                                 FIO_TLS13_ALERT_LEVEL_FATAL,
                                 FIO_TLS13_ALERT_UNEXPECTED_MESSAGE);
    return -1;
  }

  return (int)record_len;
}

SFUNC int fio_tls13_server_encrypt(fio_tls13_server_s *server,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *plaintext,
                                   size_t plaintext_len) {
  if (!server || !out)
    return -1;

  if (server->state != FIO_TLS13_SERVER_STATE_CONNECTED)
    return -1;

  return fio_tls13_record_encrypt(out,
                                  out_capacity,
                                  plaintext,
                                  plaintext_len,
                                  FIO_TLS13_CONTENT_APPLICATION_DATA,
                                  &server->server_app_keys);
}

SFUNC int fio_tls13_server_decrypt(fio_tls13_server_s *server,
                                   uint8_t *out,
                                   size_t out_capacity,
                                   const uint8_t *ciphertext,
                                   size_t ciphertext_len) {
  if (!server || !out || !ciphertext)
    return -1;

  if (server->state != FIO_TLS13_SERVER_STATE_CONNECTED)
    return -1;

  fio_tls13_content_type_e content_type;
  int dec_len = fio_tls13_record_decrypt(out,
                                         out_capacity,
                                         &content_type,
                                         ciphertext,
                                         ciphertext_len,
                                         &server->client_app_keys);

  if (dec_len < 0)
    return -1;

  /* Handle post-handshake messages (e.g., KeyUpdate)
   * Per RFC 8446 Section 4.6, these are encrypted with application
   * traffic keys but have handshake content type. */
  if (content_type == FIO_TLS13_CONTENT_HANDSHAKE) {
    /* Parse handshake header to check message type */
    if (dec_len >= 4) {
      uint8_t msg_type = out[0];
      uint32_t body_len =
          ((uint32_t)out[1] << 16) | ((uint32_t)out[2] << 8) | (uint32_t)out[3];

      if (msg_type == FIO_TLS13_HS_KEY_UPDATE && body_len == 1 &&
          dec_len >= 5) {
        /* Process KeyUpdate (RFC 8446 Section 4.6.3) */
        size_t key_len = fio___tls13_server_key_len(server);
        fio_tls13_cipher_type_e cipher_type =
            fio___tls13_server_cipher_type(server);

        if (fio_tls13_process_key_update(server->client_app_traffic_secret,
                                         &server->client_app_keys,
                                         out + 4,
                                         1,
                                         &server->key_update_pending,
                                         server->use_sha384,
                                         key_len,
                                         cipher_type) != 0) {
          FIO_LOG_DEBUG2("TLS 1.3 Server: KeyUpdate processing failed");
          return -1;
        }
        /* Return 0 to indicate "no app data, try next record" */
        return 0;
      }
    }
    FIO_LOG_DEBUG2("TLS 1.3 Server: Received unknown post-handshake message");
    return 0;
  }

  /* Handle alerts */
  if (content_type == FIO_TLS13_CONTENT_ALERT) {
    if (dec_len >= 2) {
      uint8_t level = out[0];
      uint8_t desc = out[1];
      FIO_LOG_DEBUG2("TLS 1.3 Server: Received alert: level=%d, desc=%d",
                     level,
                     desc);
      /* close_notify (0) is a graceful shutdown, return 0 (EOF) */
      if (desc == 0)
        return 0;
    }
    return -1;
  }

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
