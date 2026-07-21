# ASN.1→DER codec & X.509 refactor plan (pre-release API cleanup)

> SUPERSEDED as a plan — kept as discussion record. Authoritative ledger:
> `./AI-X509.md`; design details: `./ai-task/x509-refactor-design.md`.

Date: 2026-07-21. Status: APPROVED (user decisions recorded below). Nothing released yet, so breaking renames are free.

## Decisions (user, 2026-07-21)

1. **OID comparison** → binary DER content-byte constants + `memcmp`. No decode-to-string on the compare path.
2. **OID registry home** → `x509.h`, public names `FIO_X509_OID_*`. Codec module keeps zero X.509 knowledge.
3. **Renumbering** → `155 der` (stays), `155 rsa` (stays), `156 x509` (was 155), `156 mlkem` (stays, independent sibling), `157 pem` (was 156).
4. **Codec rename** → `fio_asn1_*` → `fio_der_*` (codec is DER-only: rejects indefinite lengths). Includes file/macro/test renames.

## Problems being fixed

- **Dependency-scheme violation**: `155 x509.h` depends on `155 asn1.h` and `155 rsa.h` (same prefix). Works only because `makefile` concat (`cat fio-stl/*.h`) sorts alphabetically (asn1 < rsa < x509). Luck, not structure.
- **Latent wiring bug**: `000 dependencies.h` enables only P256/SHA2/TIME for `FIO_X509`; `#define FIO_X509` alone doesn't compile (unguarded `fio_rsa_*` use at x509.h:1166+, asn1 API throughout, ed25519/p384 verify).
- **Inefficient OID compare**: `fio_asn1_oid_eq` decodes base-128 → decimal string → strcmp on every call. CN OID is 3 bytes on the wire (`55 04 03`).
- **Naming** (naming-things skill, Context Inheritance Rule): `subject_der`, `subject_cn`, `tbs_data`, `san_ext_data` carry context the struct already supplies.
- **Fragile backward-scan** computing `subject_der`/`issuer_der` (x509.h:867-872, 912-921) walks bytes backwards hunting `0x30`; iterator position already knows the TLV start.

## Phase 1 — Rename codec module ASN.1 → DER (stays 155)

- `git mv "fio-stl/155 asn1.h" "fio-stl/155 der.h"`; `155 asn1.md` → `155 der.md`; `git mv tests/asn1.c tests/der.c`
- Symbols: `FIO_ASN1`→`FIO_DER`, `H___FIO_ASN1___H`→`H___FIO_DER___H`, `fio_asn1_*`→`fio_der_*`, `FIO_ASN1_*`→`FIO_DER_*`, `fio___asn1_*`→`fio___der_*` (incl. `fio_asn1_element_s`→`fio_der_element_s`, `fio_asn1_iterator_s`→`fio_der_iterator_s`, `fio_asn1_tag_e/class_e`)
- Update `include.h`, `000 dependencies.h` (incl. FIO_CRYPTO catch-all block), all consumers (`155 x509.h`, `156 pem.h`, `tests/*.c`)
- Do NOT touch `tests-old/`, `fio-stl/docs-old/` (archived)

## Phase 2 — OID refactor (binary constants)

In `155 der.h`:
- `fio_der_oid_eq(const fio_der_element_s *elem, const uint8_t *oid, size_t len)` → `elem->len == len && !FIO_MEMCMP(elem->data, oid, len)`
- `fio_der_encode_oid(buf, oid, len)` → pure TLV wrap (delete dot-string parser)
- Keep `fio_der_parse_oid` (binary→dot-string) for diagnostics only
- DELETE the whole `FIO_OID_*` string registry (lines ~78-116)

