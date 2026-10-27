# Change Log

### 0.8.4 - 2026-10-27

**API**: (`p384`, `pem`, `tls13`) P-384 private keys now sign: `fio_ecdsa_p384_sign`, `fio_p384_public_key` and `fio_p384_keypair` use a constant-time Montgomery ladder, like P-256. `FIO_PEM_KEY_ECDSA_P384` loads SEC1 and PKCS#8 P-384 keys (new `fio_pem_private_key_s.ecdsa_p384`). Native TLS 1.3 servers and clients load P-384 certificates and keys (SNI selection and key/certificate matching included) and sign CertificateVerify with `ecdsa_secp384r1_sha384` (0x0503), matching OpenSSL.

**API**: (`tls13`) Client certificates work like server ones: RSA-PSS (SHA-256/384), P-256, P-384 and Ed25519 keys, with the scheme chosen from the server's CertificateRequest, and the whole configured chain is sent (new `fio_tls13_client_set_cert_chain`; `fio_io_tls_cert_add` on a client context now loads the certificate and its intermediates instead of only a leaf). A client whose key matches none of the server's schemes answers with an empty Certificate and no CertificateVerify (RFC 8446 4.4.2.3). New `fio_tls13_flight_size` returns an output capacity sufficient for every handshake flight of an endpoint presenting a given chain.

**Fix**: (`tls13`) Handshake flights are sized from the certificate chain and fragmented into records of at most 16 KiB of plaintext, instead of failing above 4 KiB of handshake messages (8 KiB in the IO layer), so chains such as Let's Encrypt's with a cross-sign work in both directions; longer flights use a temporary allocation that is wiped and released after flushing, and the IO output queue grows only while such a flight is queued. The IO read path compacts a full receive buffer before reading, so a maximum-size record following a short one (ServerHello + CCS) no longer stalls the handshake. RSA private keys loaded from PEM keep e and the CRT parameters (before, only n and d were stored), restoring CRT signing, blinding and the fault check. Peer signature scheme lists keep the first 16 known schemes rather than the first 16 offered, so long lists of unknown schemes can't crowd out supported ones, and the server's EncryptedExtensions checks its output capacity including the ALPN extension.

**Fix**: (`rand`) `FIO_DEFINE_RANDOM128_FN` (and so `fio_rand64` / `fio_rand128`) could repeat values across racing threads in unoptimized builds (MSVC `/Od`): the atomic call counter shared a slot with a working lane, so plain stores republished already-consumed counter values. The per-call value is now lane 1 (as before) plus a sequence number from its own slot of the 64 byte state, which is only written atomically; the mixing math is unchanged, but output streams differ from previous versions.

**Security**: (`md2html`) rendering failures leave a supplied output buffer unchanged, and allocation failures return `NULL` instead of partial HTML. Image-alt rendering uses linear delimiter matching without recursive calls, preventing nested-image stack exhaustion and quadratic bracket rescans.

**Security**: (`mustache`) oversized section names and incomplete bytecode on allocation failure are rejected. Length-delimited filenames and front matter no longer read past their bounds, partial traversal checks recognize both path separators, and callback values and empty loader buffers are released after their last use without leaking or releasing borrowed contexts.

**Security**: (`http`, `http1`) path and cookie parsing check input bounds, ordinary header names and values count toward the configured limit, and client request lines and authority fields are validated. Response serialization and body-storage failures return errors or request closure; file writes honor offsets, and a short declared file range stops the stream. Static-file 304/416 responses release their descriptors.

**Security**: (`websocket`, `sse`) fragmented input retains parser state, invalid UTF-8 and SSE field injection are rejected, and decoded-size limits apply before full output allocation. Buffer and file writes honor offsets and ownership; failed frame allocation preserves fragment state and releases transferred resources.

**Security**: (`pubsub`) malformed IPC lengths and missing payload terminators are rejected before delivery. History cache accounting saturates instead of wrapping, oversized entries are rejected, and asynchronous replay retains its request until completion. Eviction remains per-channel, not globally oldest-first.

**Security**: (`redis`) HELLO completes before queued commands are sent, reply allocation failures reject partial values, and written commands are not replayed after connection loss. Publish sizing and IPC payload bounds are checked, worker reply decoding requires complete input, and engine destruction releases partial parser state. Worker callbacks still depend on IPC delivery; there is no outbound queue budget or reply deadline.

**Security**: (`secret`) The memory mask is now set once per process; before, every `fio_secret_set_at` call rotated it, which corrupted earlier `fio_secret_set_at` values and could change the global `fio_secret()` (the IPC key). A whitespace-only secret now reports `fio_secret_is_random()`, so IPC no longer opens a cluster listener with an unshared random key. `SECRET_LENGTH` is validated against `SECRET` (before, a negative or oversized value read out of bounds at startup); invalid values are ignored with a warning. Decoded and random secret bytes are wiped from the stack, and `fio_secret_at(NULL)` returns zeros.

