/* *****************************************************************************
Generator: Ed25519 constant tables in `fio-stl/154 ed25519.h`

Not a unit test (excluded from `make tests` via TEST_EXCLUDE in the makefile).

Computes, from the base point and curve constants only (never reading the
table it reproduces):

* `fio___ge_base_table[16]` - table[i] = (i+1)*B in affine precomp form
  (y+x, y-x, 2*d*x*y), each field normalized to canonical radix-2^51 limbs.
* The canonical y encodings of the 8 small-order (torsion) points, the source
  of `fio___ed25519_is_small_order`'s blocklist (sign bit cleared, deduped).

Build (via make) and run:

    make tmp/tests/generators/ed25519-table
    ./tmp/tests/generators/ed25519-table --check   # exit 1 on any mismatch

Rebuild the table and splice it into the header (replaces the block from the
`static const fio___ge_precomp_s fio___ge_base_table[16] = {` line through the
following `};` line, between the `clang-format off/on` markers):

    ./tmp/tests/generators/ed25519-table > tmp/ed25519-table.inc
    awk 'FNR==NR {t = t $0 "\n"; next}
         /^static const fio___ge_precomp_s fio___ge_base_table\[16\]/ {
           printf "%s", t; s = 1 }
         s { if (/^};/) s = 0; next }
         1' tmp/ed25519-table.inc "fio-stl/154 ed25519.h" > tmp/ed25519.h &&
      mv tmp/ed25519.h "fio-stl/154 ed25519.h"

Then rebuild and run `make tests/ed25519 tests/x25519` (and `--check`). The
table depends only on the field representation; regenerate it if the radix
or `fio___ge_precomp_s` layout changes.
***************************************************************************** */
#include "../test-helpers.h"

#define FIO_ED25519
#define FIO_SHA2
#include FIO_INCLUDE_FILE

/* normalize to canonical limbs (< 2^51, value < p) via a bytes round-trip */
static void ed_gen_norm(fio___gf_s r, fio___gf_s a) {
  uint8_t b[32];
  fio___gf_tobytes(b, a);
  fio___gf_frombytes(r, b);
}

/* compute the base table from scratch */
static void ed_gen_table(fio___ge_precomp_s t[16]) {
  fio___ge_p3_s B, P;
  fio___ge_p3_base(B);
  FIO_MEMCPY(P, B, sizeof(P));
  for (int i = 0; i < 16; ++i) {
    if (i)
      fio___ge_p3_add(P, (const fio___gf_s *)B);
    fio___gf_s zi, x, y, v;
    fio___gf_inv(zi, P[2]);
    fio___gf_mul(x, P[0], zi);
    fio___gf_mul(y, P[1], zi);
    fio___gf_add(v, y, x);
    ed_gen_norm(t[i].ypx, v);
    fio___gf_sub(v, y, x);
    ed_gen_norm(t[i].ymx, v);
    fio___gf_mul(v, x, y);
    fio___gf_mul(v, v, FIO___ED25519_D2);
    ed_gen_norm(t[i].xy2d, v);
  }
}

static void ed_gen_print_field(const char *name, const fio___gf_s a) {
  printf("     /* %s */ {", name);
  for (int j = 0; j < 5; ++j)
    printf("0x%013llxULL%s", (unsigned long long)a[j], j < 4 ? ", " : "");
  printf("},\n");
}

static void ed_gen_print_table(const fio___ge_precomp_s t[16]) {
  printf("static const fio___ge_precomp_s fio___ge_base_table[16] = {\n");
  for (int i = 0; i < 16; ++i) {
    printf("    {/* %d * B */\n", i + 1);
    ed_gen_print_field("ypx", t[i].ypx);
    ed_gen_print_field("ymx", t[i].ymx);
    ed_gen_print_field("xy2d", t[i].xy2d);
    printf("    },\n");
  }
  printf("};\n");
}

/* computes the 8 small-order points [i]T (i = 0..7) for a point T of order 8;
 * returns 0 on success */
