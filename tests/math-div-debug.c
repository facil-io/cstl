/* Debug test for fio_math_div */
#define FIO_CORE
#define FIO_MATH
#include "../fio-stl/include.h"

int main(void) {
  uint64_t a[4] = {100, 0, 0, 0};
  uint64_t b[4] = {7, 0, 0, 0};

  FIO_LOG_DDEBUG("Testing 100 / 7...");
  FIO_LOG_DDEBUG("b_msb = %zu", fio_math_msb_index(b, 4));
  FIO_LOG_DDEBUG("a_msb = %zu", fio_math_msb_index(a, 4));

  /* Manual trace */
  uint64_t tr[4], tt[4], tq[4];
  memcpy(tr, a, sizeof(tr));
  memset(tq, 0, sizeof(tq));

  size_t b_msb = fio_math_msb_index(b, 4);
  FIO_LOG_DDEBUG("b_msb = %zu", b_msb);

  int iter = 0;
  for (;;) {
    size_t r_msb = fio_math_msb_index(tr, 4);
    FIO_LOG_DDEBUG("Iter %d: r_msb = %zu (r[0]=%llu)",
                   iter,
                   r_msb,
                   (unsigned long long)tr[0]);

    if (r_msb == (size_t)-1 || r_msb < b_msb) {
      FIO_LOG_DDEBUG("Breaking: r_msb=%zu, b_msb=%zu", r_msb, b_msb);
      break;
    }

    size_t shift = r_msb - b_msb;
    FIO_LOG_DDEBUG("  shift = %zu", shift);

    fio_math_shl(tt, b, shift, 4);
    FIO_LOG_DDEBUG("  shifted b = %llu", (unsigned long long)tt[0]);

    uint64_t borrow = fio_math_sub(tt, tr, tt, 4);
    FIO_LOG_DDEBUG("  after sub: tt[0]=%llu, borrow=%llu",
                   (unsigned long long)tt[0],
                   (unsigned long long)borrow);

    if (!borrow) {
      memcpy(tr, tt, sizeof(tr));
      tq[shift >> 6] |= (1ULL << (shift & 63));
      FIO_LOG_DDEBUG("  Updated r[0]=%llu, q[0]=%llu",
                     (unsigned long long)tr[0],
                     (unsigned long long)tq[0]);
    } else if (shift == 0) {
      FIO_LOG_DDEBUG("  Breaking due to shift==0 and borrow");
      break;
    } else {
      FIO_LOG_DDEBUG("  Borrow, not updating");
    }

    if (++iter > 20) {
      FIO_LOG_DDEBUG("Too many iterations, aborting");
      break;
    }
  }

  FIO_LOG_DDEBUG("Final: q[0]=%llu, r[0]=%llu",
                 (unsigned long long)tq[0],
                 (unsigned long long)tr[0]);

  return 0;
}