In `156 x509.h` (after Phase 3 rename) — public registry, arrays + sizeof:
```c
static const uint8_t FIO_X509_OID_COMMON_NAME[] = {0x55, 0x04, 0x03}; /* 2.5.4.3 */
```
Call sites: `fio_der_oid_eq(&oid, FIO_X509_OID_COMMON_NAME, sizeof(FIO_X509_OID_COMMON_NAME))`.
Full list to port (from der.h:77-116): sig algs (SHA256/384/512_WITH_RSA, RSA_PSS, ECDSA_WITH_SHA256/384/512, ED25519, ED448), key algs (RSA_ENCRYPTION, EC_PUBLIC_KEY), curves (SECP256R1/384R1/521R1, X25519, X448), extensions (SUBJECT_KEY_ID, KEY_USAGE, SUBJECT_ALT_NAME, BASIC_CONSTRAINTS, CRL_DIST_POINTS, CERT_POLICIES, AUTH_KEY_ID, EXT_KEY_USAGE), EKU (SERVER_AUTH, CLIENT_AUTH), DN attrs (COMMON_NAME, COUNTRY, LOCALITY, STATE, ORGANIZATION, ORG_UNIT).
Consumers to update: `156 x509.h` (~35 sites incl. `fio___x509_encode_rdn` calls at 1700-2130), `157 pem.h` (440, 600, 605, 663, 947 + encode side).

## Phase 3 — Renumber x509/pem + rewire dependencies

- `git mv "fio-stl/155 x509.h" "fio-stl/156 x509.h"` (+ `.md`); `git mv "fio-stl/156 pem.h" "fio-stl/157 pem.h"` (+ `.md`)
- `include.h`: update filenames; order becomes der(155), rsa(155), x509(156), pem(157)
- `000 dependencies.h` additions:
  - `#if defined(FIO_PEM)` → `#define FIO_X509`
  - `#if defined(FIO_X509)` → `#define FIO_DER FIO_RSA FIO_ED25519 FIO_P384` (keep existing P256, SHA2, TIME)
- Final graph: der(155) ← x509(156) ← pem(157); rsa(155) ← x509; mlkem(156) independent

## Phase 4 — Struct field renames (Context Inheritance Rule)

`fio_x509_cert_s`:
| old | new |
|---|---|
| `subject_der` / `issuer_der` | `subject` / `issuer` |
| `subject_cn` | `cn` |
| `tbs_data` | `tbs` |
| `san_ext_data` | `san_ext` |
| `der`, `serial`, `signature`, `san_dns`, `san_ip` | keep |

`fio_x509_cert_options_s` (generation): `subject_cn`→`cn`, `subject_cn_len`→`cn_len`, `subject_c`→`country`, `subject_org`→`org`, `subject_ou`→`ou`.

Update sites: `156 x509.h`, `157 pem.h`, `401 io api.h` (doc comment ~317), `402 io types.h` (verify), `405 tls13.h` (287-293, 1853), `405 openssl.h` (1170), `tests/{x509,pem,tls13}.c`, `156 x509.md`, `157 pem.md`, `155 der.md`.

## Phase 5 — Remove backward-scan hack

In `fio___x509_parse_tbs`: capture `const uint8_t *start = tbs_it.pos;` before `fio_der_iterator_next()`, then `cert->issuer = FIO_UBUF_INFO2(start, (elem.data + elem.len) - start)`. Same for subject. Deletes both `while (...[-1] != 0x30)` scans.

## Phase 6 — Verify & record

- `make tests/der tests/x509 tests/pem tests/tls13 tests/openssl` (concat regenerates `fio-stl.h` automatically — generated file, do not hand-edit)
- Also compile-check minimal flags: `#define FIO_X509` alone must now compile (wiring fix)
- Update `AI-DOCS.md` (T010 mentions asn1 docs batch — new filenames), `AI-TODO.md` (remove/close entry), append to `AI-HISTORY.md`
- Doc files renamed in phases 1/3 keep content in sync via api2md (`make` api2md target)

## Risks / notes

- `static const` arrays in header = internal linkage per TU; fine for SFUNC-style STL, no ODR issue (C).
- pem.h uses x509 internals across module boundary — acceptable inside STL (same TU in practice), but registry is public (`FIO_X509_OID_*`) so no triple-underscore violation.
- RSA verify stays unguarded; wiring guarantees FIO_RSA. If a future "parse-only, no crypto" mode is wanted, that's a separate feature.