**Security**: (`argon2`) `fio_argon2_hash` rejects `parallelism` above `2^24-1` (before, huge values wrapped the lane math into a division by zero or a heap overflow), memory matrices exceeding the address space (32-bit heap overflow), inputs over `2^32-1` bytes (silently truncated), `outlen` 1-3 (clamped to 4, overrunning the caller's buffer) and invalid `type` values. Near-`UINT32_MAX` tag lengths no longer wrap H′ arithmetic. Blocks use explicit little-endian order (big-endian hosts now match RFC 9106), secret temporaries are wiped on every path, and `fio_argon2` documents its all-zero error result. Hashing is ~20% faster (in-place permutation, fused XOR, no zero-fill).

**Security**: (`lyra2`) `fio_lyra2_hash` rejects `outlen`, costs, `n_cols` and password/salt lengths above `2^32-1` (before, they were silently truncated, reaching divisions by zero, zero-size allocations and out-of-bounds access) and matrices exceeding the address space (32-bit heap overflow). Password + salt longer than the matrix no longer overflow the heap; they are now hashed. `fio_lyra2` documents its all-zero error result.

**Security**: (`pem`) private-key parsing is strict: exact DER tags, every SEQUENCE consumed exactly, extra/unknown/out-of-order fields, bad versions, negative/zero/non-minimal INTEGERs, partial RSA CRT sets and malformed P-256 parameters or public keys are rejected instead of being imported or silently dropped. PEM bodies are no longer accepted when they contain undecodable bytes or data after padding (only the valid prefix was used before) while padding variants, the URL-safe alphabet and non-zero trailing bits are decoded leniently, `fio_pem_parse` returns the exact consumed length (it stopped inside the END marker before), decoded private-key DER is wiped before release, and `fio_pem_parse_certificate` no longer leaks: the certificate owns its DER, released by the new `fio_pem_certificate_free`.

**Security**: (`mlkem`) decapsulation implicit rejection now returns the full `K̄ = J(z || c)`; before, an invalid ciphertext kept the top 7 bits of every byte of the re-decrypted key `K'`, taking only the low bit from `K̄` (FIPS 203 violation). `fio_mlkem768_encaps*` now perform the FIPS 203 encapsulation-key modulus check and return `-1` for keys with coefficients `>= q`. Matrix rejection sampling no longer reads uninitialized coefficients, noise/decryption temporaries are wiped, and hybrid decapsulation propagates ML-KEM errors.

**Security**: (`p384`) DER signature parsing no longer reads past the input on truncated or malformed signatures, and accepts only strict DER (minimal positive INTEGERs, no trailing data). Public-key coordinates `>= p` are rejected.

**Security**: (`stream`) a file packet that hits EOF or a read error no longer reports unread scratch-buffer bytes as data or spins on a stale `EINTR`; it is truncated to the bytes read and dropped when empty. `fio_stream_pack_data` returns `NULL` (not a freed chain) when a later packet allocation fails, rejects lengths of 1 GiB or more on 64-bit Windows too, and reads use a loop instead of per-packet recursion. `fio_stream_advance` clamps over-advances without wrapping, `fio_stream_pack_fd` rejects negative descriptors (accepting `0`) and ranges `off_t` can't represent, and `FIO_STREAM_COPY_PER_PACKET` must be in `[1, 2^30)`.

**Security**: (`io`, `openssl`, `tls13`) TLS clients with configured trust now check the server's identity. The OpenSSL backend checked only the certificate chain, so a certificate from any trusted CA passed for any host; it now requires the host name (`SSL_set1_host`). `fio_io_connect` to an IP address now requires a matching IP address SAN (native TLS 1.3 skipped the identity check). A reused TLS object no longer verifies against a previous connection's host name. IP literals are never sent as SNI.

**Security**: (`io`, `openssl`, `tls13`) TLS contexts are reference counted, so accepted or connecting TLS connections whose handshake outlives a stopped listener (or a promoted client) no longer read freed certificates, keys or trust stores. `fio_io_attach_fd` takes ownership of its `tls` reference on success and failure; listeners pass a new reference per accepted connection using the transport's `dup_context` callback (a new `fio_io_functions_s` field; rebuild code embedding it). Listeners stopped before their deferred attach, and accept tasks queued behind a close, no longer touch freed or closed state. Overlong listen URLs (including `ADDRESS`) fail instead of looping or binding a default address.

**API**: (`io`, `openssl`, `tls13`) breaking: fixed inconsistent TLS ownership: `fio_io_attach_fd` consumes its built-context reference on success and failure, and `fio_io_listen` / `fio_io_connect` release their `.tls` settings reference on every path. To keep a reference, pass `fio_io_tls_dup(tls)` for settings or the transport's `dup_context(context)` for built contexts; do not free the transferred reference afterward. Listeners pass `dup_context` results through unchanged, including `NULL` (valid for plaintext; the bundled TLS backends close a connection without a context).

**Security**: (`tls13`) A configured certificate or private key file that fails to load (missing file, bad PEM, encrypted key, invalid key) now fails the context build with an error, as the OpenSSL backend does; before, the server silently presented a self-signed certificate instead. Only names added without files are self-signed. A connection started without a context is closed at once.

**Security**: (`io`, `tls13`, `openssl`) TLS is a must when requested: `fio_io_listen` returns `NULL` (calling `on_stop`) and `fio_io_connect` returns `NULL` (running `on_failed` once, on the IO thread) when the TLS context can't be built; before, the listener looked healthy but closed every connection. When `tls` is set the IO functions are a TLS variant, so a `build_context` returning `NULL` means the TLS context couldn't be built. Native TLS 1.3 now checks that the private key matches the leaf certificate (P-256, Ed25519, RSA; like OpenSSL's `X509_check_private_key`) and fails the context build on a mismatch.

**Security**: (`tls13`) The native TLS adapter no longer overflows its receive buffer when a failing record follows valid data in one read batch; only the failing record is retained and the error surfaces on the next read. Malformed complete handshake records (for example an invalid content type) now fail the connection instead of spinning the reactor thread. A connection-state allocation failure closes the connection instead of treating the shared context as connection state.

**Security**: (`ipc`) unauthenticated peers can no longer force large allocations (frames bigger than the read buffer wait until the handshake completes), and allocation or queue failures close the connection instead of aborting or leaking. Cluster connections now prove possession of a per-connection challenge before a declared UUID is accepted, so a replayed declaration can't impersonate a node; the replay filter keeps every ID inside the 30 second window, failing closed on overflow. UDP discovery handles bounded batches with rate-limited logs, failed decryption stays final on retry, and `NULL` data buffers add zeros instead of uninitialized bytes.

**Fix**: (`tls13`) Empty or non-application records (for example a fragmented empty record or `NewSessionTicket`) no longer read as EOF and close the connection. Native TLS clients now offer the registered ALPN names, and both roles call the negotiated protocol's `on_selected` callback when the handshake completes.

**Security**: (`openssl`) A connection setup failure (`SSL_new`, allocation or BIO creation) now closes the connection instead of leaving the shared context where reads and cleanup treated it as connection state. Malformed handshakes, bad records, `close_notify` and hard socket errors now close the connection promptly instead of waiting for the IO timeout.

**Fix**: (`openssl`) TLS records that arrive together with the peer's FIN are delivered before EOF instead of being dropped. Records buffered behind a post-handshake message no longer stall. Clients now call the negotiated ALPN protocol's `on_selected` callback, ALPN names of 255 bytes are accepted, and the deprecated NPN callback is gone.

**Fix**: (`io`) Async queues (`fio_io_async_attach`) are never destroyed while a task may be pending: stopping the reactor or forking workers joins the worker threads and runs pending tasks (before `ON_STOP` callbacks and forking), late tasks run on the next start, and at exit every queue is drained until empty (including tasks pushed by tasks, timer `on_finish` or global env `on_close`) before release. Forked workers reset inherited async locks and threads and still run inherited tasks, workers added before `fio_io_start` no longer leave the master async queues stopped.

**Fix**: (`io`) The wakeup descriptor is closed only after in-flight writers finish, so a late `fio_io_defer` can't write to a closed or reused descriptor, and a restart no longer swallows wakeups through a stale flag. `fio_io_free` no longer reads IO flags off the IO thread and always wakes the reactor after queuing the release.

**Fix**: (`io`) `fio_io_restart` stops old workers when no restart signal is configured (it sent signal 0), sentinel threads retry interrupted `waitpid`, a worker whose sentinel can't start is reaped, and `fio_io_workers(INT_MIN)` no longer overflows.

**Fix**: (`io`) An IO is removed from the poll backend before its descriptor is closed. On epoll, a listener's duplicated descriptor stayed registered after close, so a listener that survived a reactor restart could dispatch its freed IO from the previous cycle (use-after-free and double free; reported by iodine).

**Fix**: (`p256`, `p384`) ECDSA verification with the public key `Q = -G` no longer treats the point at infinity (`G + Q`) as the affine point `(0,0)` during double-scalar multiplication. Valid signatures for that key now verify, and a sum at infinity is rejected.

**Fix**: (`io`) Graceful close (`fio_io_close`) no longer drops output the transport is still holding when the socket is full. This affects encrypted TLS records and the `close_notify` alert. Before, the reactor called `finish` and closed the socket once its own stream was empty. It now waits until the transport's `flush` returns `0`, calls `finish` once, and waits for `flush` to return `0` again before closing. The wait is bounded: a closing IO that reaches its connection timeout is closed at once, and the unsent data is dropped. This happens even when `on_timeout` would keep an open connection alive (for example `fio_io_touch`). The behavior applies to both TLS backends (`tls13` and `openssl`), and no API changed.

**Fix**: (`extern`, `atol`, `poll`, `blake2`, `deflate`, `tls13`, `ipc`, `string`) declarations-only `FIO_EXTERN` units can now call `fio_aton`, `fio_poll_init` / `fio_poll_destroy`, `fio_blake2b` / `fio_blake2s`, `fio_deflate_compress_bound` / `fio_deflate_decompress_bound`, `fio_tls13_build_nonce`, `fio_tls13_record_keys_clear`, `fio_ipc_decrypt` and the string type's `dealloc`. Before, they were always declared `static` but defined only in the implementation unit, so callers failed to link and GCC failed `-Werror` builds with `-Wunused-function`. Header-only builds are unchanged (same `static` / `static inline` linkage); `tests/extern-split` now calls these functions.

**Security**: (`tls13`) the server PEM loader wipes key file contents, rejects a P-256 key whose embedded public key does not match (deriving it when absent; previously a random one was generated), and treats any undecodable certificate block as a load error instead of using a partial chain. CA bundles log and skip undecodable blocks; blocks over 16 KiB now load. The IO write path reserves room for a pending KeyUpdate response in the first record, so it always leads the data. If the response cannot be produced, the write fails with `ECONNRESET` and the connection closes. Data is never sent under stale keys. On close, `close_notify` is queued behind pending encrypted output instead of being written straight to the socket, so it can no longer reach the wire ahead of earlier records (the peer would fail to decrypt them out of order). If it can't be encrypted or queued, it is skipped.

**Security**: (`tls13`) a HelloRetryRequest cookie can no longer overflow the ClientHello2 stack buffer, record decryption enforces the caller's exact output capacity (it could write up to 15 bytes past it), and `fio_tls13_build_client_hello` checks capacity before writing (long SNI or many cipher suites overflowed). Servers requesting client certificates now fail closed without a trust store (any self-signed certificate was accepted as verified); client and server certificates are checked for the clientAuth / serverAuth purpose. RSA PKCS#1 v1.5 CertificateVerify signatures, unoffered server ALPN selections and ServerHellos without `supported_versions` are rejected (RFC 8446 / RFC 7301). Records refuse to wrap the sequence number (nonce reuse), key schedule failures abort the handshake instead of installing stale keys, the standalone encrypt functions send a requested KeyUpdate response before application data, and `fio_tls13_client_is_cert_verified` returns `0` when verification was skipped. ClientHello2 now repeats ClientHello1 (ALPN was dropped) and a cookie-only HelloRetryRequest is accepted. The client rejects ServerHello key shares it never sent (P-256 without an HRR, a hybrid share it didn't offer) and a ServerHello2 that changes the HRR's group or cipher suite. Handshake messages spanning records are reassembled (bounded by `FIO_TLS13_MAX_HANDSHAKE_MSG`); previously they were silently dropped (client) or rejected (server). Data after a message that precedes a key change is rejected. Malformed certificate lists are rejected instead of keeping a valid prefix. Every post-handshake message in a record is processed, with strict KeyUpdate framing. Keys whose length doesn't match the cipher are refused. The server's CertificateRequest lists the schemes it verifies (adds ECDSA P-384 and RSA-PSS SHA-384), and so does the client's `signature_algorithms` (adds ECDSA P-384). A missing trust store is logged once as a warning instead of an error on every connection.

**Security**: (`p256`) DER signature parsing no longer reads past the input on truncated or malformed signatures, and accepts only strict DER (minimal positive INTEGERs, no trailing data). Public-key coordinates `>= p` are rejected, and nonce/key generation fails with `-1` instead of looping on a stuck RNG. New `fio_p256_public_key` derives the public key for an existing secret key.

**Security**: (`der`, `x509`) `fio_der_parse_time` no longer reads past the element on malformed peer certificate times (e.g., a missing `Z`), and rejects trailing data, local times and invalid calendar values (zone offsets, optional minutes/seconds and UTCTime fractions are now decoded). The parser rejects tag numbers above 255 (previously truncated, e.g. tag 256 aliased `[0]`) and stays lenient on unambiguous non-minimal lengths and tags. `fio_der_parse_oid` decodes multi-byte first arcs (`2.999.3`) and rejects overflowing or non-minimal arcs. `fio_der_is_tag` and the typed `fio_der_parse_*` helpers compare the full identifier (class, constructed bit, number), so a primitive `0x10` is no longer a SEQUENCE, a constructed `0x22` no longer an INTEGER, and `[0]` no longer universal tag 0. `fio_der_parse_integer` rejects negative INTEGERs when a value is requested (X.509 enforces minimal INTEGERs itself). `fio_der_is_context_tag` and `fio_der_tag_number` handle high-tag numbers. Time encoders no longer narrow to `time_t` and return `0` when unrepresentable (`UTCTime` covers 1950-2049); X.509 validity uses `GeneralizedTime` after 2049.

**Security**: (`x509`) trust anchors are matched by key, not DN: `fio_x509_is_trusted` and chain validation require a byte-identical anchor or a same-DN anchor whose key verifies the signature (all same-DN anchors are tried). Chain validation now enforces issuer Key Usage (keyCertSign) and pathLenConstraint, leaf Key Usage (digitalSignature), Extended Key Usage (new `fio_x509_verify_chain_purpose` / `fio_x509_purpose_e`), Name Constraints (dNSName, iPAddress, permitted directoryName; other forms fail closed) and rejects unknown critical extensions, with new `FIO_X509_ERR_KEY_USAGE`, `_PATH_LENGTH`, `_NAME_CONSTRAINTS` and `_CRITICAL_EXTENSION` codes. Parsing rejects extra elements or trailing bytes inside certificate structures, padded key/signature BIT STRINGs, inner/outer signature algorithm mismatches, duplicate or malformed recognized extensions and invalid RSA keys (negative or even modulus, even/small/oversized exponent), while tolerating unambiguous non-DER encodings (non-minimal INTEGERs, KU padding bits are masked, empty SAN/EKU lists grant nothing). `cert->der` now covers only the Certificate TLV, so callers can detect trailing bytes. The outer signatureAlgorithm must match the signed copy (by decoded value for supported algorithms, byte for byte otherwise). `fio_x509_fingerprint` stays the standard SHA-256 of the exact certificate bytes; it is not a unique certificate identity (non-minimal headers outside the signed TBS and ECDSA signatures are malleable), so blocklist by issuer + serial or SPKI instead; RSA-PSS parameters are validated and signature algorithms must match the key type. IP-literal hostnames match only SAN iPAddress entries. `fio_tls_parse_certificate_message` rejects length mismatches and lists exceeding `max_entries`. `fio_x509_self_signed_cert` rejects reversed validity, encodes IP SANs as iPAddress and adds clientAuth to the EKU. `fio_x509_cert_s` grew (`name_constraints`, `path_len`, `ext_flags`).

**Security**: (`chacha`, `aes`) inputs that would wrap the 32-bit block counter (reusing keystream) now fail with `-1` instead of being processed, and never abort the process. `fio_chacha20`, `fio_xchacha20`, the ChaCha20-Poly1305 / XChaCha20-Poly1305 / AES-GCM encryption functions and the shared `fio_crypto_enc_fn` type now return `int` (`0` on success); on failure the data (and tag) are zeroed so no plaintext or valid tag is left behind. Decryption rejects over-limit input without touching the data. `fio_x25519_encrypt` and TLS 1.3 record encryption propagate the error. All public ChaCha20 / Poly1305 functions also reject `NULL` `key` / `nonce` / `mac` and `NULL` `data` / `ad` with a non-zero length the same way (previously `fio_poly1305_auth` and the others dereferenced them, or asserted in debug builds); the MAC-only functions `fio_poly1305_auth` and `fio_chacha20_poly1305_auth` now return `int` (`0` on success, `-1` on invalid arguments or, for the AEAD variant, over-limit input) and zero a writable MAC on failure, so callers reject instead of comparing a cleared tag (IPC UDP discovery now does). Zero-length `NULL` buffers remain valid.

**Security**: (`aes`) AES-GCM now rejects AAD of `2^61` bytes or more (the bit length would overflow) and wipes key-schedule, GHASH-key and keystream temporaries on every backend.

**Security**: (`ed25519`) `fio_ed25519_verify` rejects small-order public keys and `R` values, non-canonical keys (`y >= p`, negative zero) and `S >= L`; before, an identity public key verified a fixed signature for any message. The base-point table is now a compile-time constant (no racy lazy initialization), and secret scalars/nonces are wiped after use. `fio_x25519_decrypt` zeroes the plaintext on authentication failure and `fio_x25519_encrypt` zeroes its output when the cipher fails.

**Security**: (`rsa`) verification rejects even moduli and public exponents that are even, below 3 or over 64 bits; before, a certificate key with `e = 1` let anyone forge signatures. A zero-prefixed modulus no longer causes out-of-bounds reads, and PSS now works for moduli whose bit length is `8k + 1`. Signing no longer divides by the secret CRT primes (a timing leak). Unbalanced primes are handled, and every signature is checked against `e`, so faults or inconsistent keys return `-1` instead of emitting a bad signature. Nothing aborts on bad input. Signing is about 3.5x faster and verification about 90x faster.

**Security**: (`random`, `ipc`) RiskyHash 256/512 adds round constants and per-block feed-forward, so a chosen block can no longer erase earlier input. The new `fio_risky256_keyed` / `fio_risky512_keyed` seed all 16 state words from the key, and the HMACs are now nested keyed hashes; before, one lane was unkeyed, allowing key-independent tag forgeries. IPC derives per-message keys with `fio_risky256_keyed`, keyed by a 32-byte secret window that rotates with the message `id`. RiskyHash 256/512 outputs change.

**Security**: (`resp3`, `redis`) strict RESP3 framing and length validation prevent reply misassociation, preserve attribute ownership, and enforce chunked-string payload budgets. Large chunks stream incrementally through bounded read buffers; malformed input and container-count or depth errors are rejected before allocation callbacks.

**Security**: (`files`) recursive removal normalizes trailing separators before checking links and holds no-follow directory descriptors on POSIX. Windows traversal holds reparse-safe directory handles without write/delete sharing to block directory substitution during descent.

**Security**: (`http1`) a rejected `Expect: 100-continue` (i.e., 413) now closes the connection instead of parsing the declared body as the next request (request smuggling). An over-limit chunked body no longer dispatches a NULL handle (crash) when its last chunk arrives in the same read.

**Security**: (`http1`) the parser rejects framing differentials: `gzip chunked` style transfer-coding lists, missing or bare-CR chunk data delimiters, incomplete empty lines after the last chunk, and status codes that aren't exactly three digits.

**Security**: (`http1`) a client receiving an over-limit response (body or headers) closes the connection instead of crashing or sending an error "response" to the server. `Expect` in a response is a regular header, so a server can no longer make the client write `100 Continue`.

**Security**: (`multipart`) fragmented part headers and fields no longer cause quadratic rescans (tens of seconds of CPU per 32 MiB request): the parser resumes its search and part headers are capped by `FIO_MULTIPART_MAX_HEADER_SIZE` (8192). Malformed closing delimiters (e.g., `--B--junk`, a partial CRLF) are errors, reported before the part's callbacks.

**Security**: (`multipart`, `http`) a closing delimiter ending exactly at the end of a read is no longer trusted: its part waits until the line's tail (`[padding] CRLF`, or the end of input) is validated, so `--B--garbage` split at a read edge is an error. Closing-line padding is capped like part headers.

**Security**: (`sha2`) SHA-256 / SHA-512 length counters no longer truncate to `size_t`, so streams over 4 GiB hash correctly on 32-bit targets, and SHA-512 encodes the full 128-bit length. HMAC-SHA-256/384/512 wipe key-derived stack state before returning and accept a `NULL` key with zero length.

**Security**: (`sha3`, `mlkem`) SHAKE partial-word output is now correct on big-endian systems. `fio_sha3_finalize` and the one-shot functions wipe their context, `fio_sha3_consume` accepts `(NULL, 0)`, and ML-KEM wipes its secret temporaries with `fio_secure_zero`.

**Security**: (`hkdf`) `fio_hkdf_extract`, `fio_hkdf_expand` and `fio_hkdf` return `int` (`0` or `-1`) and reject over-limit output lengths, PRKs shorter than the hash length, and `NULL` pointers with a length, without writing output (previously lengths were silently clamped and a huge `info_len` could wrap the work-buffer allocation). `NULL` info is empty info. Expand no longer allocates, reuses the keyed HMAC states per block (faster), and wipes its secret intermediates.

**Security**: (`otp`) `fio_otp` / `fio_otp_at` return the new `FIO_OTP_ERROR` for `digits` above 10 (before, a division by zero or a code below 15), Hex secrets with invalid digits (`"0G"` aliased `"10"`) or over 128 bytes (silently truncated), and Base32 secrets with invalid characters or data after `=` padding (before, silently decoded as a prefix). Secrets otherwise parse leniently: any case, whitespace / `-` / `:` separators, and optional or extra Base32 padding. Ten-digit codes are correct on 32-bit targets, keys over 64 bytes follow RFC 2104 (the nonstandard SHA-512 pre-hash is gone, so such keys now match authenticator apps), and key material is wiped. `fio_otp_print_key` documents its 33-byte requirement for generated keys and returns `0` instead of aborting on error.

**Security**: (`string core`) Base32 and Base64 decoding now check the whole input first. They return `-1` and leave `dest` unchanged on invalid characters, data after `=` padding or impossible lengths; before, they returned success with a partial or zero-filled result. Both stay lenient where decoding is unambiguous: whitespace, any amount of padding (or none), non-zero trailing bits and empty input are accepted, and Base64 still accepts both alphabets.

**API**: (`tls13`) the key schedule functions (`fio_tls13_hkdf_expand_label`, `fio_tls13_derive_*`, `fio_tls13_update_traffic_secret`) return `int` (`0` on success, `-1` on invalid arguments) instead of `void`.

**API**: (`string core`, `otp`) `fio_string_write_base32enc` pads output with `=` to a multiple of 8 characters (RFC 4648); before, it added a single `=`. `fio_otp_print_key` prints unpadded Base32 with no `=`, as authenticator apps expect, and no longer fails for keys whose length leaves 1 byte after the last 5-byte group.

**API**: (`multipart`) breaking: one stateful parser reports each part once, complete: `fio_multipart_init` copies the boundary, `fio_multipart_parse(parser, callbacks, udata, data, len, eof)` resumes block by block (`!consumed && !err` = more data) and `eof` replaces `fio_multipart_stream_parse` / `fio_multipart_stream_close`. The callbacks are `on_string` and `on_file` (complete, borrowed values); `on_field*`, `on_file_start/data/end` and `on_error` are gone. Callback errors are opaque and returned unchanged; the result's field and file counts are removed.

**API**: (`http`) breaking: `on_file` receives the complete upload (`data`, borrowed: RAM and memory mapped bodies are never copied) and returns an optional `{obj, err}` stored at the field name; `on_file_data` is removed. Truncated or aborted uploads never create a file object. Without a memory mapping, a pending part is read into the body's heap buffer, so the body size limit bounds memory use.

**Fix**: (`redis`) `fio_redis_free` transfers its reference to a release task on the main IO queue while the reactor is running, instead of destroying an engine on a calling worker thread. Otherwise it releases directly and must be called from the main thread, including during shutdown; no extra reference or lifetime state is added.

**API**: (`redis`) `fio_redis_state` returns a snapshot from any thread without traversing the command queue. `FIO_REDIS_STATE_STOPPED` is distinct from handshake failure (`FIO_REDIS_STATE_ERROR`); keep an engine reference alive while querying.

**API**: (`io`, `http`) `fio_io_connect` and `fio_http_connect` accept an opt-in `.verify` flag: a TLS connection without trust entries verifies the server against the system trust store, the same as the (now documented) `?trust=sys` URL query. Explicit trust is kept, plain connections ignore the flag, and unverified TLS clients remain the default.

**API**: (`io`, `openssl`, `tls13`) TLS servers with several `fio_io_tls_cert_add` certificates select one per handshake from the client's SNI, identically in both backends: case-insensitive exact match, then a single-label wildcard (`*.example.com`), else the default certificate (the one added with an empty server name, else the first one added). Each certificate is loaded once per context and indexed by name; before, OpenSSL presented the last certificate added, and the native backend sent all certificates as one chain with the last key (failing the handshake). `fio_tls13_server_on_hello` lets a native server pick its credentials once the ClientHello is parsed.

**Update**: (`p256`, `p384`, `x509`, `pem`, `tls13`) Compressed EC public keys (SEC 1 `02/03 || x`, allowed by RFC 5480) now work as they do in OpenSSL. `fio_ecdsa_p256_verify` accepts 33-byte and `fio_ecdsa_p384_verify` 49-byte keys (decompressed and checked against the curve), so certificate chains and TLS 1.3 handshakes with compressed P-256 / P-384 certificates verify, and native TLS servers load compressed P-256 certificates and key files. SEC1 private keys may embed a compressed public key (new `fio_pem_private_key_s.ecdsa_p256.public_key_len` field).

**Update**: (`data`) without a memory mapping, reads reuse the bytes the heap read buffer already holds and only read the missing ones (sliding or growing windows, `fio_data_read` after `fio_data_read_until`): spilled multipart uploads parse ~20% faster.

**Security**: (`websocket`) close writers treat a `NULL` reason as empty instead of emitting uninitialized stack bytes. The parser rejects non-minimal payload-length encodings (1002), and `FIO_WEBSOCKET_DEFAULT_MAX_FRAME` overrides above `UINT32_MAX` fail at compile time.

**Security**: (`entities`) numeric entity decoding respects the explicit source length, including truncated input passed through GFM and Markdown-to-HTML rendering.

**Security**: (`url`, `ipc`, `sock`) TLS diagnostics no longer print key, certificate, trust or passphrase values; IPC diagnostics and socket URL-length errors no longer dump complete URLs. URL parsing no longer reads past empty hosts, and TLS detection uses bounded reads for three-byte query names and scheme slices.

**Security**: (`signals`) handler snapshots and atomic pending consumption prevent mixed callback/data pairs and lost deferred notifications during review. Immediate NULL callbacks are ignored, and POSIX propagation forwards the original `SA_SIGINFO` arguments. Reactor stop handlers defer unsafe shutdown work to normal context.

**Security**: (`logging`) invalid `FIO_LOG_LENGTH_LIMIT` values are rejected at compile time. Truncated messages with limits from 3 through 128 no longer emit uninitialized stack bytes; the default limit was unaffected.

**Security**: (`redis`) error-response log fields escape control and non-ASCII bytes, preventing peer-supplied terminal commands. Returned error strings remain unchanged.

**Security**: (`atol`) `fio_aton` no longer reads 8 bytes past short `inf` prefixes such as `infi`, which could fault on RESP3 or other exact-length input. The wide-integer hex readers no longer read past empty input.

**Security**: (`imap`) inserts fail safely instead of writing past a full map, and colliding hashes no longer cause runaway memory growth. Growth leaves the map unchanged on failure, checks capacity arithmetic, and keeps the index aligned for odd-sized elements.

**Security**: (`glob`) `fio_glob_match` no longer reads past a pattern ending in `\` (which also wrapped the remaining length); a trailing escape is now a non-match. Malformed class ranges no longer form out-of-bounds pointers.

**Security**: (`ipc`) message IDs that cannot be recorded are dropped instead of processed, preventing replays. Filter truncation now keeps the newest 16,384 IDs instead of mostly the oldest.

**Security**: (`core`, `random`) `fio_secure_zero` uses unaligned-safe word stores followed by the new `FIO_COMPILER_GUARD_PTR(ptr)` barrier, which makes the buffer visible to the barrier so its stores cannot be removed as dead (a plain `FIO_COMPILER_GUARD` doesn't cover private stack buffers; no `memset`, same speed). Risky Hash 256/512 and their HMACs wipe internal state and padded blocks.

**Security**: (`ipc`) messages carry a fresh random `nonce` per encryption, so ChaCha20-Poly1305 keys never repeat even for forced or repeated timestamp/ID pairs; replies default to a fresh ID. Every connection's first message must be the peer's sealed UUID declaration (local: fresh within 1 second, same instance, opposite role; cluster: 30 second window, replay filter, unique UUID) before it receives broadcasts or has messages processed. Cluster duplicates are dropped before decryption, and cluster pings no longer close the connection.

**Fix**: (`extern`, `fiobj`) declarations-only `FIO_EXTERN` builds: programs using the library header (`lib/fio.h`) now link FIOBJ big numbers, floats and data objects (their internal helpers were declared `static` but never defined). With `FIO_EXTERN` + `FIO_EVERYTHING`, `FIO_EXTERN` no longer gets dropped partway through the header, so later modules (IO, TLS 1.3, HTTP) stay declarations-only instead of failing to compile. Debug builds split across translation units report each leak counter once (from the `FIO_EXTERN_COMPLETE` unit) instead of once per unit; `make tests` now includes a split-build regression test (`make tests/extern-split`).

**Fix**: (`core`, `http`, `gfm`, `map`, `brotli`, `cli`) 32-bit builds: layout assertions in the HTTP params and GFM parsers no longer assume 8-byte pointers (a hard compile error on 32-bit), `fio_brotli_decompress_bound` saturates instead of overflowing, and the PRNG call counter's 64-bit atomic no longer falls back to a lock-based library call on i386. Headers now compile cleanly under `-Wall -Wextra` (no `-Wmissing-field-initializers`, GCC `-Wnonnull` or 32-bit `-Wformat` / `-Wsign-compare` warnings).

**Fix**: (`ipc`, `pubsub`, `io`) cluster messages, cluster declarations, UDP discovery and pub/sub message timestamps use the wall clock (`fio_io_last_tick_time`) instead of the monotonic tick, so hosts that didn't boot within 30 seconds of each other no longer reject each other's messages (cluster hosts need synchronized clocks). UDP discovery wrote its timestamp big-endian but read it little-endian, so discovery messages always failed the time window. `fio_io_last_tick_time` is now initialized at startup and refreshed like `fio_io_last_tick`.

**Fix**: (`io`) `fio_io_last_tick` and `fio_io_last_tick_time` return the current time while the reactor is stopped, and `fio_io_run_every`, `fio_io_async_every` and new IO objects use them. Timers scheduled between reactor runs used the previous run's last tick, so repeating timers fired a burst of catch-up calls on start, and IO attached before `fio_io_start` could time out early.

**Security**: (`websocket`) generated client masks use all 32 random bits instead of forcing four bits on.

**Security**: (`p256`) ECDSA signing no longer reduces secret scalars with variable-time division. Scalar reduction now uses fixed-work Barrett reduction, which also makes it about 200 times faster.

**Security**: (`sort`) quicksort falls back to heap sort when partitioning degenerates, so crafted input can no longer force O(n²) comparisons. `FIO_SORT_THRESHOLD` values below 3, which caused out-of-bounds access, are rejected at compile time.

**Security**: (`pubsub`) the history cache no longer dereferences NULL when a message is cached in a channel after an eviction. Eviction now removes the oldest message in that case.

**Security**: (`mem`) `realloc` of page-mapped memory rejects sizes that wrap the mapping length, which could unmap a live allocation. Failures keep the old allocation and set `ENOMEM`. `calloc` and `calloc_aligned` catch every product overflow, and `calloc(1, SIZE_MAX)` no longer returns the shared zero-size pointer.

**Security**: (`string`) fixed-size and allocation-failed writes no longer overflow: JSON escaping and `write2` floats truncate in bounds, and `fio_string_replace` rejects wrapping ranges. Truncated `\x`, octal, `%u` and HTML entities no longer read past the input, and HTML unescaping no longer under-reserves `&#NN` entities without `;`. Shrinking a shared `fio_bstr` and failed `fio_bstr_len_set` growth no longer write out of bounds, and key-string OOM no longer borrows the caller's buffer.

**Security**: (`mem`) in-place `realloc2` zeroes old bytes past `copy_len`, as documented for `FIO_MEMORY_INITIALIZE_ALLOCATIONS`. In-place growth no longer reserves more block space than requested.

**Fix**: (`http1`) the client accepts status lines with an empty or omitted reason phrase (`HTTP/1.1 200 ` or `HTTP/1.1 200`).

**Fix**: (`files`) zero-length bounded filename parsing no longer reads its input. POSIX temporary creation fails if the name cannot be unlinked; save cleanup failures are logged, and the documentation states that atomic replacement does not preserve metadata and cleanup can leave a sibling after a filesystem error.

**Fix**: (`entities`) numeric entities reject dividers and duplicate hex prefixes, and surrogate code points become U+FFFD. Named-entity case folding no longer treats control bytes as digits.

**Fix**: (`url`) component scans preserve non-ASCII bytes regardless of `char` signedness. Colon lookahead and query termination keep cursor arithmetic within the input, and zero-length parsing permits a NULL input.

**Fix**: (`threads`) Windows thread creation uses a CRT-compatible callback trampoline and retains handles until join/detach; child polling closes handles and returns the child PID. Finite condition waits no longer wrap or become infinite, POSIX create/lock/unlock/priority failures honor their documented `-1` result, and invalid priorities fail. Multi-threaded copy treats a failed worker join as fatal rather than returning with live buffer access.

**Fix**: (`string`) JSON and URL unescaping pair every high surrogate (`D800`–`DBFF`) and replace lone surrogates with U+FFFD. JSON escaping now escapes overlong and surrogate UTF-8, and `fio_string_readfd` reports short reads.

**Fix**: (`signals`) failed removal preserves the watcher, review counts only invoked callbacks, and Windows monitor slots are reusable. Windows rearms before callbacks without overwriting the saved handler; the CRT reset-before-entry window remains. Reactor and OpenSSL check signal setup, and shared SIGPIPE suppression survives reactor shutdown.

**Fix**: (`mem`) cleanup at exit is skipped, instead of unlocking another thread's lock, while the allocator is in use. Out-of-memory in `DEBUG` `fio_malloc` or no-initialization `fio_calloc` returns `NULL` instead of crashing.

**Fix**: (`mem`) allocation failures set `ENOMEM`, and the `posix_memalign` override never reports success on failure. Windows releases a reservation when its commit fails. `FIO_MEMORY_SYS_ALLOCATION_SIZE_LOG` values outside 17–24 fail to compile.

**Fix**: (`mem`) `FIO_MALLOC_OVERRIDE_SYSTEM` no longer hangs at startup, and freeing big allocations after exit cleanup no longer crashes. The override also replaces `memalign` and `valloc` (plus `pvalloc` and `reallocarray` on Linux), and rejects invalid alignments with `EINVAL`.

**Fix**: (`imap`) `rehash` returns `0` on success. `remove` returns `-1` for maps where zeroed elements stay valid, instead of leaving them visible.

**Fix**: (`atol`) hex float exponents saturate to infinity or zero instead of wrapping, so `0x1p18446744073709551615` is no longer `0.5`. Hex subnormals such as `0x1p-1073` no longer become zero, and zero with a huge exponent is no longer NaN.

**Fix**: (`atol`) `fio_u2d` / `fio_i2d` scale subnormals correctly, return infinity rather than NaN at the top exponent, and accept `INT64_MIN`. The optional `FIO___ATON_TIE2EVEN` mode now rounds correctly to nearest, ties to even.

**Fix**: (`atol`) `fio_u128_hex_read` and its wider variants parse digits correctly and stop at the first non-hex character. The hex writers NUL-terminate zero.

**Fix**: (`atol`) `fio_atol_xbase` reports `E2BIG` before an overflow wraps. It and the generic-base helpers reject bases outside 2-36 instead of dividing by zero or looping.

**Fix**: (`memalt`) `fio_strlen` stops at NUL without reading beyond the string, and scalar `fio_memchr` finds the correct first match on big-endian targets. Copy overlap checks and SIMD search loops avoid invalid pointer operations. `fio_memcpy` always returns the destination, and `fio_memcmp` returns exactly `-1`, `0`, or `1`.

**Fix**: (`logging`) empty messages produce a newline, formatting failures are identified correctly, and logging preserves `errno` even when output fails. Logs remain best-effort; log-level changes require application synchronization.

**Fix**: (`logging`) restored compiler checks for format arguments and corrected mismatches in library and test messages.

**Fix**: (`patches`) Windows environment reads reject oversized values and distinguish empty values from errors. Positional writes wait for completion, validate offsets and counts, and restore the file cursor; shared-cursor operations still require caller synchronization. Clock conversion no longer shares mutable state, and file metadata uses 64-bit sizes.

**Fix**: (`files`) Windows temporary files use exclusive child names rather than an `O_TMPFILE` alias. Overlong temporary-directory settings correctly fall back to the current directory.

**Fix**: (`math`) `fio_math_shl` / `fio_math_shr` handle whole-word, oversized, and in-place shifts without invalid pointers, and `fio_math_inv` propagates carries across words. `fio_math_div` rejects lengths above 1024 words instead of using unbounded stack space.

**Update**: (`string`) `fio_string_write2` writes floats with `fio_ftoa` instead of `snprintf("%.15g")`, matching FIOBJ and Redis output. Floats now use `%g` precision (6 significant digits), and integral values end in `.0`.

**Fix**: (`core`) build support clarification: the C11 `<stdatomic.h>` atomics fallback is removed and now stops with an `#error`. `fio_atomic_*` operates on plain (non-`_Atomic`) storage, which the C11 generics do not accept (strict compilers reject it at compile time). Only C11 compilers lacking GNU-compatible `__atomic` / `__sync` builtins and MSVC `Interlocked` intrinsics were affected; GCC, Clang and MSVC never selected this branch, so their builds and performance are unchanged. Not a runtime vulnerability.

### 0.8.3 - 2026-10-26

**Security**: (`sha2`) SHA-384 and SHA-512 returned wrong digests for inputs 112 bytes past a multiple of 128. This failed about 1 in 128 TLS 1.3 handshakes using `TLS_AES_256_GCM_SHA384`, and affected HMAC / HKDF with SHA-384 / SHA-512 for those lengths.

**Security**: (`http`) `fio_http_mimetype` overflowed a stack buffer for file extensions longer than 7 bytes, such as a static file request for `notes.markdown_old`. Long extensions now return an empty MIME type.

**Security**: (`string core`) `fio_string_write_escape`, used for JSON output, could write past its buffer when the input ended in a truncated UTF-8 sequence. Truncated bytes are now escaped as `\xXX`.

**Fix**: (`http`) `fio_http_etag_is_match` follows RFC 9110 §13.1.2: weak comparison, every `If-None-Match` tag is checked, and `*` matches. Only 2xx `GET` / `HEAD` responses become `304`, and a tag that only shares a prefix with the `etag` no longer matches.

**Fix**: (`http`) `fio_http_upgrade_websocket` no longer upgrades after sending `400` for an unsupported `Sec-WebSocket-Version`.

**Fix**: (`http`) `*_header_set_if_missing` with an empty value no longer removes an existing header.

**Fix**: (`http`) `fio_http_write` with `copy` set no longer closes `fd` on error paths, WebSocket client writes or SSE.

**Fix**: (`io`) `fio_io_write2` on a `NULL` IO now closes `fd` when `copy` isn't set, even without a `dealloc` callback.

**Fix**: (`http`) `fio_http_is_clean` returns false once headers were sent.

**Fix**: (`http`) `fio_http_from` falls back to `x-forwarded-for` when a `forwarded` header has no `for=`, handles `forwarded` headers of 1024 bytes or more, and no longer reads `xfor=` as `for=`.

**Fix**: (`http`) `FIO_HTTP_PATH_EACH` decodes a `%xx` at the end of the path.

**Fix**: (`http`) `fio_http_body_read_until` on a RAM body stops after `limit` bytes when the token isn't found, instead of jumping to the end of the body.

**Fix**: (`http`) returning `NULL` from `on_file` skips the upload: `on_file_data` isn't called.

**Fix**: (`http` client) requests include the URL query string. RAM request bodies are no longer sent as an offset from the handle's address (undefined behavior).

**Fix**: (`multipart`) `fio_multipart_parse` no longer returns `consumed == 0` when the input ends right after a delimiter, which made callers report complete parts twice.

**Fix**: (`json`) `FIO_JSON_MAX_DEPTH` allows the full number of nesting levels, counting the root as level 1; one level was lost before. `0` selects the default (128).

**Fix**: (`atol`) `fio_aton` rounds decimal floats correctly, matching `strtod`; results were up to 2 ulps off, and extreme values became `0` or `inf`. `fio_aton` and `fio_atol` now follow the documented formats: `012.5` is decimal, bare `x1F` is hex, `b101` is binary, and input without digits isn't consumed.

**Fix**: (`files`) `fio_filename_tmp` no longer leaves temporary files behind. Files are unlinked right after opening (POSIX) or opened delete-on-close (Windows).

**Fix**: (`files`) `fio_fd_read` returns `0`, not `(size_t)-1`, when its `lseek` fallback fails. On Windows, `fio_filename_make_path` sets `errno` on failure.

**Fix**: (`sock`) public Unix sockets (`FIO_SOCK_UNIX`) are `0777` again, so other users can connect; an always-false guard skipped the `chmod`. Socket creation no longer changes the process-wide `umask`, which raced other threads, and `FIO_SOCK_AVOID_UMASK` is deprecated.

**Fix**: (`sock`) back-to-back `fio_sock_peer_addr` calls no longer overwrite each other's results.

**API**: (`data`) new Data Store module (`FIO_DATA`): a seekable buffer that keeps data in RAM up to a limit, then moves it to an anonymous temporary file. Spilled data is read through a memory mapping, write errors are sticky (`fio_data_is_error`), and `fio_data_save` saves the content atomically. HTTP bodies use it.

**API**: (`fiobj`) added `FIOBJ_T_DATA` (`fiobj_data_new`, `fiobj_data_ptr`), which wraps a `fio_data_s` and holds upload metadata (`fiobj_data_filename`, `fiobj_data_type`, `fiobj_data_headers`).

**API**: (`http`) added `fio_http_query_parse` and `fio_http_params_parse`, which parse the query and body into one map. Field names may be nested (`user[name]`, `tags[]`), and the `max_params` setting limits the fields parsed per call (default 4096). Also added `fio_http_body_save`.

**API**: (`http` + `fiobj`) added `fio_http_params2FIOBJ`, which returns the request params as a FIOBJ Hash or merges them into an existing one. File uploads become `FIOBJ_T_DATA` objects. It's available once both modules are included, in any order.

**API**: (`http`) breaking: params parsing uses the JSON callback contract, so callback errors are returned unchanged and the caller owns the result, which is partial on error. The types are renamed `fio_http_params_parse_callbacks_s` / `fio_http_params_parse_result_s`, `on_error` and `on_file_done` were removed, `on_file` receives the raw part headers, and the parse functions take named arguments (`.callbacks`, `.udata`, `.limit`). Text values are always Strings, so URL-encoded bodies no longer guess types. Upload file names lose their path components.

**API**: (`http`) breaking: `fio_http_body_write` returns `int` (`-1` on error), and an HTTP/1.1 server responds `500` when it can't store a request body. Negative `fio_http_body_seek` positions count back like `lseek(SEEK_END)`, so `-1` is the last byte; the `SSIZE_MAX` "keep position" value was removed.

**API**: (`http`) breaking: routes no longer inherit the listener's `on_stop`, which could free shared `udata` more than once. `connect_timeout` is now a `uint16_t` in milliseconds, so timeouts above 255 ms work.

**API**: (`json`) breaking: new callback contract. Callbacks return result structs with an `int err` that `fio_json_parse` returns unchanged, push callbacks always take ownership of the value, keys arrive decoded, and `on_map` / `on_array` place the new container in its parent and hand back any value they replace. `on_error`, `map_finished` and `array_finished` were removed: the caller owns the result, which is partial on error. `fio_json_parse` takes a `const` callback table and an optional `root` to update in place. Added `fio_json_resolve_*` helpers for the shared conflict rules, `fio_json_callbacks_validate`, and `FIO_JSON_ERROR_GENERIC` / `FIO_JSON_ERROR_NESTING`; numbers always use `fio_aton` (`FIO_JSON_USE_FIO_ATON` was removed).

**API**: (`fiobj`) `fiobj_hash_update_json` merges nested objects and appends nested arrays instead of replacing them.

**API**: (`urlencoded`) breaking: `fio_url_encoded_parse` decodes names and values before calling `on_pair`. Remove any decoding in your callbacks. `on_pair` returns an `int`: `0` continues, and any other value stops parsing and is returned unchanged in `result.err` (the parser's own error is `-1`, on allocation failure). `on_error` was removed, and `udata` is passed unchanged. On error, `consumed` and `count` exclude the failed pair. A `NULL` `on_pair` only counts pairs, without decoding.

**API**: (`multipart`) added `fio_multipart_stream_parse` and `fio_multipart_stream_close`, which stream file parts in chunks. HTTP uses them, so uploads are never fully loaded into memory. Breaking: `on_file_start` receives the raw part headers.

**API**: (`files`) `fio_filename_overwrite` replaces the file atomically: it writes a sibling file, calls `fsync` and renames it. New files get mode `0666` minus the `umask`. Added `fio_fd_save`, which does the same from a file descriptor.

**API**: (`atol`) `fio_atof` uses `fio_aton`, so results are correctly rounded and locale independent. `fio_ftoa` base 16 writes C99 `%a` hex floats and base 2 writes the raw bits (`0b...f`); both read back exactly.

**API**: (`io`) TLS clients send SNI. `fio_io_connect` sets it from the URL host name, and `fio_io_tls_sni_set` sets it directly. IP literals are skipped, as RFC 6066 requires.

**API**: (`str`) added `fio_string2primitive`, which detects `null`, `true`, `false` and numerals in a String.

**API**: (`core`) added `FIO_STR_INFO_TMP_RESET`, which resets a `FIO_STR_INFO_TMP_VAR`.

**Update**: (`atol`, `string core`, `json`) `fio_ftoa` base 10 is about 4x faster, `fio_aton` up to 2x faster, and JSON string escaping and scanning about 2x faster.

**Update**: (`http`) rewrote the HTTP API documentation, with exact signatures, return values, ownership and limits.


### 0.8.2 - 2026-10-22

**Security**: (`ipc`) cluster peers must complete the UUID handshake before any other message is handled. Messages sent before the handshake are rejected. A peer is refused when its UUID is zero, equals this instance's UUID, or was already connected. Each instance now draws a non-zero UUID at startup.

**Fix**: (`http`) `fio_http_body_read` overflow protection when a caller passes a very large length.

**Fix**: (`http1` client) the offset into the request header buffer was calculated backwards when sending an HTTP/1 request.

**Fix**: (`ipc`) periodic UDP discovery broadcasts ran only once after startup, because `repetitions = 0` was treated as a single run. Broadcasts now repeat every 4–5 seconds, with jitter.

**API**: (`http`) added case-insensitive method tests for `HEAD`, `OPTIONS`, `CONNECT`, `TRACE`, `COPY`, `MOVE`, `LOCK`, `UNLOCK`, `SEARCH`, `MKCOL`, `PROPFIND`, and `PROPPATCH`: `fio_http_method_is_head`, `fio_http_method_is_options`, and so on.

**API**: (`core`) `fio_buf_info_is_eq` provides a safe `!memcmp` alternative. `FIO_BUF_INFO_IS_EQ` and `FIO_STR_INFO_IS_EQ` call it when `FIO_MEMALT` is set.

**API**: (`ipc`) added `fio_ipc_cluster_on_peer` to receive cluster peer attach and detach events. Register up to 32 subscribers on the master before `fio_io_start`. Callbacks borrow each event; call `fio_ipc_dup` to keep one. An attached event holds a reference to the peer IO, and its data holds the peer UUID.


### 0.8.1 - 2026-10-15

**API**: (`http`) added case-insensitive method helpers for `GET`, `PUT`, `POST`, `PATCH`, `QUERY`, and `DELETE`: `fio_http_method_is_get`, `fio_http_method_is_put`, `fio_http_method_is_post`, `fio_http_method_is_patch`, `fio_http_method_is_query`, and `fio_http_method_is_delete`.

**API**: (`http`) HTTP response state and compression-control flags are now private implementation details. Applications should inspect handles through `fio_http_is_clean`, `fio_http_is_finished`, `fio_http_is_freeing`, `fio_http_is_streaming`, `fio_http_is_upgraded`, `fio_http_is_websocket`, and `fio_http_is_sse`. Configure compression through listener or route settings instead of controller flags.

**Fix**: (`http`) public inline helpers, path iteration, and resource-action routing now live with the public HTTP API declarations. This keeps them available when the library uses separate declarations and implementation through `FIO_EXTERN`.

**Update**: (`http`) clarified the public API documentation, including controller assignment, static-file compression, route-relative resource paths, and reserved controller fields.

**Update**: (`p256`, `rsa`) adjusted ignored arithmetic results to avoid compiler warnings. Cryptographic behavior is unchanged.



### 0.8.0 - 2026-10-12

This release includes the fixes and API changes below, plus the changes recorded under the earlier `edge` and `v.0.8.0.rc.01` entries. The `edge` tag remains a rolling prerelease, not the numbered 0.8.0 release.

**Fix**: (`websocket`) generic client writes now frame and mask their payloads, including writes sourced from a file descriptor. The client no longer sends an unmasked payload while setting the WebSocket MASK bit. Round-trip coverage includes client and server messages.

**Fix**: (`string core`, `http`) URL decoding now reserves enough space for decoded `%XX`, `%uXXXX`, and literal `%` sequences at allocation boundaries. HTTP body parsing has JSON escape and URL-encoded form boundary regressions, including concurrent JSON decoding coverage.

**Fix**: (`string core`) Base32 encoding and decoding now handle padding and partial final groups correctly. The 3-byte round-trip case and RFC 4648 vectors have regression coverage.

**Fix**: (`fx86`, `random`) portable vector fallbacks initialize their result storage before use; this includes vector results in the random module. (`ipc`) cluster peer UUID transfer no longer assumes function pointers occupy 16 contiguous bytes; it copies the UUID across the two pointer-sized slots.

**Security**: (`rand`) `fio_rand_bytes_secure` always failed on Windows (no `/dev/urandom`), so callers either errored or silently used the non-cryptographic PRNG (RSA blinding, RSA-PSS salt, `fio_otp_generate_key`). Windows now uses `BCryptGenRandom` (`BCRYPT_USE_SYSTEM_PREFERRED_RNG`; MSVC auto-links `bcrypt.lib`, **MinGW / GNU-driver builds must link `-lbcrypt`**). Linux prefers `getrandom` (no fd, works in chroots), falling back to `/dev/urandom` (now opened `O_CLOEXEC`) when the syscall is unavailable or blocked. A `NULL` target with `len > 0` now returns `-1` instead of a false success.

**Security**: (`http1 parser`, `http`) request smuggling via `Transfer-Encoding`. A request whose `Transfer-Encoding` did not end with `chunked` (i.e., `gzip`, `identity`, `chunked, gzip`) was treated as bodyless, so its body was parsed as the next pipelined request; `chunked` applied twice (`chunked, chunked`) was accepted. Per RFC 9112 §6.3 / §7, the parser now rejects (the server responds `400` and closes) any request whose coding list (all field lines, RFC 9110 §5.3) does not end with a single `chunked`. `gzip, chunked` is still accepted: it is framed as chunked and `transfer-encoding: gzip` stays visible to the application (a `501` is only a SHOULD, RFC 9112 §6.1). Responses with such framing are rejected too, rather than misread.

**Fix**: (`queue`) worker threads could miss a wakeup and leave a pushed task in the queue until the next push. The idle worker checked for work and then waited on its condition variable, but pushers signaled without holding that mutex. If the signal arrived between the check and the wait, it was lost. Workers now register as sleeping, then check `fio_queue_count`, both under the group mutex, before waiting. Pushers signal (holding the group mutex) only groups that have sleeping workers, so busy queues don't pay for the mutex. `fio_queue_destroy` now updates the task-ring leak counter when it discards pending tasks, so destroying a full queue no longer reports false leaks. `fio_queue_workers_stop` now returns the number of worker groups it signaled. Pending tasks are performed only by `fio_queue_workers_join` (and `fio_queue_destroy`) on a queue with workers; stopping workers, or destroying a queue without workers, may leave tasks unperformed.

**Fix**: (`http1 parser`, `http`) HTTP/1.1 requests without a `Host` header, or with more than one, now receive `400` and the connection closes (RFC 9112 §3.2). HTTP/1.0 requests may omit `Host`. New `fio_http1_version_is_legacy` helper (exactly `HTTP/1.0`; every other version gets HTTP/1.1 semantics, HTTP/0.9 is unsupported).

**Fix**: (`http`) HTTP/1.0 persistence and framing. HTTP/1.0 requests without a `keep-alive` connection option now close after the response drains (RFC 9112 §9.3; previously the connection was held open, which stalled ApacheBench without `-k`), and kept-alive HTTP/1.0 responses confirm with `connection: keep-alive`. Streamed responses to HTTP/1.0 are no longer sent with `transfer-encoding: chunked` (RFC 9112 §6.1); they are sent unframed and delimited by closing the connection.

**Fix**: (`http`) responses without content (`1xx`, `204`, `205`, `304`) were framed like any other response. Every finished response got an automatic `content-length` (`204` sent `content-length:0`, and the WebSocket `101` upgrade sent one too), against RFC 9110 §8.6. Worse, a `205` written with a body declared `content-length:<body length>` but sent no body, and a streamed `204` / `304` was sent with `transfer-encoding: chunked` plus a `0\r\n\r\n` terminator. Both desynchronized keep-alive clients. Now: `1xx` / `204` never send `content-length`, `205` always sends `content-length: 0`, and `304` keeps an application-provided `content-length` but gets none automatically. `transfer-encoding` is removed, and these responses are never streamed or compressed.

**Fix**: (`http`) responses to `HEAD` requests sent the body written by the application (only the static file helper special-cased `HEAD`), against RFC 9110 §9.3.2. A keep-alive client then misread the body as the next response. The decision is now made once, when the response starts: `HEAD` responses and no-content statuses discard body writes and are never streamed. A finished `HEAD` response keeps its automatic `content-length`, as `GET` would send.

**Fix**: (`http1`) the first large (64 KiB or more) no-copy or file chunk of a streamed response was sent without its chunk-size line, which corrupted the chunked framing. The chunk header was appended to the pending response-header buffer with a non-growing (`NULL`) reallocator, so the write failed whenever that buffer had no spare capacity, which in practice was always. Both paths now share `fio___http1_write_chunk_header`, which grows the buffer it owns.

**Update**: (`http`) an empty `fio_http_write` without `finish` no longer starts the response. Previously it committed the response to streaming, so an empty stream was sent as `transfer-encoding: chunked` with an empty `0\r\n\r\n` body. The response now starts with the first write carrying body data, or with `finish`. An empty stream is sent with `content-length: 0`, and response headers may still be set after such an empty write (`fio_http_is_clean` stays true).

**Fix**: (`http1 parser`) Windows release builds (MSVC `/O2`, clang + `/OPT:ICF`) sent a spurious `HTTP/1.1 100 Continue` before every `GET` / `HEAD` / `OPTIONS` response. The `Expect: 100-continue` state was encoded as function pointer identity (`fio_http1___read_header_post_expect`), but that stage function had a body identical to `fio_http1___read_header`, and identical-code-folding linkers merged the two, so the state test was always true. `fio_http1_parser_s` now carries a `size_t flags` word (state is never inferred from function pointer identity), and `on_expect` only fires once, and only when a body may follow (RFC 9110 §10.1.1).

**Fix**: (`http1 parser`, `http` client) responses without a body (RFC 9112 §6.3): `1xx`, `204` and `304` responses now complete at the end of their headers even when they carry `content-length` / `transfer-encoding` (those headers are forwarded as regular headers). New `fio_http1_parser_skip_body` lets the parser user mark a message as bodyless; the HTTP client uses it for `HEAD` responses, which previously waited for a body that never arrives. New `fio_http1_parser_skips_body` reports bodyless messages as soon as the first line is parsed (GET / HEAD / OPTIONS requests, 1xx / 204 / 304 responses, marked messages); a bodyless message's `content-length` is still reported to `on_header_content_length` as an informational value, and the HTTP layer checks `skips_body` before enforcing `max_body_size` or reserving body space (a `HEAD` response for a large file no longer looks "too big").

**Fix**: (`http`) a server connection receiving an HTTP response line tripped a debug assertion (`on_version called without a pre-existing handle!`). It is now logged with `FIO_LOG_SECURITY` and the connection is closed (also for unsolicited responses received by a client).

**Fix**: (`http` client) interim `1xx` responses other than `101` (i.e., `100 Continue`, `103 Early Hints`) were reported to `on_http` as the final response. They are now skipped (RFC 9110 §15.2); `on_http` runs once, for the final response, without interim headers.

**Fix**: (`io`, `poll`) use-after-free / double-free of IO objects on Windows (Iodine report: `FATAL: fio___io 'free' after 'free' detected`, Ruby-side `undefined method 'on_close' for nil`). Root cause was two compounding bugs: (1) `fio___io_monitor_forget` skipped the actual `fio_poll_forget` whenever the io-level event flags had already been consumed, so the poll/WSAPoll backend retained the fd entry — with its raw `udata` — after the IO was destroyed; (2) the poll backend's one-shot flag strip (`events &= ~revents`) is partial on Windows, where WSAPoll reports sub-band bits (`POLLRDNORM`) while `POLLIN` is a superset — leaving `POLLRDBAND` armed. The next review then dispatched `POLLNVAL` for the closed socket with the freed `udata`, resurrecting the destroyed IO (`dup2` 0→1) and destroying it a second time. POSIX `poll` strips `POLLIN` fully and kqueue filters self-clean on `close()`, hence Windows-only. Fixes: destroy now always forgets, and the dispatch strip removes whole event groups. Reproduced and verified with a connect-lifecycle hammer (plain/TLS × success/refused/invalid-host/timeout/TLS-mismatch; 200 rounds × 8 variants) on macOS (kqueue+OpenSSL, poll+tls13/asan) and Windows via Wine (WSAPoll + native TLS); audit trail in `ai-cache/poll-fix/refcount-audit.md`.

**Fix**: (`io`) `fio_io_attach_fd`'s invalid-fd error path passed the TLS **context** to the connection destructor (`cleanup` expects the per-io connection created by `start`) — type confusion and double free on TLS connect attempts that fail before attaching (e.g., DNS failure). The ownership contract is now explicit (`401 io api.h`): `tls` ownership transfers to the reactor whenever `fio_io_attach_fd` is called, success or failure; the error path releases it with the correct destructor (`free_context`), and `fio_io_connect` handles never-attached failures itself so each context is released exactly once.

**Fix**: (`io`) a failed non-blocking connect could be promoted to the application protocol when the writability event raced the error report, swallowing `on_failed`. Promotion now checks `SO_ERROR` first (new `fio_sock_error` helper in `004 sock.h`). Additionally, the promotion's protocol swap is now performed synchronously (with an explicit reference), so no queued task can observe the connecting protocol paired with the application's `udata` — previously that consistency relied on queue FIFO ordering alone.

**Fix**: (`poll` backends) event-dispatch parity: a broken socket no longer reports readiness. The kqueue backend fired `on_ready` for any `EVFILT_WRITE` event even when it carried `EV_EOF`/`EV_ERROR`, and the epoll backend fired `on_ready` for `EPOLLOUT` even when the same event carried `EPOLLERR`/`EPOLLHUP` — so a failed non-blocking connect could reach the protocol's `on_ready` (and be promoted) before the close was processed. Both backends now dispatch `on_close` instead of `on_ready` when the event carries error/hangup flags, matching the poll backend (error bits win over `POLLOUT`). Verified on all three backends (macOS/kqueue, Linux/epoll, Wine/WSAPoll) with the connect-lifecycle hammer.

**Fix**: (`poll`) the poll backend's fd map used `FIO_IMAP_ALWAYS_VALID`, so removed (zeroed, `fd == 0`) slots stayed "valid": the map never shrank, rehashes resurrected removed entries as `fd 0` zombies, and duplicate `fd 0` entries then failed every rehash — an unbounded expansion cascade that could also make live entries unreachable (`fio_poll_forget` returning -1 for a monitored fd). Entry validity is now socket-based (`FIO_SOCK_FD_ISVALID(...) && fd`); note fd 0 can no longer be monitored by the poll backend (poll only monitors sockets).

**Update**: (`io`) DEBUG-build tripwire: the reactor thread is recorded on first IO-layer use (via `fio_thread_nid`) and `fio___io_tick` / `fio___io_free_task` assert they run on it — catching violations of the single-IO-thread funnel (e.g., worker threads attached to the main queue) at debug time instead of as memory corruption.

**Update**: (makefile) new build patterns `tsan/<target>` and `<target>/tsan` — ThreadSanitizer builds mirroring the `asan/%` / `ub/%` presets.

**Update**: (`release`) The rolling `edge` candidate is now selected nightly rather than once per CI completion. It moves the `edge` tag explicitly and refreshes the prerelease asset only for the newest `master` commit that passed the POSIX, macOS, MSVC, Clang, and full nightly suites.

### edge (2026-08-08)

**Fix**: (`tls13`, `sha2`, `hkdf`) TLS 1.3 cipher suites using SHA-384 (`TLS_AES_256_GCM_SHA384`) now inter-operate with real TLS peers. The SHA-384 paths previously ran SHA-512 primitives: the transcript hash used the SHA-512 IV (FIPS 180-4 requires different SHA-384 IVs) and HMAC was SHA-512 truncated to 48 bytes, which is NOT HMAC-SHA-384. Added `fio_sha384_hmac` (`152 sha2.h`); HKDF extract/expand (`152 sha2z hkdf.h`), the transcript hash, the empty-string hash and the Finished computation (`190 tls13.h`) now use real SHA-384. The TLS client additionally feeds BOTH transcript hashes until the ServerHello cipher-suite selection is known (previously the ClientHello/ServerHello only landed in the SHA-256 transcript, breaking SHA-384 suite negotiation). New OpenSSL interop gate in `stress/tls13-openssl-roundtrip.c` asserts `TLS_AES_256_GCM_SHA384` negotiation in both directions (OpenSSL client and OpenSSL server).

**Fix**: (`tls13` IO) application data received in the same read batch as a following alert record (e.g., payload + `close_notify` coalesced into one TCP segment) was silently discarded when the alert record reported an error. Plaintext already produced in the batch is now delivered first; the error surfaces on the next read.

**Fix**: (`x509`) `fio_x509_self_signed_cert` could emit a certificate whose TBSCertificate SEQUENCE declared a longer content than actually encoded: the 16-byte serial INTEGER was sized at a fixed 18 bytes, but `fio_der_encode_integer` strips leading zero bytes from the random serial (whenever the masked first byte came out 0x00). Strict DER parsers (e.g., OpenSSL) reject such certificates with a sequence length mismatch. The first serial byte is now forced non-zero, so the serial encoding is always exactly the sized 18 bytes.

**Update**: (tests) `tests/hkdf.c` SHA-384 cases upgraded from non-zero smoke assertions to real vector assertions (RFC 5869 parameter sets computed with an OpenSSL oracle — derivation script in `./ai-research/`); `tests/sha.c` gained RFC 4231 HMAC-SHA-384 vectors covering `fio_sha384_hmac` (including the over-block-size key path).

**Fix**: (`memalt`) `fio_memcpy` no longer routes overlapping copies to a raw `memcpy` path (undefined behavior; caught by the new `asan` build pattern). Overlapping ranges always take the buffered/reversed (memmove-safe) paths.

**Fix**: (`core`) the vector rotation macros (`FIO_MATH_UXXX_OP_RROT/CRROT/LROT/CLROT`, i.e., `fio_u*_rrot*` / `fio_u*_crrot*` / `fio_u*_lrot*` / `fio_u*_clrot*`) now mask the rotation count on both shifts — rotating by the lane width (e.g., `fio_u256_crrot64(., 64)`) was shift-by-width undefined behavior and returned garbage under some optimizer/sanitizer combinations.

**Fix**: (`tls13`) `FIO_NO_TLS` now correctly selects the native TLS 1.3 implementation as the IO default even when OpenSSL was detected — previously `HAVE_OPENSSL` + `FIO_NO_TLS` left no registered implementation (`build_context` failed with "no SSL/TLS implementation found").

**Update**: (makefile) new build patterns `asan/<target>` and `ub/<target>` (also `<target>/asan`, `<target>/ub`) — AddressSanitizer / UBSan (fatal) builds that mirror the Windows CI environment (`-DFIO_MEMORY_DISABLE -DFIO_MEMALT -DFIO_NO_TLS`, native TLS). Detected `-DHAVE_*` defines moved from `CFLAGS` to `FLAGS`; `DEBUG_CFLAGS` / `ASAN_CFLAGS` / `UB_CFLAGS` presets live at the top of the makefile. OpenSSL-dependent units are auto-excluded from sanitizer aggregates.

**Security**: (`files`, `http1 parser`, `resp3`, `atol`, `json`, `http`) proactive security audit fixes and hardening:

- (`files`) `fio_filename_is_unsafe` now guards BOTH `/` and `\` on Windows (Win32 APIs accept either separator); previously only `\` was guarded, leaving Unix-flavored traversal (`foo/../../secret`) open on Windows. Regression tests added to `tests/files.c`.
- (`http1 parser`) fixed three 1-byte out-of-bounds reads (ASAN-verified): `eol[-1]` underflow when an empty header line starts at the buffer; whitespace skip reading one byte past an all-whitespace buffer; and the `Transfer-Encoding` separator-trim loop evaluating `c_start[-1]` at the value start. Regression tests added to `tests/http1-parser.c`.
- (`resp3`) a negative streamed chunk length (`$?\r\n;-1\r\n`) is now a protocol error; previously the parser rewound and retried forever (infinite-loop DoS). Regression test added to `tests/resp3.c`.
- (`atol`) the `"0x"` prefix probes (`fio_atol16u`, `possible_misread` paths) no longer read `p[1]` unconditionally (short-circuit `&&`).
- (`atol`, `json`) documented the guard-byte contract: number parsing may read up to and including the first non-numeric byte following a number — callers MUST pass a NUL-terminated string or append a guard byte (`fio_bstr` always qualifies). Documented in the `atol` module and on `fio_json_parse`.
- (`atol`) new strict, bounded parsing family: `fio_stol10u`, `fio_stol10`, `fio_stol16u` — parse within an explicit `[pos, end)` window; digits only (no whitespace, underscores, `0x`/`0b` prefixes or `inf`/`nan`); `errno == E2BIG` on overflow. Use for wire protocols and any length/size field. `fio_u2i_limit` no longer flags `-9223372036854775808` (exactly `INT64_MIN`) as overflow.
- (`http1 parser`) `Content-Length` and chunk sizes now use the strict bounded parsers: `Content-Length: 1_0`, `0x` chunk-size prefixes and chunk-size separators are rejected (request-smuggling differentials, CWE-444); `Content-Length` is additionally range-checked against `size_t` (32-bit truncation desync, CWE-197).
- (`resp3`) integer parsing now delegates to the strict bounded parser via `FIO_RESP3` -> `FIO_ATOL` dependency: overflowing lengths/counts (`$99999999999999999999`) are a protocol error instead of UB/silent wrap (CWE-190); map/attribute counts that would overflow the internal `count * 2` doubling are rejected.
- (`http`) documented the `fio_http_static_file_response` root contract: `root_folder` MUST name an existing folder (settings-provided folders are validated; use `"."` for the CWD) — the traversal guard rejects `..` folding, not absolute paths; symlinks inside the root are followed.

**API Changes** (`files`): the files module is now fully zero-allocation (it is included before the memory module, so `FIO_MEM_REALLOC` / `FIO_MEM_FREE` were never safe to use). Path manipulation uses a fixed-size stack buffer (`FIO_STR_INFO_TMP_VAR`) capped by the new `FIO_FILENAME_PATH_CAPA` configuration macro (default `PATH_MAX | 4094`); over-long paths fail with `errno == ENAMETOOLONG`.

- New API: `fio_filename_remove(.path = ..., .folder = ..., .recursive = ...)` — removes a file / link (`unlink`-like, the default), an empty folder (`.folder = 1`, `rmdir`-like) or a whole tree (`.recursive = 1`, `rm -r`-like; implies `.folder`). Links (symlinks / reparse points) are removed, never followed into. Abstracts `unlink` / `rmdir` vs. `DeleteFileA` / `RemoveDirectoryA`.
- New API: `fio_filename_make_path(.path = ..., .mode = ...)` — creates a folder including any missing parents (`mkdir -p`-like). An existing folder is not an error; a non-folder component fails with `errno == ENOTDIR`. `mode` defaults to `0755` (POSIX only). Root / drive / UNC `\\server\share` prefixes are never created.
- `fio_filename_open` no longer allocates for `"~/"` expansion; over-long expansions fail with `errno == ENAMETOOLONG` (previously the limit was 64KB via allocation).

**API Changes** (`http`): the HTTP module was split into per-concern files — `430 http api.h` (public declarations), `432 http types.h` (internal types + handle/core implementation), `434 http accept.h` / `434 http1.h` / `434 sse.h` / `434 websocket.h` (protocol glue), `438 http.h` (listen/connect/protocol wiring) and `439 http.h` (cleanup tail). The standalone parsers moved from `431 http1 parser.h` / `431 websocket parser.h` to `004 http1 parser.h` / `004 websocket parser.h`.

- `FIO_HTTP_HANDLE` was removed: `FIO_HTTP` is the only HTTP module flag.
- New client API: `fio_http_websocket_connect(url, h, ...)` — a convenience wrapper around `fio_http_connect` that normalizes the URL scheme (`http://` -> `ws://`, `https://` -> `wss://`, missing scheme -> `ws://`).
- New body API: `fio_http_body_close(h)` releases the body's RAM buffer and temporary file (if any) and resets the body to empty, so body resources can be freed early once the body was consumed.
- `compress_static` is now per-route (routes inherit the listener's root value at route-creation; it is no longer copied to the handle at attach — `compress_dynamic` / `compress_ws` still are). It became a failure-memoization shift register: a compression success re-seeds bit 0 (`value |= 1`), any other failure shifts left (`value <<= 1`, so 8 consecutive failures disable on-demand creation), and `ENOSPC` / `EACCES` / `EROFS` / `EDQUOT` disables immediately (`value = 0`) until the settings are re-applied. Detached handles (no settings) gate on the `FIO_HTTP_CFLAG_COMPRESS_STATIC` handle cflag instead.
- On-demand static brotli compression uses quality 4 (benchmark-confirmed fast-path ceiling).
- (`files`) `fio_filename_overwrite` now preserves `errno` on failure.

**API Changes** (`x509`): renamed the signature/public-key algorithm identifiers in `fio_x509_cert_s` for clarity and internal consistency (both fields are `AlgorithmIdentifier`s per RFC 5280):

- Struct members: `sig_alg` -> `signature_algo`, `key_type` -> `key_algo`
- Enum types: `fio_x509_sig_alg_e` -> `fio_x509_signature_algo_e`, `fio_x509_key_type_e` -> `fio_x509_key_algo_e`
- Constants: `FIO_X509_SIG_*` -> `FIO_X509_SIGNATURE_*` (e.g., `FIO_X509_SIG_RSA_PKCS1_SHA256` -> `FIO_X509_SIGNATURE_RSA_PKCS1_SHA256`); `FIO_X509_KEY_*` unchanged
- Internal helpers: `fio___x509_parse_sig_alg` / `fio___x509_encode_sig_alg` -> `fio___x509_parse_signature_algo` / `fio___x509_encode_signature_algo`

**API Changes** (`tls13`, `x509`): unified signature-algorithm naming and buffer-view APIs across the TLS 1.3 and X.509 modules.

- **Naming**: `fio_tls13_signature_scheme_e` -> `fio_tls13_signature_algo_e`, `FIO_TLS13_SIG_*` -> `FIO_TLS13_SIGNATURE_*`, and all `sig_algs` / `signature_algorithms` / `signature_scheme` / `key_type` identifiers -> `signature_algo(s)` (matches the x509 module).
- **Struct grouping**: prefix-namespaced members of `fio_tls13_client_s` / `fio_tls13_server_s` were grouped into anonymous sub-structs (`auth`, `peer_auth`, `hrr`, `alpn`, `credentials`) plus the private `fio___tls13_cert_chain_s` type; `fio_tls13_client_hello_s` became private (`fio___tls13_client_hello_s`). Members are ordered by descending alignment (scalars before byte arrays) to avoid padding and keep scalar groups cache-line resident.
- **Buffer views**: `fio_buf_info_s` / `fio_ubuf_info_s` replace pointer+length pairs and parallel pointer/length arrays:
  - `fio_tls13_server_set_cert_chain(server, const fio_ubuf_info_s *chain, size_t count)`
  - `fio_tls13_server_set_private_key(server, fio_ubuf_info_s key, fio_tls13_signature_algo_e algo)`
  - `fio_tls13_client_set_cert(client, fio_ubuf_info_s cert, fio_ubuf_info_s private_key, fio_tls13_signature_algo_e algo)`
  - `fio_tls13_build_certificate_request(..., fio_ubuf_info_s context, ...)`
  - `fio_tls13_server_get_client_cert` now returns `fio_ubuf_info_s`
  - `fio_x509_verify_chain(const fio_ubuf_info_s *certs, size_t count, ...)`
  - Views are stored, not copied — pointed-to data must outlive the context.

### 0.8.0.rc.01 (2026-01-19)

**Breaking Change**: (`pubsub`) Complete rewrite of the Pub/Sub module with a new IPC-first architecture.

The Pub/Sub module has been completely rewritten to use the IPC module (`FIO_IPC`) for inter-process communication instead of a custom protocol. This provides better integration with the cluster system and simplifies the codebase.

**API Changes** (`pubsub`):

- **Types renamed**:
  - `fio_msg_s` -> `fio_pubsub_msg_s`
  - Message field `published` -> `timestamp`
  - Removed `is_json` field (deprecated)

- **Functions renamed**:
  - `fio_subscribe()` -> `fio_pubsub_subscribe()`
  - `fio_unsubscribe()` -> `fio_pubsub_unsubscribe()`
  - `fio_publish()` -> `fio_pubsub_publish()`

- **Engine API**:
  - `fio_pubsub_engine_ipc()` - Get IPC engine (local machine: master + workers)
  - `fio_pubsub_engine_cluster()` - Get cluster engine (all machines)
  - `fio_pubsub_engine_default()` / `fio_pubsub_engine_default_set()` - Manage default engine
  - `fio_pubsub_engine_attach()` / `fio_pubsub_engine_detach()` - Custom engine management

- **New History Manager Interface**:
  - `fio_pubsub_history_s` - Interface for message history/caching
  - `fio_pubsub_history_attach()` / `fio_pubsub_history_detach()` - Attach/detach history managers
  - `fio_pubsub_history_cache()` - Built-in in-memory cache with configurable size limit
  - Subscriptions can request message replay via `replay_since` argument

- **Removed/Deprecated**:
  - `FIO_PUBSUB_ROOT`, `FIO_PUBSUB_PROCESS`, `FIO_PUBSUB_SIBLINGS` engines
  - `fio_pubsub_ipc_url_set()`, `fio_pubsub_ipc_url()` - Use `fio_ipc_url_set()` instead
  - `fio_pubsub_broadcast_on_port()` - Use `fio_ipc_cluster_listen()` instead
  - `fio_message_metadata_*()` - Metadata API removed
  - `is_json` flag - No longer supported

**New Feature**: (`pubsub`) Cluster support via IPC broadcast. When `fio_ipc_cluster_listen()` is called before `fio_io_start()`, the default pub/sub engine automatically switches to the cluster engine for cross-machine message distribution.

**New Feature**: (`pubsub`) Message history with replay support. Late-joining subscribers can request message replay from a specified timestamp using the `replay_since` argument.

**Update**: (`ipc`) The IPC module now provides the transport layer for pub/sub messaging, enabling seamless integration between local IPC and cluster-wide RPC.

---

### 0.8.0.beta4

- The 0.8.x versions is a **total rewrite** with a **new API**, totally revised and redesigned. Why?

    1. Because facil.io has features that are useful for any C program, and I wanted to make these features more accessible and visible.

    2. Because the previous API wasn't consistent and the same words in function names didn't always have the same meaning.

    3. Because I wanted to make the code bade easier to maintain and expand.

- Major API changes!

   **Important**: Some API changes were made just to unify argument ordering across API functions. This is especially helpful where memory management is concerned, making sure all functions that are named in a similar way behave in a similar way.

- Multiple core rewrites / refinements (while, possibly, replacing old boring bugs with new and interesting one).

- Core types and common helpers have been re-written and consolidated.

   This updates the core type API in an attempt to create a more unified and intuitive API. for both core types and dynamic types (FIOBJ).

   Note that this changes some function and macro names as well as possible behavior.

   Some changes are semantic - such as renaming `fio_atomic_xchange` to `fio_atomic_exchange`.

   Some have meaningful side-effects. i.e., Hash Maps and Arrays are now limited to a theoretical capacity of `(1<<31) - 1` elements, improving memory usage for most common use-cases (if you need more than 2 billion members, you might want to roll a non-generic solution anyway).

- FIOBJ was re-written from the ground up, unifying the API used for core types and for the FIOBJ library.

    This updates makes the FIOBJ types dynamically extendible, allowing new types to be added, removed or altered during both compilation and runtime stages.

    This update simplifies some behavior (some FIOBJ algorithms are now recursive), making FIOBJ nesting slightly more dangerous (could explode the stack if abused). Nesting limits should be enforced and cyclic nesting should be avoided.

    Also, this update **should** improve type recognition performance for primitive types (`true`, `false`, `null`, numbers, floats, Arrays, Strings and Hash Maps), increasing pointer tagging usage (at the expense of the number range that optimizes away memory allocation).

- Risky Hash algorithm was updated and a Stable Hash algorithm was added.

**Security**: Fixes a number of issues with the HTTP parser that could have been leveraged in potential exploit attempts such as request smuggling. Credit to @dcepelik (David Čepelík). Later on the HTTP parser was rewritten, but with the same considerations in mind.

---

### v. 0.7.6 (2022-02-19)

**Fix**: fixed some 32 bit compatibility concerns. Credit to Franz Brausse ( @fbrausse ) for the [PR @ boazsegev/facil.io#96](https://github.com/boazsegev/facil.io/pull/96).

**Fix**: (`fiobj`) fixes an issue with type testing for `FIOBJ_T_NUMBER`. Credit goes to @alvkeke for opening issue #141.

**Compatibility**: (2021-07-14) The library is now semi-official Windows compatible. Credit to Jan Biedermann ( GitHub: @janbiedermann ) for his contributions towards making this happen as well as his efforts to review, test and fix any hiccups.

### v. 0.7.5 (2020-05-18)

**Security**: backport the 0.8.x HTTP/1.1 parser and it's security updates to the 0.7.x version branch. This fixes a request smuggling attack vector and Transfer Encoding attack vector that were exposed by Sam Sanoop from [the Snyk Security team (snyk.io)](https://snyk.io). The parser was updated to deal with these potential issues.

**Fix**: (`http`) fixes an issue with date calculation by backporting code from the 0.8.x branch.

**Fix**: (`fio`) call less signal handlers during shutdown.

### v. 0.7.4 - 2020-05-18 (skipped)

**Fix**: (`http`) fixes an issue and improves support for `chunked` encoded payloads. Credit to Ian Ker-Seymer ( @ianks ) for exposing this, writing tests  (for the Ruby wrapper) and opening both the issue boazsegev/iodine#87 and the PR boazsegev/iodine#88.

**Fix**: (`http`) requests will fail when the path contains a dangling `?` (empty query). Credit to @adam12 for exposing this and opening issue boazsegev/iodine#86.

### v. 0.7.3 - 2019-10-04

**Fix**: (`http`) fixes a security issue in the static file name resolution logic, where a maliciously encoded request could invoke an arbitrary response.

**Fix**: (`fio`, `fiobj`) improved C++ compatibility. Credit to Joey (@joeyhoek) for PR #76.

**Fix**: (`fio`) fixes an issue where timer cleanup wasn't performed after `fio_stop` (or SIGINT/SIGTERM). No a "clean slate" will be provided if `fio_start` is called more then once. Note: this may **break previous behavior**, which should be considered undocumented and unexpected behavior. (this fax **may** be deferred to version 0.8.x, still undecided). Credit to @fbrausse for opening issue #72. 

**Fix**: (`fio`) fixes an issue where timer cleanup would be performed after the `AT_EXIT` state callbacks. Now the timer cleanup callbacks will be performed **before** the `AT_EXIT` callback (as they should). (See issue #72).

**Fix**: (`fio`) fixes signal handler (re)establishment test to prevent recursive signal calling.

### v. 0.7.2 - 2019-08-29

**Fix**: (`fio_tls`) fixes a memory leak in the trusted certificate chain. Credit to @fbrausse for opening PR #71.

**Fix**: (`fio_tls`) fixes compilation / linking flags (including a bug caused by the `gcc` optimizer `-fipa-icf`) and improves support for OpenSSL using `pkg-config`. Credit to @fbrausse for PR #71.

**Fix**: (`http1`) fixes a race-condition between the `on_ready` and `on_data` events, that could result in the `on_data` event being called twice instead of once (only possible with some clients). On multi-threaded workers, this could result in the CPU spinning while the task lock remains busy. Credit to Néstor Coppi (@Shelvak) for exposing the issue and providing an example application with detailed reports. Issue #75.

### v. 0.7.1 - 2019-05-18

**Security**: a heap-overflow vulnerability was fixed in the WebSocket parser, which could have been triggered by a maliciously crafted message-header. Credit to Dane (4cad@silvertoque) for exposing this issue and providing a Python script demonstrating the attack. 

### v. 0.7.0 - 2019-03-29

Stable API release. Future API updates will be wait for the 0.8.x release.

**Fix**: (`fio`, `fiobj`) fixed some `gcc` and `clang` compatibility issues and warnings.

**Fix**: (`http`) fixed HTTP date format to force the day of the month to use two digits. Credit to @ianks (Ian Ker-Seymer) for exposing this issue (iodine#64).

**Compatibility**: (`http`) updated time-zone compile-time tests with a safer fall-back.

### v. 0.7.0.beta8 - 2019-03-03

**Security**: (`fio`) Slowloris mitigation is now part of the core library, where `FIO_SLOWLORIS_LIMIT` pending calls to `write` (currently 1,024 backlogged calls) will flag the connection as an attacker and either close the connection or ignore it. This protocol independent approach improves security.

**Security**: (`http`) HTTP/1.1 client throttling - new requests will not be consumed until pending responses were sent. Since HTTP/1.1 is a response-request protocol, this protocol specific approach should protect the HTTP application against slow clients.

**Fix**: (`fio`) fixed fallback implementation for `fio_atomic_xchange` when missing atomic primitives in compiler (older compilers). Credit to @Low-power for identifying and fixing the issue (PR #55).

**Fix**: (`fio`) fixed a possible unreleased lock when a memory allocation failed (no memory in the system). Credit to @Low-power for identifying and fixing the issue (PR #54).

**Fix**: (`fio`) fixed the `fio_sock_sendfile_from_fd` fall-back for a missing `sendfile`. Credit to @Low-power for identifying and fixing the typo (PR #49).

**Fix**: (`fio`) fixed `fio_pending` not decrementing packet count before reaching zero.

**Fix**: (`fio`) fixed logging message for overflowing log messages. Credit to @weskerfoot (Wesley Kerfoot) and @adam12 (Adam Daniels) for exposing the issue (issue iodine/#56).

**Fix**: (`fio`, `fio_risky_hash`) Florian Weber (@Florianjw) [exposed a byte ordering error (last 7 byte reading order) and took time challenge the algorithm](https://www.reddit.com/r/crypto/comments/9kk5gl/break_my_ciphercollectionpost/eekxw2f/?context=3). The exposed errors were fixed and the exposed a possible attack on RiskyHash using a variation on a Meet-In-The-Middle attack, written by Hening Makholm (@hmakholm). This prompted an update and fixes to the function.

**Fix**: (`fio`) fixed `fio_str_resize` where data might be lost if data was written beyond the current size and the requested size is larger then the String's capacity (i.e., when `fio_str_resize` is (mis)used as an alternative to `fio_str_capa_assert`).

**Fix**: (`json` / `redis`) fixed JSON formatting error caused by buffer reallocation when multiple (more then 48) escape sequences were detected. This issue also effected the Redis command callback handler (which was using JSON for IPC).

**Fix**: (`redis`) fixed a potential double `free` call.

**Fix**: (`redis`) fixed a recursive endless loop when converting nested Hash Tables to Redis objects (which normally wouldn't happen anyway, since they would be processed as JSON).

**Fix**: (`redis`) fixed Redis reconnection. Address and port data was mistakingly written at the wrong address, causing it to be overwritten by incoming (non-pub/sub) data.

**Fix**: (`redis`) fixed a race condition in the Redis reconnection logic which might have caused more then a single pub/sub connection to be established and the first pending command to be sent again.

**Fix**: (`fio`) fix capacity maximization log to accommodate issues where `getrlimit` would return a `rlim_max` that's too high for `rlim_cur` (macOS).

**Fix**: (`fio`) fix uninitialized `kqueue` message in `fio_poll_remove_fd`.

**Fix**: (`http`) possible fix for `http_connect`, where `host` header length might have been left uninitialized, resulting in possible errors.

**Fix**: (`fio`) fixed logging error message for long error messages.

**Update**: (`fio` / `makefile`) improved detection for polling system call, `sendfile`, etc'.

**Update**: (`fio`) improved signal handling. Signal handling now propagates to pre-existing signal handlers. In addition, the `fio_signal_handler_reset` function was made public, allowing facil.io signal handlers to be removed immediately following startup (using `fio_state_callback_add` with `FIO_CALL_PRE_START` to call `fio_signal_handler_reset`).

**Update**: (`fio`) improved pub/sub memory usage to minimize message copying in cluster mode (same memory is used for IPC and local-process message publishing).

**Update**: (`fio`) updated the non-cryptographic PRG algorithm for performance and speed. Now the `fio_rand` functions are modeled after the `xoroshiro128+` algorithm, with an automated re-seeding counter based on RiskyHash. This should improve performance for non cryptographic random requirements.

**Compatibility**: (`fio`) mitigate undefined MAP_ANONYMOUS on MacOS <= 10.10. Credit to @xicreative (Evan Pavlica) for iodine/PR#61.

**Compatibility**: (`fio`) various Solaris OS compatibility patches, courtesy of @Low-power (PR #52, #53).

### v. 0.7.0.beta7 - 2019-01-18

**BREAK**: (`fio_tls`) breaking API changes to the SSL/TLS API... I know, I'm sorry, especially since there's a small and misleading change in argument ordering for `fio_tls_cert_add` and `fio_tls_new`... but if we don't fix the API now, before the 0.7.0 release, bad design might ruin our Wednesday meditation for all eternity.

**BREAK**: (`http`) breaking API changes to `http_connect` were required in order to support Unix Socket connections in client mode.

**Deprecation**: (`http`) deprecating the `http_url_parse` in favor of `fio_url_parse` (moved the function to the core library and rewrote it in part).

**Security**: facil.io hash maps now limit the number of full-collisions allowed in a hash map. This mitigates the effects of hash flooding attacks. As a side effect, hash maps that are under attack might return false results for collision objects.

**Fix**: (`websocket`) fixed an issue with the WebSocket parser where network byte order for long message lengths wasn't always respected and integer bit size was wrong for larger payloads. Credit to Marouane Elmidaoui (@moxgeek) for exposing the issue.

**Fix**: (`http`) fixed `udata` in WebSocket client callback for failed WebSocket client connections.

**Fix**: (`fio`) logging message when listening to a Unix Socket.

**Fix**: (`fio`) numerous minor design fixes, such as Big-Endian string memory access, allowing `fio.h` to be used as a header only library (requires `FIO_FORCE_MALLOC`) and other adjustments.

**Fix**: (`fio`) fixed unaligned memory access in SipHash implementation and added secret randomization for each application restart.

**Fix**: (`redis`) fixed an issue where destroying the Redis engine and exiting pre-maturely, before running facio.io (`fio_start`), will cause a segmentation fault during cleanup.

**Update**: (`fio`) added Risky Hash, for fast hashing of safe data. This is a fast hashing function (about twice as fast as the default SipHash1-3 secure function) that wasn't tested for security. For this reason it should be limited to internal / safe data, such as CLI argument names.

### v. 0.7.0.beta6 - 2018-12-31

**BREAK**: (`fio_tls`) breaking API changes to the SSL/TLS API, adding support for password protected private key files. *Note*: The TLS API is still fragile and should only be considered stable once version 0.7.0 is released with SSL/TLS support.

**Security** / **Fix**: (`http`) fixed an issue with the HTTP/1.1 parser, where maliciously crafted white-space data could cause a segmentation fault, resulting in a potential DoS.

**Fix**: (`fio`) fixed an issue exposed by implementing the TLS layer, where the highet `fd` for a connection that wasn't assigned a `protocol_s` object immediately after the connection was opened, might avoid timeout review or avoid cleanup during shutdown (which will be marked as a memory leak).

**Update**: (`fio_tls`) added experimental support for OpenSSL. This was only partially tested and should be considered experimental.

**Update**: (`fio`) added, the `fio_rw_hook_replace_unsafe` to allow r/w hook switching from within a r/w hook callback.

**Update**: (`fio_cli`) a common user-error is a a missing `fio_cli_end`, resulting in a memory leak notification. Now facil.io protects against this common error by automatically calling  `fio_cli_end` during the exit stage, if `fio_cli_start` was called.

### v. 0.7.0.beta5

**Fix**: (`fio_cli`) fixed an issue introduced in version 0.7.0.beta4, where `fio_cli_get_i` would dereference NULL if the value wasn't set. Now `fio_cli_get_i` returns zero (0) for missing values, as expected. Note: this related to the new hex and binary base support in command line numerals.

### v. 0.7.0.beta4

**BREAK**: (`fio_cli`) breaking API changes make this extension easier than ever to use... *I do apologize for this, but part of the reason 0.7.0 is still in beta is to test the API itself for ease of use and readability*.

**Fix**: (`fio`) fixed a minor memory leak in cluster mode, caused by the root process not freeing the hash map used for child process subscription monitoring.

**Fix**: (`fio`) fixed superfluous and potentially erroneous pub/sub engine callback calls to `unsubscribe`, caused by (mistakingly) reporting filter channel closure.

**Fix**: (`mustache`, `FIOBJ`) added support for dot notation in mustache templates.

**Fix**: (`http/1.1`) avoid processing further requests if the connection was closed.

**Fix**: (`fio_test`) fixed some memory leaks in the testing functions.

**Update**: (`fio_cli`) stylize and beautify `FIO_CLI_PRINT_HEADER` lines.

**Update**: (`fio`) updated the automatic concurrency calculations to leave resources for the system when a negative value is provided (was only available for worker count calculations, now available for thread count as well).

### v. 0.7.0.beta3

**Fix**: (`fio`) fixed superfluous `ping` events that might occur after a `fio_write` (but before the scheduled write actually occurred).

**Fix**: (`mustache`) updated the mustache parser to fix an issue with template loading path names. The partial template path resolution logic was re-written, fixed and improved (I hope). This also adds support for text in mustache lambda, though not applicable when used with FIOBJ.

**Fix**: (`fio`) prevent Read/Write Hooks from delaying `fio_force_close` when an error occures while polling a connection.

**Fix**: (`fio`) deletes Unix sockets once done listening. Fixes an issue where the files would remain intact.

**Fix**: (`fio`) replaced `fio_malloc` existing memory allocation / free-list implementation. This also fixes an issue with large memory pools being retained on multi-core systems with many reported CPU cores.

**Fix**: (`fio`) the `FIO_FORCE_MALLOC` flag was fixed to accommodate for the fact that fio_malloc returns zeroed data (all bytes are set to zero) vs. the system's `malloc` which might return junk data.

**Fix**: (`http`) fixes a possible memory leak in `http_mimetype_register`, where clearing the registry wouldn't free the FIOBJ Strings.

**Update**: (`cli`) updated handling of empty strings in CLI values by ignoring the argument rather than printing an error or experiencing an undefined value.

**Optimization**: (`fio`) pub/sub channel names appear to be (often) objects with a long life-span. Hence, these objects now use `malloc` (instead of `fio_malloc`). Also, temporary allocations in `fio_subscribe` were removed.

**Optimization**: (`fio`) pub/sub meta-data information and callbacks now use an Array (instead of link lists and a hash map). This should improve cache locality when setting and retrieving pub/sub meta-data.

**Optimization**: (`fio`) added an urgent task queue for outbound IO, possibly improving protection against non-evented / blocking user code.

**Optimization**: (`http`) WebSocket broadcasting optimizations are now automated.

### v. 0.7.0.beta2

**Breaking Changes**!

A lot of the code was re-written and re-organized, minimizing the name space used by the core library and consolidating the core library into a two file library (`fio.h` and `fio.c`).

This translated directly to **breaking the API and ABI and bumping the version number**.

This should make the library easier to copy and use as well as minimize possible name collisions (at the price of maintaining a couple of monolithic files as the core library).

**These are the main changes**:

* Extracted the FIOBJ library from the core library, making it an add-on that could used by the extensions (such as the HTTP extension) rather than a core requirement.

* Condensed the core library and it's namespace to two files (`fio.h` and `fio.c`) - replaced all `facil_` function names with `fio_` to accommodate the new namespace.

    ...why?

    It's a choice to sacrifice ease of maintainability in favor of ease of use.

    Although two files are more difficult to maintain than 10 files (assuming it's the same amount of code)... it seems that two files are easier for other developers to copy and paste into their projects.

* Added poll support to allow for performance testing and CYGWIN compatibility. The system epoll/kqueue calls should perform better for higher loads, but now you can see for yourself.

* Timers are now in user space, allowing for more timers and less kernel dependencies.

* The `on_idle` and `on_finish` settings in `facil_run` (now `fio_start`) were removed, replaced by the more flexible `fio_state_callback_add` approach.

* The `fio_listen` and `http_listen` functions now return the listening socket's `uuid` (much like `fio_connect` and `http_connect` did).

* The Protocol's `on_shutdown` callback is now expected to return a `uint8_t`, hinting at a requested timeout before the socket is forcefully closed. A return value of 0 will indicate immediate socket closure with an 8 second timeout for outgoing buffer flushing.

* The cluster messaging system and the Pub/Sub system were both refactored, **the API changed** and the FIOBJ dependency was removed. This change cascades to effect all the Pub/Sub system elements.

* The Pub/Sub system's `use_pattern` was replaced with the optional callback argument `match` (a function pointer), allowing for custom pattern matching approaches (such as implementing NATs and RabbitMQ pattern matching). The previous glob matching approach (Redis compatible) is available using the provided `FIO_MATCH_GLOB` function pointer.

* The WebSocket upgrade (`http_upgrade2ws`) now matches the SSE upgrade function (starts with the handle `http_s *` and named arguments come later).

* The CLI API and implementation was completely rewritten. The new code is slightly more "monolithic" (one long function does most of the work), but should waste less memory with a simpler API (also got rid of some persistent data).

* The Read/Write socket hooks were redesigned.

* An SSL/TLS API stub was designed for SSL/TLS library abstraction (not implemented yet). This API is experimental and might change as I author the first SSL/TLS library wrappers (roadmap includes OpenSSL and BearSSL).

**Update**: (`fio_mem` => `fio.h`) updated the allocator defaults to lower the price of a longer life allocation. Reminder: the allocator was designed for short/medium allocation life-spans _or_ large allocations (as they directly map to `mmap`). Now 16Kb will be considered a larger allocation and the price of holding on to memory is lower (less fragmentation).

**Fix**: (`fio`) fixed a typo in the shutdown output. Credit to @bjeanes (Bo Jeanes) for the Iodine#39 PR.

**Feature**: (`FIOBJ`) added [mustache template support](http://mustache.github.io).

**Logo**: Logo created by @area55git ([Area55](https://github.com/area55git))

**Documentation**: A new website!

### v. 0.6.4

**Fix**: (`sock`) fixed an issue where calls to `sock_write` could potentially add data to the outgoing queue even after `sock_close` in cases where the outgoing queue isn't empty.

**Fix**: (`facil.io`) fixed a race condition between pre-scheduled tasks (`defer` calls) and the worker process initialization. The race condition might have resulted in some pre-scheduled tasks not running on all the workers.

**Fix**: (`http`) fixed an issue with the HTTP request logging, where the peer address wasn't shown.

**Fix**: (`websocket`) made sure that `on_ready` is never called before `on_open` returns.

**Fix**: (`fio_mem`, `facil`, `http`) fixed compatibility issues with Alpine Linux distro and older OS X versions (< 10.12).

**Fix**: (`http`) fixed the `http_date2rfc2109` method where a space wasn't written to the buffer after the month name (resulting in a junk byte).

**Fix**: (`pubsub`) made sure that newly registered engines get the full list of existing subscriptions (no need to call `pubsub_engine_resubscribe`).

**Fix**: (`facil`) possible fix for protocol attachment with `NULL` protocol.


### v. 0.7.0.beta1

**(yanked)**

### v. 0.6.3

**Fix**: (`fio_hashmap` / `fiobj_hash`) fixed a possible issue that could occur when compacting a severely fragmented Hash (where the length of the new storage requirements is shorter than the fragmented ordered array of data).

**Fix**: (`http` / `websocket`) fixed an issue where the WebSocket's `on_close` callback wouldn't be called if certain errors prevented the upgrade. Now the `on_close` callback is always called.

**Fix**: (`http`) fixed an issue where the Content-Type header might be missing when sending unrecognized files. Now an additional best attempt to detect the content type (this time using the URL instead of the file name) will be performed. If no content type is detected, the default RFC content type will be attached (`application/octet-stream`).

**Fix**: (`http1_parser`) fixed a possible unaligned memory access concern.

**Fix**: (`FIOBJ`) fixed compiler compatibility concerns with the `fiobj_num_new` logic, removing some possibly undefined behavior.

**Fix**: (`facil`) a missing `on_data` protocol callback (missing during `facil_attach`) will now call `facil_quite`, preventing the event from firing endlessly.

**Update**: (`http`) the `on_upgrade` callback now supports SSE connections with `sse` protocol identifier and the `http_upgrade2sse` function.

**Update**: (`sock`) initial support for TCP Fast Open (TFO) when listening for connections.

### v. 0.6.2

This version fixes a number of issues, including a serious issue that prevented sockets from fully flushing their buffer.

This version also improved the shutdown and hot restart logic and fixes numerous issues with cluster mode an pub/sub services.

It's recommended that all 0.6.0.beta, 0.6.0 and 0.6.1 upgrade to this version.

**Security**: (`http1`) added a hard-coded limit on the number of headers allowed per request (regardless of size). `HTTP_MAX_HEADER_COUNT` defaults to 128, which should be enough by all accounts.

**Fix**: (`pubsub`, `facil`, `redis-engine`) fixed numerous cluster and Pub/Sub issues, including support for new `on_startup` callback for `pubsub_engine_s` objects (to make handling `fork`s that much easier. This fixes a memory leak, a reference counting error that freed memory prematurely, message parsing errors on fragmented messages, an obsolete ping formatting error, and more.

**Fix**: (`sock`, `facil`) fixed an issue where socket buffers wouldn't be completely cleared (the `on_ready` event wouldn't be properly re-armed). This was discovered as a serious issue and upgrading to 0.6.2 is recommended.

**Fix**: (`http`) fixed an HTTP status string output error, where status codes above 410 would degrade to status 500 (internal error) instead of printing the correct status string to the response.

**Fix**: (`websockets`) fixed a missing "close" packet signal that should have been sent immediately after the user's `on_shutdown` callback.

**Fix**: (`FIOBJ`) fixed the `fiobj_str_tmp` function to add thread safety (temp string should be stored in the thread's local storage, not globally accessible).

**Fix**: (`redis`) fixed a race condition in the Redis engine that could prevent publishing connections from being established in worker processes.

**Fix**: (`facil`) fixed an issue where `facil_attach` wouldn't call the `on_close` callback if the failure was due to the file descriptor being equal to -1.

**Fix**: (`facil`) fixed a signaling issue where a `SIGUSR1` sent to a worker process might inadvertently shutdown the server instead or wind down the specific worker and re-spawn a new one.

**Fix**: (`facil`) fixed a signal handling logic to make it async-safe, protecting it against possible deadlocks or cluster stream corruption.

**Update/Fix**: (`facil`) the `on_data` event will no longer be fired for sockets that were flagged to be closed using `sock_close`.

**Update**: (`FIOBJ`) updated the `fiobj_str_readfile` to allow for a negative `stat_at` position (calculated from the end of file of the file backwards).

**Update**: (`facil`) strengthened the `on_shutdown` callback lock to prevent the `on_shutdown` callback from being called while the `on_data` callback (or other connection tasks) is running.

**Update**: (`facil`) shutdown logic provides more time for socket buffers to flush (only when required).

### v. 0.6.1

**Fix**: (`pubsub`) fixed a possible issue where a channel name might be freed before it's callback is handled. This was overlooked during the Hash caching logic update that prevented key hashing when the last item of the ordered Hash is removed.

**Fix**: (`pubsub`) pubsub will now compact the memory used for client and channel data if the storage becomes excessively fragmented.

**Fix**: (`hashmap`) fixed a possible memory reading issue that could occur when a Hash contains only one object and that object is removed (causing a memory read into the location just before the Hash map's address).

**Fix**: (`defer`) defer now prefers the statically allocated buffer over the dynamically allocated buffer when all tasks have completed, preventing the last allocated buffer from leaking during the shutdown stage.

**Fix**: (`websocket`) subscriptions created during the on_close callback (besides indicating that the API was being abused) are now properly removed.

### v. 0.6.0

Version 0.6.0 is a major release, changing much of the extension API (HTTP, pub/sub, CLI) and some of the core API (i.e., moving the evio polling from level-triggered to one-shot polling, a rewrite to the facil.io dynamic object types FIOBJ, and more).

The following updates are included in this release (in addition to the beta updates):

**Fix**: (`pubsub`) Fixed an issue where deferred pub/sub messages would have `udata2` set to `udata1` instead of the actual correct value.

**Fix**: (`facil`) Fixed the `facil_is_running()` function, which could crash if facil.io wasn't initialized before the function was called.

**Fix**: (`facil`) Fix CPU limit detection. Negative values are now meaningful (fraction of CPU cores, so -2 == cores/2). Zero values are replaced by facil.io.

**Update**: (`facil`) Hot restart is now available for cluster mode. By sending the `SIGUSR1` signal to the program, facil.io will shutdown any worker processes and re-spawn new workers, allowing for a hot restart feature. Disable using `FACIL_DISABLE_HOT_RESTART`

**Update**: (`facil`) Dedicated system mode can be toggled by setting the `FIO_DEDICATED_SYSTEM` macro during compile time. When `FIO_DEDICATED_SYSTEM` is set, facil.io will assume all the CPU cores are available and it will activate threads sooner. When `FIO_DEDICATED_SYSTEM` is defined as 0, facil.io will limit thread to protect against slow user code (rather than attempt concurrent IO).

**Update**: (`fio_mem`) replaced the double linked list logic with a single linked list to make the library more independent as well as reduce some operations.

As well as some refactoring and minor adjustments.

### v. 0.6.0.beta.8

**Fix**: (`defer`) the `defer_free_thread` symbol is now correctly marked as weak, allowing the function to be overridden.

**Fix**: (`http`) fixes an issue where cookies without an explicit age would be marked for immediate deletion (instead of the expected "session" lifetime).

**Fix**: (`http`) fixes a potential issue where a missing or corrupt `accept-encoding` header could cause a segmentation fault.

**Fix**: (`http`) fixes an issue where a cookie encoding errors would reported repeatedly.

**Fix**: (`fio_hash`) fixes an issue where resubmitting a removed object wouldn't increase the object count.

**Fix**: (`fiobj`) fixes an issue where testing the type of specific FIOBJ_T_NUMBER objects using `FIOBJ_TYPE_IS` would return a false positive for the types FIOBJ_T_HASH or FIOBJ_T_STRING.

**Update**: Added an experimental custom memory allocator (`fio_mem.h`) optimized for small concurrent short-lived allocations (anything over 16Kb and reallocations start to take a toll). It can replace the system's `malloc` function family when `FIO_OVERRIDE_MALLOC` is defined. To use `tcmalloc` or `jemalloc`, define `FIO_FORCE_MALLOC` to prevent `fio_mem` from compiling.

**Update**: (`http`) added cookie parsing.

**Update**: minor optimizations, `fio_malloc` incorporation and documentation updates.

### v. 0.6.0.beta.7

**Fix**: (`websockets`) fixed an issue with client pinging would break the protocol in a way that would result in either loss of data or disconnections.

**Fix**: (`websockets`) removed the debugging ping (3 seconds interval ping) from the Websocket client. Pings can be sent manually or by setting the connection's timeout using `facil_set_timeout`.

**Fix**: (`websockets`) made sure the client mask is never zero by setting a few non-random bits.

**Fix**: (`redis`) fixed an issue where the command queue (for busy pipelined Redis commands and for reconnection) would send the last message repeatedly instead of sending the messages in the queue.

**Fix**: (`facil`) Fixed a possible memory leak related to `facil_connect` and failed connections to localhost. Improved process exit cleanup.

**Fix**: (`pubsub`) improved process exit cleanup.

**Fix**: (`fio_cli`) fixed text coloring on terminal output.

**Fix**: (`fio_hash`) fixed looping logic to remove the need for the "data-end" marker optimizing allocations in a very small way.

**Update**: (`websockets`) added a client example using the terminal IO for Websocket text communication.

### v. 0.6.0.beta.6

This beta release is a performance oriented release and includes mostly performance related changes.

This release updates some default values to make them more reasonable for common use cases and to help minimize memory consumption.

These values, such as the `LIB_SOCK_MAX_CAPACITY`, `FIO_HASH_INITIAL_CAPACITY` and the `FIOBJ_ARRAY_DEFAULT_CAPA` values, can be updated during compile time.

Some of these default values can be bypassed during runtime by using specific function calls (such as `fio_hash_new2`).

Other notable performance changes include the short string hash cashing (shortening the FIOBJ short-string capacity in exchange for reducing `fio_siphash` calls).

These are lessons learned from the TechEmpower benchmarks... although they will not be reflected in the Round 15 results.

### v. 0.6.0.beta.5

Released fixes for issues related to the [TechEmpower Framework Benchmarks](https://github.com/TechEmpower/FrameworkBenchmarks) 80 core startup.

**Fix**: fixed error handling during cluster mode startup, making sure facil.io fails to start.

**Update**: capped maximum core detection value to 120 cores. Any value larger than 120 will raise a warning and the cap (120) will be used.

### v. 0.6.0.beta.4

Released after stress testing and memory leakage testing.

### v. 0.6.0.beta.3

**Breaking Change**: (`websockets`) the websocket `on_close` callback signature had changed to allow it to be called on connection/upgrade failures as well (easier `udata` cleanup).

**Fix** (`facil`): fixes an issue introduced in the beta.2 version, where deferred events that where scheduled before a call to `facil_run` would only be called for the parent process. Now these events would perform as intended (once in the root process and once in each worker process).

**Fix** (`facil`): updates the logical assumption about open connections, to make sure any open connections are closed when re-spawning a child worker. This shift the connection assumption from unsafe (forked connections should be closed by extensions) to safe (reconnection should be handled by extension). This change should effect internal extensions only, since active connections aren't handled by the root process in clustered mode.

**Change** (`websocket`): the protocol is now more forgiving in cases where clients don't mask empty messages.

**Feature** (`websockets`): A simple and easy Websocket client using `websocket_connect` as well as support for more complex clients (with authentication logic etc') using a combination of the `http_connect` and `http_upgrade2ws` functions.

**Minor**: some changes to the inner HTTP logic, fixed some error messages, and other minor updates.

### v. 0.6.0.beta.2

Version 0.6.0 is a major release, changing much of the extension API (HTTP, pub/sub, CLI) and some of the core API (i.e., moving the `evio` polling to One-Shot polling).

In this beta 2 release:

**Fix** (`facil`): fixes an issue that could occur when forking a large number of processes, where cluster connection locks would remain locked, causing the cluster connections to spin the CPU and prevent shutdown.

**Fix** (`redis`, `pubsub`, `evio`): fixes for the internal Redis engine. There was a connection re-establishing error related to updates in the new `evio` event logic.

**Update**: (`http`) Added experimental query parsing helpers that perform nested parameter name resolution (i.e. `named_hash[named_array][]=value`). Logic might change as performance considerations apply. I'd love to read your feedback on this feature.

**Update**: (`facil`) Simplified the child worker sentinel observation logic, to use threads instead of IPC.

### v. 0.6.0.beta

Version 0.6.0 is a major release, changing much of the extension API (HTTP, pub/sub, CLI) and some of the core API (i.e., moving the `evio` polling to One-Shot polling).

In this beta 1 release:

**Fix** (`http`): fixed an issue where receiving the same header name more than once would fail to convert the header value into an array of values.

**Minor fixes**: more error handling, more tests, fixed `fiobj_iseq` to test hash keys as well as objects. The `fio_hashmap.h` key caching for removed objects is cleared when hash is empty (i.e, if it's empty, it's really empty).

**Performance** minor improvements. For example, Header Hash Maps are now cleared and reused by HTTP/1.1 during keep-alive (instead of deallocated and reallocated).

### v. 0.6.0.dev

This is a major release, changing much of the extension API (HTTP, pub/sub, CLI) and some of the core API (i.e., moving the `evio` polling to One-Shot polling).

Migration isn't difficult, but is not transparent either.

**Fix** (backported): (`websocket_parser`) The websocket parser had a memory offset and alignment handling issue in it's unmasking (XOR) logic and the new memory alignment protection code. The issue would impact the parser in rare occasions when multiple messages where pipelined in the internal buffer and their length produced an odd alignment (the issue would occur with very fast clients, or a very stressed server).

**Note About Fixes**:

-  I simply rewrote much of the code to know if the issues I fixed were present in the 0.5.x version or not.

   I believe some things work better. Some of the locking concerns will have less contention and I think I fixed some issues in the `fiobj` core types as well as the `http` extension.

   However, experience tells me a new major release (0.6.0) is always more fragile than a patch release. I did my best to test the new code, but experience tells me my tests are often as buggy as the code they test.

   Anyway, I'm releasing 0.6.0 knowing it works better than the 0.5.8 version, but also knowing it wasn't battle tested just yet.

**Changes!**: (`fiobj` / facil.io objects):

- Major API changes.

    The facil.io dynamic type library moved closer to facil.io's core, integrating itself into the HTTP request/response handling, the Pub/Sub engine, the Websocket internal buffer and practically every aspect of the library.

    This required some simplification of the `fiobj` and making sure future changes would require less of a migration process.

- The Symbol and Couplet types were removed, along with nesting protection support (which nobody seemed to use anyway).

- We're back to static typing with `enum`, using macros and inline functions for type identification (better performance at the expense of making extendability somewhat harder).

- Hashes are now 100% collision resistant and have improved memory locality, at the expense of using more memory and performing calling `memcmp` (this can be avoided when seeking / removing / deleting items, but not when overwriting items).

**Changes!**: (`http`):

- The HTTP API and engine was completely re-written (except the HTTP/1.1 parser), both to support future client mode (including chunked encoding for trailing headers) and to make routing and request parsing easier.

- The updates to the HTTP API might result in decreased performance during the HTTP request reading due to the need to allocate resources and possibly copy some of the data into dynamic storage... For example, header Hash Tables replaced header Arrays, improving lookup times and increasing creation time. 

- The HTTP parser now breaks down long URI schemes into a short URI + `host` header (which might become an array if it's included anyway).

**Changes!**: (`websocket`):

- The Websocket API includes numerous breaking changes, not least is the pub/sub API rewrite that now leverages `FIOBJ` Strings / Symbols.

- `websocket_write_each` was deprecated (favoring a pub/sub only design).

**Changes!**: (`pubsub`):

- The `pubsub` API was redesigned after re-evaluating the function of a pub/sub engine and in order to take advantage of the `FIOBJ` type system.

- Channel names now use a hash map with collision protection (using `memcmp` to compare channel names). The means that the 4 but trie is no longer in use and will be deprecated.

**Changes!**: (`redis`):

- The `redis_engine` was rewritten, along with the RESP parser, to reflect the changes in the new `pubsub` service and to remove obsolete code.

**Changes!**: (`facil`):

- Slight API changes:

    - `facil_last_tick` now returns `struct timespec` instead of `time_t`, allowing for more accurate time stamping.

    - `facil_cluster_send` and `facil_cluster_set_handler` were redesigned to reflect the new cluster engine (now using Unix Sockets instead of pipes).

- Internal updates to accommodate changes to other libraries.

- Cluster mode now behaves as sentinel, re-spawning any crashed worker processes (except in DEBUG mode).

**Changes!**: (`evio`):

- the evented IO library was redesigned for **one-shot** notifications, requiring a call to `evio_add` or `evio_set_timer` in order to receive future notifications.


    This was a significant change in behavior and the changes to the API (causing backwards incompatibility) were intentional.

- the code was refactored to separate system specific logic into different files. making it easier to support more systems in the future.

**Changes!**: (`sock`):

- the socket library now supports automatic Unix detection (set `port` to NULL and provide a valid Unix socket path in the `address` field).

- the socket library's Read/Write hooks API was revised, separating the function pointers from the user data. At server loads over 25%, this decreases the memory footprint.

- the socket library's packet buffer API was deprecated and all `sock_write2(...)` operations take ownership of the memory/file (enforce `move`).

- The internal engine was updated, removing pre-allocated packet buffers altogether and enabling packet header allocation using `malloc`, which introduces a significant changes to the internal behavior, possibly effecting embedded systems.

**Changes!**: (`defer`):

- Removed forking from the defer library, moving the `fork` logic into the main `facil` source code.

- Defer thread pools and now include two weak functions that allow for customized thread scheduling (wakeup/wait). These are overwritten by facil.io (in `facil.c`).

    By default, defer will use `nanosleep`.

**Refactoring**: (`fiobj`) moved the underlying Dynamic Array and Hash Table logic into single file libraries that support `void *` pointers, allowing the same logic to be used for any C object collection (as well as the facil.io objects).

---

### Ver. 0.5.10 (backported)

**Fix** (backported): (`facil` / `pubsub`) fixed an issue where cluster messages would be corrupted when passed in high succession. Credit to Dmitry Davydov (@haukot) for exposing this issue through the Iodine server implementation (the Ruby port).

It should be noted that this doesn't fix the core weakness related to large cluster or pub/sub messages, which is caused by a design weakness in the `pipe` implementation (in some kernels).

The only solution for large message corruption is to use the new pub/sub engine introduced in the facil.io 0.6.x release branch, which utilizes Unix Sockets instead of pipes.

### Ver. 0.5.9 (backported)

**Fix** (backported from 0.6.0): (`websocket_parser`) The websocket parser had a memory offset and alignment handling issue in it's unmasking (XOR) logic and the new memory alignment protection code. The issue would impact the parser in rare occasions when multiple messages where pipelined in the internal buffer and their length produced an odd alignment (the issue would occur with very fast clients, or a very stressed server).

### Ver. 0.5.8

**Fix**: (`defer`, `fiobj`) fix Linux compatibility concerns (when using GCC). Credit goes to @kotocom for opening issue #23.

### Ver. 0.5.7

**Fix**: (`defer`) fixes the non-debug version of the new (v.0.5.6) defer, which didn't define some debug macros.

**Updates**: minor updates to the boilerplate documentation and the "new application" creation process.

### Ver. 0.5.6 (yanked)

**Fix**: Added `cmake_minimum_required` and related CMake fixes to the CMake file and generator. Credit to David Morán (@david-moran) for [PR #22](https://github.com/boazsegev/facil.io/pull/22) fixing the CMakelist.txt.

**Compatibility**: (`websocket_parser`) removed unaligned memory access from the XOR logic in the parser, making it more compatible with older CPU systems that don't support unaligned memory access or 64 bit word lengths.

**Optimization**: (`defer`) rewrote the data structure to use a hybrid cyclic buffer and linked list for the task queue (instead of a simple linked list), optimizing locality and minimizing memory allocations.

**Misc**: minor updates and tweaks, such as adding the `fiobj_ary2prt` function for operations such as quick sort, updating some documentation etc'.

### Ver. 0.5.5

**Fix**: (`fiobj`) fixed an issue #21, where `gcc` would complain about overwriting the `fio_cstr_s` struct due to `const` members. Credit to @vit1251 for exposing this issue.

**Fix**: (`fiobj`) fixed NULL pointer testing for `fiobj_free(NULL)`.

**Compatibility**: (`gcc-6`) Fix some compatibility concerns with `gcc` version 6, as well as some warnings that were exposed when testing with `gcc`.

**Optimization**: (`fiobj`) optimized the JSON parsing memory allocations as well as fixed some of the function declarations to add the `const` keyword where relevant.

### Ver. 0.5.4

I've been making so many changes, it took me a while.

This version includes all the updates in the unreleased version 0.5.3 as well as some extra work.

The new HTTP/1.1 and Websocket parsers have been debugged and tested in the Iodine Ruby server, the dynamic type system (`fiobj_s`) will be updated to revision 2 in this release...

**Change/Update**: (`fiobj`) The dynamic type library was redesigned to make it extendable. This means that code that used type testing using a `switch` statement needs to be rewritten.

**Fix**: (`websocket`) issues with the new websocket parser were fixed. Credit to Tom Lahti (@uidzip) for exposing the issues.

### Ver. 0.5.3 (unreleased, included in 0.5.4)

**Change**: minor changes to the versioning scheme removed some version MACROS... this isn't API related, so I don't consider it a breaking change, but it might effect code that relied too much on internal workings. The only valid version macros are the `FACIL_VERSION_*` macros, in the `facil.h` header.

**Change**: (`http`) the HTTP/1.x parser was re-written and replaced. It should perform the same, while being easier to maintain. Also, the new parser could potentially be used to author an HTTP client.

**Change**: (`websocket`) the Websocket parser was re-written and replaced, decoupling the parser and message wrapper from the IO layer. Performance might slightly improve, but should mostly remain the same. The new code is easier to maintain and easier to port to other implementations. Also, the new parser supports a client mode (message masking).

**Fix**: (`websocket`) fix #16, where a client's first message could have been lost due to long `on_open` processing times. This was fixed by fragmenting the `upgrade` event into two events, adding the `facil_attach_locked` feature and attaching the new protocol before sending the response. Credit to @madsheep and @nilclass for exposing the issue and tracking it down to the `on_open` callbacks.

**Fix**: (`sock`) sockets created using the TCP/IP `sock` library now use `TCP_NODELAY` as the new default. This shouldn't be considered a breaking change as much as it should be considered a fix.

**Fix**: (`http1`) HTTP/1.x is now more fragmented, avoiding a `read` loop to allow for mid-stream / mid-processing protocol upgrades. This also fixes #16 at it's root (besides the improved `websocket` callback handling).

**Fix**: (`http1`) HTTP/1.x now correctly initializes the `udata` pointer to NULL fore each new request.

**Fix**: (`facil`) the `facil_run_every` function now correctly calls the `on_finish` callback when a timer initialization fails. This fixes a leak that could have occurred due to inconsistent API expectations. Workarounds written due to this issue should be removed.

**Fix**: (`facil`) connection timeout is now correctly ignored for timers.

**Fix**: (`defer`) a shutdown issue in `defer_perform_in_fork` was detected by @cdkrot and his fix was implemented.

**Fix**: (`evio`) fixed an issue where the evented IO library failed to reset the state indicator after `evio_close` was called, causing some functions to believe that events are still processed. Now the `evio_isactive` will correctly indicate that the evented IO is inactive after `evio_close` was called.

**Fix**: (`evio`) fixes an issue where `evio_add_timer` would fail with `EEXIST` instead of reporting success (this might be related to timer consolidation concerns in the Linux kernel).

**Fix**: (`evio`) better timer `fd` creation compatibility with different Linux kernels.

**Fix**: (documentation) credit to @cdkrot for reporting an outdated demo in the README.

**Fix**: (linking) added the missing `-lm` linker flag for `gcc`/Linux (I was using `clang`, which automatically links to the math library, so I didn't notice this).

**Portability**: added `extern "C"` directive for untested C++ support.

**Feature**: 🎉 added a CLI helper service, allowing easy parsing and publishing of possible command line arguments.

**Feature**: 🎉🎉 added a dynamic type library to `facil.io`'s core, making some common web related tasks easier to manage.

**Feature**: 🎉🎉🎉 added native JSON support. JSON strings can be converted to `fiobj_s *` objects and `fiobj_s *` objects can be rendered as JSON! I'm hoping to get it [benchmarked publicly](https://github.com/miloyip/nativejson-benchmark/pull/92).

### Ver. 0.5.2

**Change**: non-breaking changes to the folder structure are also reflected in the updated `makefile` and `.clang_complete`.

**Fix**: (`defer`) fixed `SIGTERM` handling (signal was mistakingly filtered away).

**Fix**: (`http_response`) fixed `http_response_sendfile2` where path concatenation occurred without a folder separator (`/`) and exclusively safe file paths were being ignored (the function assumed an unsafe path to be used, at least in part).

**Fix**: minor fixes and documentation.

**Fix / Feature**: (`facil`) sibling processes will now detect a sibling's death (caused by a crashed process) and shutdown.

**Feature**: @benjcal suggested the script used to create new applications. The current version is a stand-in draft used for testing.

**Feature**: temporary boiler plate code for a simple "hello world" HTTP application using the new application script (see the README). This is a temporary design to allow us to test the script's functionality and decide on the final boiler plate's design.

### Ver. 0.5.1

**Fix**: (`sock`) Fixed an issue where `sock_flush` would always invoke `sock_touch`, even if no data was actually sent on the wire.

**Fix**: (`sock`) fixed a possible issue with `sock_flush_strong` which might have caused the facil.io to hang.

**Fix**: (`Websocket`) fixed an issue with fragmented pipelined Websocket messages.

**Feature**: (`facil`) easily force an IO event, even if it did not occur, using `facil_force_event`.

### Ver. 0.5.0

**Braking changes**: (`pubsub`) The API was changed / updated, making `pubsub_engine_s` objects easier to author and allowing allocations to be avoided by utilizing two `void * udata` fields... Since this is a breaking change, and following semantic versioning, the minor version is updated. I do wish I could have delayed the version bump, as the roadmap ahead is long, but it is what it is.

**Braking changes**: (`facil`) Since the API is already changing a bit, I thought I'd clean it up a bit and have all the `on_X` flow events (`on_close`, `on_fail`, `on_start`...) share the same function signature where possible.

**Changes**: (`facil`) Minor changes to the `fio_cluster_*` API now use signed message types. All negative `msg_type` values are reserved for internal use.

**Fix**: plugging memory leaks while testing the system under very high stress.

**Fix**: (`pubsub`, `fio_dict`) Fixed glob pattern matching... I hope. It seems to work fine, but I'm not sure it the algorithm matches the Redis implementation which is the de-facto standard for channel pattern matching.

**Security**: (`http`) the HTTP parser now breaks pipelined HTTP requests into fragmented events, preventing an attacker from monopolizing requests through endless pipelining of requests that have a long processing time.

**Fix**: (`http`) `http_listen` will now always *copy* the string for the `public_folder`, allowing dynamic strings to be safely used.

**Fix**: (`http`) default error files weren't located due to missing `/` in name. Fixed by adjusting the requirement for the `/` to be more adaptive.

**Fix**: (`http`) dates were off by 1 day. Now fixed.

**Fix**: (`http1`) a minor issue in the `on_data` callback could have caused the parser to crash in rare cases of fragmented pipelined requests on slower connections. This is now fixed.

**Fix**?: (`http`) When decoding the path or a request, the `+` sign is now left unencoded (correct behavior), trusting in better clients in this great jungle.

**Fix**: (`facil`) `facil_defer` would leak memory if a connection was disconnected while a task was scheduled.

**Fix**: (`facil`) `facil_connect` now correctly calls the `on_fail` callback even on immediate failures (i.e. when the function call was missing a target address and port).

**Fix**: (`facil`) `facil_connect` can now be called before other socket events (protected form library initialization conflicts).

**Fix**: (`facil`) `facil_listen` will now always *copy* the string for the `port`, allowing dynamic strings to be safely used when `FACIL_PRINT_STATE` is set.

**Fix**: (`facil`) `facil_last_tick` would crash if called before the library was initialized during socket operations (`facil_listen`, `facil_attach`, etc')...  now `facil_last_tick` falls back to `time()` if nothing happened yet.

**Fix**: (`facil`) `.on_idle` now correctly checks for non networks events as well before the callback is called.

**Fix**: (`defer`) A large enough (or fast enough) thread pool in a forked process would complete the existing tasks before the active flag was set, causing the facil.io reactor to be stranded in an unscheduled mode, as if expecting to exit. This is now fixed by setting a temporary flag pointer for the forked children, preventing a premature task cleanup.

**Changes**: Major folder structure updates make development and support for CMake submodules easier. These changes should also make it easier to push PRs for by offering the `dev` folder for any localized testing prior to submitting the PR.

**Feature**: (`websockets`) The websocket pub/sub support is here - supporting protocol tasks as well as direct client publishsing (and autu text/binary detection)! There are limits and high memory costs related to channel names, since `pubsub` uses a trie for absolute channel matching (i.e. channel name length should be short, definitely less than 1024Bytes).

**Feature**: (`redis`) The websocket pub/sub support features a shiny new Redis engine to synchronize pub/sub across machines! ... I tested it as much as I could, but I know my tests are as buggy as my code, so please test before using.

**Feature**: (`facil_listen`, `http_listen`) supports an optional `on_finish_rw` callback to clean-up the `rw_udata` object.

**Feature**: (`pubsub`) channels now use the available `fio_dict_s` (trie) data store. The potential price of the larger data-structure is elevated by it's absolute protection against hash collisions. Also, I hope that since channels are more often searched than created, this should improve performance when searching for channels by both pattern and perfect match. I hope this combination of hash tables (for client lookup) and tries (for channel traversal) will provide the best balance between string matching, pattern matching, iterations and subscription management.

**Feature**: (`http`) `http_listen` now supports an `on_finish` callback.

**Feature**: (`http1`) HTTP/1.1 will, in some cases, search for available error files (i.e. "400.html") in the `public_folder` root, allowing for custom error messages.

**Feature**: CMake inclusion. Credit to @OwenDelahoy (PR#8).

> To use facil.io in a CMake build you may add it as a submodule to the project's repository.
>
>       git submodule add https://github.com/boazsegev/facil.io.git

> Then add the following line the project's `CMakeLists.txt`
>
>       add_subdirectory(facil.io)

**Optimize**: (`fio_hash_table`) optimize `fio_ht_s` bin memory allocations.

### Ver. 0.4.4

**Fix**: (`pubsub`) Fixed collisions between equal channel names on different engines, so that channels are only considered equal if they share the same name as well as the same engine (rather than only the same name)... actually, if they share the same channel name SipHash value XOR'd with the engine's memory location.

**Fix**: (`facil`) Fixed compiling error on older `gcc` v.4.8.4, discovered on Ubuntu trusty/64.

**Fix**: Fix enhanced CPU cycles introduced in the v.0.4.3 update. Now CPU cycles are lower and thread throttling handles empty queues more effectively.

**Performance**: (`pubsub`) now uses a hash-table storage for the channels, clients and patterns, making the duplicate review in `pubsub_subscribe` much faster, as well as improving large channel collection performance (nothing I can do about pattern pub/sub, though, as they still need to be matched using iterations, channel by channel and for every channel match).

**Feature**: (`http`) The `http_response_sendfile2` function will now test for a `gzip` encoded alternative when the client indicated support for the encoding. To provide a `gzip` alternative file, simply `gzip` the original file and place the `.gz` file in the original file's location.

**Folder Structure**: Updated the folder structure to reflect source code relation to the library. `core` being required files, `http` relating to `http` etc'.

### Ver. 0.4.3

**Fix**: Some killer error handling should now signal all the process group to exit.

**Fix**: (`sock`, `websocket`) `sock_buffer_send` wouldn't automatically schedule a socket buffer flush. This caused some websocket messages to stay in the unsent buffer until a new event would push them along. Now flushing is scheduled and messages are send immediately, regardless of size.

**Fix**: (`facil`) `facil_attach` now correctly calls the `on_close` callback in case of error.

**Fix**: (`facil`) `facil_protocol_try_lock` would return false errors, preventing external access to the internal protocol data... this is now fixed.

**Feature**: (`facil`) Experimental cluster mode messaging, allowing messages to be sent to all the cluster workers. A classic use-case would be a localized pub/sub websocket service that doesn't require a backend database for syncing a single machine... Oh wait, we've added that one too...

**Feature**: (`facil`) Experimental cluster wide pub/sub API with expendable engine support (i.e., I plan to add Redis as a possible engine for websocket pub/sub).

**Update**: (`http`) Updated the `http_listen` to accept the new `sock_rw_hook_set` and `rw_udata` options.

**Update**: (`sock`) Rewrote some of the error handling code. Will it change anything? only if there were issues I didn't know about. It mostly effects errno value availability, I think.

### Ver. 0.4.2

**Fix**: (`sock`) Fixed an issue with the `sendfile` implementation on macOS and BSD, where medium to large files wouldn't be sent correctly.

**Fix**: (`sock`) Fixed the `sock_rw_hook_set` implementation (would lock the wrong `fd`).

**Design**: (`facil`) Separated the Read/Write hooks from the protocol's `on_open` callback by adding a `set_rw_hook` callback, allowing the same protocol to be used either with or without Read/Write hooks (i.e., both HTTP and HTTPS can share the same `on_open` function).

**Fix**: (`evio`, `facil`) Closes the `evio` once facil.io finished running, presumably allowing facil.io to be reinitialized and run again.

**Fix**: (`defer`) return an error if `defer_perform_in_fork` is called from within a running defer-forked process.

**Fix**: (`sock`, `facil`, bscrypt) Add missing `static` keywords.

**Compatibility**: (bscrypt) Add an alternative `HAS_UNIX_FEATURES` test that fits older \*nix compilers.

---

### Ver. 0.4.1

**Fix**: (HTTP/1.1) fixed the default response `date` (should have been "now", but was initialized to 0 instead).

**Fix**: fixed thread throttling for better energy conservation.

**Fix**: fixed stream response logging.

**Compatibility**: (HTTP/1.1) Automatic `should_close` now checks for HTTP/1.0 clients to determine connection persistence.

**Compatibility**: (HTTP/1.1) Added spaces after header names, since some parsers don't seem to read the RFC.

**Fix/Compatibility**: compiling under Linux had been improved.

---

### Ver. 0.4.0

Updated core and design. New API. Minor possible fixes for HTTP pipelining and parsing.

# Historic Change log

The following is a historic change log, from before the `facil_` API.

---

Note: This change log is incomplete. I started it quite late, as interest in the libraries started to warrant better documentation of any changes made.

Although the libraries in this repo are designed to work together, they are also designed to work separately. Hence, the change logs for each library are managed separately. Here are the different libraries and changes:

* [Lib-React (`libreact`)](#lib-react) - The Reactor library.

* [Lib-Sock (`libsock`)](#lib-sock) - The socket helper library (development incomplete).

* [Lib-Async (`libasync`)](#lib-async) - The thread pool and tasking library.

* [Lib-Server (`libserver`)](#lib-server) - The server writing library.

* [MiniCrypt (`minicrypt`)](#minicrypt) - Common simple crypto library (development incomplete).

* [HTTP Protocol (`http`)](#http_protocol) - including request and response helpers, etc'.

* [Websocket extension (`websockets`)](#websocket_extension) - Websocket Protocol for the basic HTTP implementation provided.

## General notes and _future_ plans

Changes I plan to make in future versions:

* Implement a `Server.connect` for client connections and a Websocket client implementation.

* Implement Websocket writing using `libsock` packets instead of `malloc`.

* Remove / fix server task container pooling (`FDTask` and `GroupTask` pools).

## A note about version numbers

I attempt to follow semantic versioning, except that the libraries are still under pre-release development, so version numbers get updated only when a significant change occurs or API breaks.

Libraries with versions less then 0.1.0 have missing features (i.e. `mini-crypt` is missing almost everything except what little published functions it offers).

Minor bug fixes, implementation optimizations etc' might not prompt a change in version numbers (not even the really minor ones).

API breaking changes always cause version bumps (could be a tiny version bump for tiny API changes).

Git commits aren't automatically tested yet and they might introduce new issues or break existing code (I use Git also for backup purposes)...

... In other words, since these libraries are still in early development, test before adopting any updates.

## Lib-React

### V. 0.3.0

* Rewrite from core. The code is (I think) better organized.

*  Different API.

* The reactor is now stateless instead of an object. All state data (except the reactor's ID, which remains static throughout during it's existence), is managed by the OS implementation (`kqueue`/`epoll`).

* Callbacks are statically linked instead of dynamically assigned.

* Better integration with `libsock`.

* (optional) Handles `libsock`'s UUID instead of direct file descriptors, preventing file descriptor collisions.

### V. 0.2.2

* Fixed support for `libsock`, where the `sock_flush` wasn't automatically called due to inline function optimizations used by the compiler (and my errant code).

### V. 0.2.1

Baseline (changes not logged before this point in time).

## Lib-Sock

### V. 0.2.3 (next version number)

* Apple's `getrlimit` is broken, causing server capacity limits to be less than they could / should be.

### V. 0.2.2

* Fixed an issue introduced in `libsock` 0.2.1, where `sock_close` wouldn't close the socket even when all the data was sent.

### V. 0.2.1

* Larger user level buffer - increased from ~4Mb to ~16Mb.

* The system call to `write` will be deferred (asynchronous) when using `libasync`. This can be changed by updating the `SOCK_DELAY_WRITE` value in the `libsock.c` file.

    This will not prevent `sock_write` from emulating a blocking state while the user level buffer is full.

### V. 0.2.0

* Almost the same API. Notice the following: no initialization required; rw_hooks callbacks aren't protected by a lock (use your own thread protection code).

   There was an unknown issue with version 0.1.0 that caused large data sending to hang... tracking it proved harder then re-writing the whole logic, which was both easier and allowed for simplifying some of the code for better maintenance.

* `sock_checkout_packet` will now hang until a packet becomes available. Don't check out more then a single packet at a time and don't hold on to checked out packets, or you might find your threads waiting.

### V. 0.1.0

* Huge rewrite. Different API.

* Uses connection UUIDs instead of direct file descriptors, preventing file descriptor collisions. Note that the UUIDs aren't random and cannot be used to identify the connections across machines or processes.

* No global lock, spin-lock oriented design.

* Better (optional) integration with `libreact`.

### V. 0.0.6

* `libsock` experienced minor API changes, specifically to the `init_socklib` function (which now accepts 0 arguments).

* The `rw_hooks` now support a `flush` callback for hooks that keep an internal buffer. Implementing the `flush` callback will allow these callbacks to prevent a pre-mature closure of the socket stream and ensure that all the data will be sent.

### V. 0.0.5

* Added the client implementation (`sock_connect`).

* Rewrote the whole library to allow for a fixed user-land buffer limit. This means that instead of having buffer packets automatically allocated when more memory is required, the `sock_write(2)` function will hang and flush any pending socket buffers until packets become available.

* File sending is now offset based, so `fseek` data is ignored. This means that it would be possible to cache open `fd` files and send the same file descriptor to multiple clients.

### V. 0.0.4

* Fixed issues with non-system `sendfile` and with underused packet pool memory.

* Added the `.metadata.keep_open` flag, to allow file caching... however, keep in mind that the file offset for read/write is the file's `lseek` position and sending the same file to different sockets will create race conditions related to the file `lseek` position.

* Fix for epoll's on_ready not being sent (sock flush must raise the EAGAIN error, or the on_ready event will not get called). Kqueue is better since the `on_ready` refers to the buffer being clear instead of available (less events to copy the same amount of data, as each data write is optimal when enough data is available to be written).

* optional implementation of sendfile for Apple, BSD and Linux (BSD **not** tested).

* Misc. optimizations. i.e. Buffer packet size now increased to 64Kb, to fit Linux buffer allocation.

* File sending now supports file descriptors.

* TLC support replaced with a simplified read/write hook.

* Changed `struct SockWriteOpt` to a typedef `sock_write_info_s`.

### V. 0.0.3

* Changed `struct Packet` to a typedef `sock_packet_s`.

* fixed and issue where using `sock_write(2)` for big data chunks would cause errors when copying the data to the user buffer.

    it should be noted, for performance reasons, that it is better to send big data using external pointers (especially if the data is cashed) using the `sock_send_packet` function - for cached data, do not set the `packet->external` flag, so the data isn't freed after it was sent.

### V. 0.0.2

* fixed situations in which the `send_packet` might not close a file (if the packet buffer references a `FILE`) before returning an error.

* The use of `sock_free_packet` is now required for any unused Packet object pointers checked out using `sock_checkout_packet`.

    This requirement allows the pool management to minimize memory fragmentation for long running processes.

* `libsock` memory requirements are now higher, as the user land buffer's Packet memory pool is pre-allocated to minimize memory fragmentation.

* Corrected documentation mistakes, such as the one stating that the `sock_send_packet` function will not handle the Packet object's memory on error (it does handle the memory, **always**).

### V. 0.0.1

Baseline (changes not logged before this point in time).

## Lib-Async

### V. 0.4.0

* I rewrote (almost) everything.

* `libasync` now behaves as a global state machine. No more `async_p` objects.

* Uses (by default) `nanosleep` instead of pipes (you can revert back to pipes by setting a simple flag). This, so far, seems to provide better performance (at the expense of a slightly increased CPU load).

### V. 0.3.0

* Fixed task pool initialization to zero-out data that might cause segmentation faults.

* `libasync`'s task pool optimizations and limits were added to minimize memory fragmentation issues for long running processes.

Baseline (changes not logged before this point in time).

## Lib-Server

### V. 0.4.2 (next version number)

* Limited the number of threads (1023) and processes (127) that can be invoked without changing the library's code.

* Minor performance oriented changes.

* Fixed an issue where Websocket upgrade would allow code execution in parallel with `on_open` (protocol locking was fixed while switching the protocol).

* Added `server_each_unsafe` to iterate over all client connections to perform a task. The `unsafe` part in the name is very important - for example, memory could be deallocated during execution.

### V. 0.4.1

* Minor performance oriented changes.

* Shutdown process should now allow single threaded asynchronous (evented) task scheduling.

* Updating a socket's timeout settings automatically "touches" the socket (resets the timeout count).

### V. 0.4.0

* Rewrite from core. The code is more concise with less duplications.

* Different API.

* The server is now a global state machine instead of an object.

* Better integration with `libsock`.

* Handles `libsock`'s UUID instead of direct file descriptors, preventing file descriptor collisions and preventing long running tasks from writing to the wrong client (i.e., if file descriptor 6 got disconnected and someone else connected and receive file descriptor 6 to identify the socket).

* Better concurrency protection and protocol cleanup `on_close`. Now, deferred tasks (`server_task` / `server_each`), the `on_data` callback and even the `on_close` callback all run within a connection's "lock" (busy flag), limiting concurrency for a single connection to the `on_ready` and `ping` callbacks. No it is safe to free the protocol's memory during an `on_close` callback, as it is (almost) guarantied that no running tasks are using that memory (this assumes that `ping` and `on_ready` don't use any data placed protocol's memory).

### V. 0.3.5

* Moved the global server lock (the one protecting global server data integrity) from a mutex to a spin-lock. Considering API design changes that might allow avoiding a lock.

* File sending is now offset based, so `lseek` data is ignored. This means that it should be possible to cache open `fd` files and send the same file descriptor to multiple clients.

### V. 0.3.4

* Updated `sendfile` to only accept file descriptors (not `FILE *`). This is an optimization requirement.

### V. 0.3.3

* fixed situations in which the `sendfile` might not close the file before returning an error.

* There was a chance that the `on_data` callback might return after the connection was disconnected and a new connection was established for the same `fd`. This could have caused the `busy` flag to be cleared even if the new connection was actually busy. This potential issue had been fixed by checking the connection against the UUID counter before clearing the `busy` flag.

* reminder: The `Server.rw_hooks` feature is deprecated. Use `libsock`'s TLC (Transport Layer Callbacks) features instead.

### V. 0.3.2

Baseline (changes not logged before this point in time).

## MiniCrypt (development incomplete)

### V. 0.1.1

* added a "dirty" (and somewhat faster then libc) `gmtime` implementation that ignores localization.

Baseline (changes not logged before this point in time).

## HTTP Protocol

* Sep. 13, 2016: `ETag` support for the static file server, responding with 304 on valid `If-None-Match`.

* Sep. 13, 2016: Updated `HEAD` request handling for static files.

* Fixed pipelining... I think.

* Jun 26, 2016: Fixed logging for static file range requests.

* Jun 26, 2016: Moved URL decoding logic to the `HttpRequest` object.

* Jun 20, 2016: Added basic logging support.

* Jun 20, 2016: Added automatic `Content-Length` header constraints when setting status code to 1xx, 204 or 304.

* Jun 20, 2016: Nicer messages on startup.

* Jun 20, 2016: Updated for new `lib-server` and `libsock`.

* Jun 16, 2016: HttpResponse date handling now utilizes a faster (and dirtier) solution then the standard libc `gmtime_r` and `strftime` solutions.

* Jun 12, 2016: HTTP protocol and HttpResponse `sendfile` and `HttpResponse.sendfile` fixes and optimizations. Now file sending uses file descriptors instead of `FILE *`, avoiding the memory allocations related to `FILE *` data.

* Jun 12, 2016: HttpResponse copy optimizes the first header buffer packet to copy as much of the body as possible into the buffer packet, right after the headers.

* Jun 12, 2016: Optimized mime type search for static file service.

* Jun 9, 2016: Rewrote the HttpResponse implementation to leverage `libsock`'s direct user-land buffer packet injection, minimizing user land data-copying.

* Jun 9, 2016: rewrote the HTTP `sendfile` handling for public folder settings.

* Jun 9, 2016: Fixed an issue related to the new pooling scheme, where old data would persist in some pooled request objects.

* Jun 8, 2016: The HttpRequest object is now being pooled within the request library (not the HTTP protocol implementation) using Atomics (less mutex locking) and minimizing memory fragmentation by pre-initializing the buffer on first request (preventing memory allocated after the first request from getting "stuck behind" any of the pool members).

Jun 7, 2016: Baseline (changes not logged before this point in time).

## Websocket extension

* Resolved issue #6, Credit to @Filly for exposing the issue.

* Memory pool removed. Might be reinstated after patching it up, but when more tests were added, the memory pool failed them.

* Jan 12, 2017: Memory Performance.

     The Websocket connection Protocol now utilizes both a C level memory pool and a local thread storage for temporary data. This helps mitigate possible memory fragmentation issues related to long running processes and long-lived objects.

     In addition, the socket `read` buffer was moved from the protocol object to a local thread storage (assumes pthreads and not green threads). This minimizes the memory footprint for each connection (at the expense of memory locality) and should allow Iodine to support more concurrent connections using less system resources.

     Last, but not least, the default message buffer per connection starts at 4Kb instead of 16Kb (grows as needed, up to `Iodine::Rack.max_msg_size`), assuming smaller messages are the norm.

### Date 20160607

Baseline (changes not logged before this point in time).