static int ed_gen_small_order(uint8_t out[8][32]) {
  static const uint8_t L[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12,
                                0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9,
                                0xde, 0x14, 0,    0,    0,    0,    0,
                                0,    0,    0,    0,    0,    0,    0,
                                0,    0,    0,    0x10};
  for (int seed = 0; seed < 1024; ++seed) {
    /* deterministic candidate encodings; about half decode to curve points */
    fio_u256 h = fio_sha256(&seed, sizeof(seed));
    h.u8[31] &= 0x7F;
    fio___ge_p3_s Q, T, S;
    if (fio___ge_p3_frombytes(Q, h.u8))
      continue;
    /* T = [L]Q kills the prime-order part, leaving the torsion component */
    fio___gf_zero(T[0]);
    fio___gf_one(T[1]);
    fio___gf_one(T[2]);
    fio___gf_zero(T[3]);
    for (int i = 255; i >= 0; --i) {
      fio___ge_p3_dbl(T);
      if ((L[i >> 3] >> (i & 7)) & 1)
        fio___ge_p3_add(T, (const fio___gf_s *)Q);
    }
    /* need order exactly 8: [4]T must not be the identity (encoding {1}) */
    FIO_MEMCPY(S, T, sizeof(S));
    fio___ge_p3_dbl(S);
    fio___ge_p3_dbl(S);
    uint8_t e[32], identity[32] = {1};
    fio___ge_p3_tobytes(e, S);
    if (!FIO_MEMCMP(e, identity, 32))
      continue;
    fio___gf_zero(S[0]);
    fio___gf_one(S[1]);
    fio___gf_one(S[2]);
    fio___gf_zero(S[3]);
    for (int i = 0; i < 8; ++i) {
      fio___ge_p3_tobytes(out[i], S);
      fio___ge_p3_add(S, (const fio___gf_s *)T);
    }
    return 0;
  }
  return -1;
}

static int ed_gen_check(void) {
  int errors = 0;
  fio___ge_precomp_s t[16];
  ed_gen_table(t);
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 5; ++j) {
      if (t[i].ypx[j] != fio___ge_base_table[i].ypx[j] ||
          t[i].ymx[j] != fio___ge_base_table[i].ymx[j] ||
          t[i].xy2d[j] != fio___ge_base_table[i].xy2d[j]) {
        fprintf(stderr, "base table mismatch: entry %d limb %d\n", i, j);
        ++errors;
      }
    }
  }
  uint8_t so[8][32];
  if (ed_gen_small_order(so)) {
    fprintf(stderr, "failed to find an order-8 point\n");
    return 1;
  }
  for (int i = 0; i < 8; ++i) {
    if (!fio___ed25519_is_small_order(so[i])) {
      fprintf(stderr, "small-order point [%d]T not in blocklist\n", i);
      ++errors;
    }
  }
  if (errors)
    return 1;
  fprintf(stderr, "OK: base table (16 entries) and small-order blocklist (8 "
                  "points) match fio-stl/154 ed25519.h\n");
  return 0;
}

static void ed_gen_print_small_order(void) {
  uint8_t so[8][32];
  if (ed_gen_small_order(so)) {
    fprintf(stderr, "failed to find an order-8 point\n");
    exit(1);
  }
  for (int i = 0; i < 8; ++i) {
    so[i][31] &= 0x7F; /* the blocklist ignores the sign bit */
    int dup = 0;
    for (int k = 0; k < i; ++k)
      dup |= !FIO_MEMCMP(so[k], so[i], 32);
    if (dup)
      continue;
    printf("/* [%d]T */ {", i);
    for (int j = 0; j < 32; ++j)
      printf("0x%02x%s", so[i][j], j < 31 ? ", " : "");
    printf("},\n");
  }
}

int main(int argc, char const *argv[]) {
  const char *mode = argc > 1 ? argv[1] : "";
  if (!strcmp(mode, "--help") || !strcmp(mode, "-h")) {
    printf("usage: %s [--check | --small-order | --help]\n"
           "  (no args)      print fio___ge_base_table to splice into "
           "fio-stl/154 ed25519.h\n"
           "  --check        compare against the compiled header; exit 1 on "
           "mismatch\n"
           "  --small-order  print canonical small-order y encodings "
           "(blocklist source)\n"
           "See the header comment of tests/generators/ed25519-table.c for "
           "the splice command.\n",
           argv[0]);
    return 0;
  }
  if (!strcmp(mode, "--check"))
    return ed_gen_check();
  if (!strcmp(mode, "--small-order")) {
    ed_gen_print_small_order();
    return 0;
  }
  if (*mode) {
    fprintf(stderr, "unknown option: %s (try --help)\n", mode);
    return 1;
  }
  fio___ge_precomp_s t[16];
  ed_gen_table(t);
  ed_gen_print_table(t);
  return 0;
}
