// dtoa bigint Gay's algorithm
/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991, 2000, 2001 by Lucent Technologies.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/
#ifndef IV_DTOA_BIGINT_H_
#define IV_DTOA_BIGINT_H_
#include <cmath>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cfloat>
#include <vector>
#include <string>
#include <algorithm>
#include "detail/cstdint.h"
#include "detail/array.h"
#include "detail/memory.h"
#include "platform_math.h"
#include "round.h"
#include "thread.h"
#include "byteorder.h"
#include "stringpiece.h"
#include "static_assert.h"

namespace iv {
namespace core {
namespace dtoa {

static const std::size_t kDToABufferSize = 96;

union U {
  double d;
  uint32_t L[2];
};

// IEEE754 double macros
#define Exp_shift           20
#define Exp_shift1          20
#define Exp_msk1      0x100000
#define Exp_msk11     0x100000
#define Exp_mask    0x7ff00000
#define P                   53
#define Bias              1023
#define Emin           (-1022)
#define Exp_1       0x3ff00000
#define Exp_11      0x3ff00000
#define Ebits               11
#define Frac_mask      0xfffff
#define Frac_mask1     0xfffff
#define Bletch            0x10
#define Bndry_mask     0xfffff
#define Bndry_mask1    0xfffff
#define LSB                  1
#define Sign_bit    0x80000000
#define Log2P                1
#define Tiny0                0
#define Tiny1                1
#define Quick_max           14
#define Int_max             14

#define rounded_product(a, b) a *= b
#define rounded_quotient(a, b) a /= b

#define Big0 (Frac_mask1 | Exp_msk1 * (DBL_MAX_EXP + Bias - 1))
#define Big1 0xffffffff

#ifdef IV_IS_LITTLE_ENDIAN
#define word0(x) (x)->L[1]
#define word1(x) (x)->L[0]
#else
#define word0(x) (x)->L[0]
#define word1(x) (x)->L[1]
#endif

#define dval(x) (x)->d

static const std::array<double, 23> kTens = { {
  1e0,   1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,
  1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
  1e20, 1e21, 1e22
} };

static const std::array<double, 5> kBigTens = { {
  1e16, 1e32, 1e64, 1e128, 1e256
} };

static const std::array<double, 5> kTinyTens = { {
  1e-16, 1e-32, 1e-64, 1e-128,
  9007199254740992. * 9007199254740992.e-256  // = 2^106 * 1e-256
} };

#define Scale_Bit 0x10

inline int Hi0Bits(uint32_t x) {
  int k = 0;
  if (!(x & 0xffff0000)) {
    k = 16;
    x <<= 16;
  }
  if (!(x & 0xff000000)) {
    k += 8;
    x <<= 8;
  }
  if (!(x & 0xf0000000)) {
    k += 4;
    x <<= 4;
  }
  if (!(x & 0xc0000000)) {
    k += 2;
    x <<= 2;
  }
  if (!(x & 0x80000000)) {
    ++k;
    if (!(x & 0x40000000)) {
      return 32;
    }
  }
  return k;
}

inline int Low0Bits(uint32_t* y) {
  int k;
  uint32_t x = *y;
  if (x & 7) {
    if (x & 1) {
      return 0;
    }
    if (x & 2) {
      *y = x >> 1;
      return 1;
    }
    *y = x >> 2;
    return 2;
  }
  k = 0;
  if (!(x & 0xffff)) {
    k = 16;
    x >>= 16;
  }
  if (!(x & 0xff)) {
    k += 8;
    x >>= 8;
  }
  if (!(x & 0xf)) {
    k += 4;
    x >>= 4;
  }
  if (!(x & 0x3)) {
    k += 2;
    x >>= 2;
  }
  if (!(x & 1)) {
    ++k;
    x >>= 1;
    if (!x) {
      return 32;
    }
  }
  *y = x;
  return k;
}

inline double ULP(U* x) {
  U u;
  const int32_t L = (word0(x) & Exp_mask) -  (P - 1) * Exp_msk1;
  word0(&u) = L;
  word1(&u) = 0;
  return dval(&u);
}

namespace detail {

static thread::Mutex kBigIntMutex;

}  // namespace iv::core::dtoa::detail

class BigInt : protected std::vector<uint32_t> {
 public:
  typedef std::vector<uint32_t> container_type;
  typedef BigInt this_type;

  BigInt() : container_type(), sign_(0) { }

  explicit BigInt(int i)
    : container_type(1),
      sign_(0) {
    (*this)[0] = i;
  }

  BigInt(U* d, int* e, int* bits)
    : container_type(1),
      sign_(0) {
#define d0 word0(d)
#define d1 word1(d)
    uint32_t z = d0 & Frac_mask;
    d0 &= 0x7fffffff;

    int de;
    if ((de = static_cast<int>(d0 >> Exp_shift))) {
      z |= Exp_msk1;
    }
    int i, k;
    if (uint32_t y = d1) {
      if ((k = Low0Bits(&y))) {
        (*this)[0] = y | (z << (32 - k));
        z >>= k;
      } else {
        (*this)[0] = y;
      }
      if (z) {
        resize(2);
        (*this)[1] = z;
      }
      i = size();
    } else {
      k = Low0Bits(&z);
      (*this)[0] = z;
      i = 1;
      resize(1);
      k += 32;
    }
    if (de) {
      *e = de - Bias - (P - 1) + k;
      *bits = P - k;
    } else {
      *e = de - Bias - (P - 1) + 1 + k;
      *bits = (32 * i) - Hi0Bits((*this)[i - 1]);
    }
#undef d0
#undef d1
  }

  void clear() {
    sign_ = 0;
    container_type::clear();
  }

  void SetValue(int i) {
    clear();
    resize(1);
    (*this)[0] = i;
  }

  using container_type::size;
  using container_type::resize;
  using container_type::begin;
  using container_type::end;
  using container_type::data;
  using container_type::operator[];

  BigInt& operator*=(const BigInt& rhs) {
    (operator*(*this, rhs)).swap(*this);
    return *this;
  }

  friend BigInt operator*(const BigInt& lhs, const BigInt& rhs) {
    if (lhs.size() < rhs.size()) {
      return operator*(rhs, lhs);
    }
    const int wa = lhs.size();
    const int wb = rhs.size();
    int wc = wa + wb;
    BigInt c;
    c.resize(wc);
    std::fill_n(c.begin(), wc, 0u);

    container_type::iterator xc0 = c.begin();
    for (container_type::const_iterator xa = lhs.begin(),
         xae = xa + wa,
         xb = rhs.begin(),
         xbe = xb + wb;
         xb < xbe; ++xc0) {
      if (uint32_t y = *xb++) {
        container_type::const_iterator x = xa;
        container_type::iterator xc = xc0;
        uint64_t carry = 0;
        do {
          const uint64_t z = (*x++) * static_cast<uint64_t>(y) + *xc + carry;
          carry = z >> 32;
          *xc++ = static_cast<uint32_t>(z) & 0xFFFFFFFFUL;
        } while (x < xae);
        *xc = static_cast<uint32_t>(carry);
      }
    }

    for (container_type::const_iterator xc0 = c.begin(),
         xc = xc0 + wc; wc > 0 && !*--xc; --wc) {
    }
    c.resize(wc);
    return c;
  }

  BigInt& operator<<=(int k) {  // NOLINT
    const int n = k >> 5;
    const int original = size();
    const int n1 = n + original + 1;
    if (k &= 0x1f) {
      resize(size() + n + 1);
    } else {
      resize(size() + n);
    }
    const container_type::const_pointer src_begin  = data();
    const container_type::pointer dst_begin = data();
    container_type::const_pointer src = src_begin + original - 1;
    container_type::pointer dst = dst_begin + n1 - 1;
    if (k) {
      uint32_t hi_subword = 0;
      int s = 32 - k;
      for (; src >= src_begin; --src) {
        *dst-- = hi_subword | *src >> s;
        hi_subword = *src << k;
      }
      *dst = hi_subword;
      assert(dst == dst_begin + n);
      resize(original + n + !!(*this)[n1 - 1]);
    } else {
      do {
        *--dst = *src--;
      } while (src >= src_begin);
    }
    for (dst = dst_begin + n; dst != dst_begin;) {
      *--dst = 0;
    }
    assert(size() <= 1 || (*this)[size() - 1]);
    return *this;
  }

  friend BigInt operator<<(const BigInt& lhs, int k) {
    BigInt c(lhs);
    c <<= k;
    return c;
  }

  double ToDouble(int* e) const {
    U d;
#define d0 word0(&d)
#define d1 word1(&d)
    container_type::const_iterator xa0 = begin();
    container_type::const_iterator xa = end();
    uint32_t y = *--xa;
    assert(y);
    int k = Hi0Bits(y);
    *e = 32 - k;
    if (k < Ebits) {
      d0 = Exp_1 | (y >> (Ebits - k));
      const uint32_t w = xa > xa0 ? *--xa : 0;
      d1 = (y << (32 - Ebits + k)) | (w >> (Ebits - k));
      return dval(&d);
    }
    const uint32_t z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits) {
      d0 = Exp_1 | (y << k) | (z >> (32 - k));
      y = xa > xa0 ? *--xa : 0;
      d1 = (z << k) | (y >> (32 - k));
    } else {
      d0 = Exp_1 | y;
      d1 = z;
    }
#undef d0
#undef d1
    return dval(&d);
  }

  // multiply by m and add a
  void MultiAdd(int m, int a) {
    uint64_t carry = a;
    const int wds = size();
    int i = 0;
    container_type::iterator it = begin();
    do {
      const uint64_t y = (*it) * static_cast<uint64_t>(m) + carry;
      carry = y >> 32;
      *it++ = static_cast<uint32_t>(y) & 0xFFFFFFFFUL;
    } while (++i < wds);
    if (carry) {
      push_back(static_cast<uint32_t>(carry));
    }
  }

  double Ratio(const BigInt& rhs) const {
    U da, db;
    int ka, kb;
    dval(&da) = ToDouble(&ka);
    dval(&db) = rhs.ToDouble(&kb);
    const int k = ka - kb + 32 * (size() - rhs.size());
    if (k > 0) {
      word0(&da) += k * Exp_msk1;
    } else {
      word0(&db) += (-k) * Exp_msk1;
    }
    return dval(&da) / dval(&db);
  }

  BigInt Diff(const BigInt& rhs) const {
    using std::swap;
    const BigInt* a = this;
    const BigInt* b = &rhs;
    int i = Compare(rhs);
    if (!i) {
      return BigInt(0);
    }
    if (i < 0) {
      swap(a, b);
      i = 1;
    } else {
      i = 0;
    }
    int wa = a->size();
    container_type::const_iterator xa = a->begin();
    const container_type::const_iterator xae = xa + wa;
    const int wb = b->size();
    container_type::const_iterator xb = b->begin();
    const container_type::const_iterator xbe = xb + wb;

    BigInt c;
    c.resize(wa);
    c.sign_ = i;
    container_type::iterator xc = c.begin();
    uint64_t borrow = 0;
    do {
      const uint64_t y = static_cast<uint64_t>(*xa++) - *xb++ - borrow;
      borrow = y >> 32 & static_cast<uint32_t>(1);
      *xc++ = static_cast<uint32_t>(y) & 0xFFFFFFFFUL;
    } while (xb < xbe);
    while (xa < xae) {
      const uint64_t y = *xa++ - borrow;
      borrow = y >> 32 & static_cast<uint32_t>(1);
      *xc++ = static_cast<uint32_t>(y) & 0xFFFFFFFFUL;
    }
    while (!*--xc) {
      wa--;
    }
    c.resize(wa);
    return c;
  }

  int Compare(const BigInt& rhs) const {
    int i = size();
    int j = rhs.size();
    if (i -= j) {
      return i;
    }
    const container_type::const_iterator xa0 = begin();
    container_type::const_iterator xa = xa0 + j;
    const container_type::const_iterator xb0 = rhs.begin();
    container_type::const_iterator xb = xb0 + j;
    for (;;) {
      if (*--xa != *--xb) {
        return *xa < *xb ? -1 : 1;
      }
      if (xa <= xa0) {
        break;
      }
    }
    return 0;
  }

  friend bool operator==(const BigInt& rhs, const BigInt& lhs) {
    return rhs.Compare(lhs) == 0;
  }

  friend bool operator!=(const BigInt& rhs, const BigInt& lhs) {
    return rhs.Compare(lhs) != 0;
  }

  friend bool operator<(const BigInt& rhs, const BigInt& lhs) {
    return rhs.Compare(lhs) < 0;
  }

  friend bool operator<=(const BigInt& rhs, const BigInt& lhs) {
    return rhs.Compare(lhs) <= 0;
  }

  friend bool operator>(const BigInt& rhs, const BigInt& lhs) {
    return rhs.Compare(lhs) > 0;
  }

  friend bool operator>=(const BigInt& rhs, const BigInt& lhs) {
    return rhs.Compare(lhs) >= 0;
  }

  friend void swap(BigInt& lhs, BigInt& rhs) {
    lhs.swap(rhs);
  }

  void swap(BigInt& rhs) {
    using std::swap;
    container_type::swap(rhs);
    swap(sign_, rhs.sign_);
  }

  int sign() const {
    return sign_;
  }

  void S2B(const char* s, int nd0, int nd, uint32_t y9) {
    sign_ = 0;
    resize(1);
    (*this)[0] = y9;

    int i = 9;
    if (9 < nd0) {
      s += 9;
      do {
        MultiAdd(10, *s++ - '0');
      } while (++i < nd0);
      ++s;
    } else {
      s += 10;
    }
    for (; i < nd; ++i) {
      MultiAdd(10, *s++ - '0');
    }
  }

  typedef std::array<BigInt, 30> array_type;

  void Pow5Multi(int k) {
    typedef std::vector<BigInt> list_type;
    static const std::array<int, 3> p05 = { { 5, 25, 125 } };
    static list_type kList;
    if (const int i = k & 3) {
      MultiAdd(p05[i - 1], 0);
    }
    if (!(k >>= 2)) {
      return;
    }
    {
      thread::ScopedLock<thread::Mutex> lock(&detail::kBigIntMutex);
      list_type::const_iterator it = kList.begin();
      if (it == kList.end()) {
        kList.push_back(BigInt(625));
        it = kList.begin();
      }
      while (true) {
        if (k & 1) {
          (operator*(*this, *it)).swap(*this);
        }
        if (!(k >>= 1)) {
          break;
        }
        if ((it + 1) == kList.end()) {
          kList.push_back(operator*(*it, *it));
          it = kList.end() - 1;
        } else {
          ++it;
        }
      }
      return;
    }
  }

 private:
  int sign_;
};

inline int Quorem(BigInt* b, BigInt* S) {
  typedef BigInt::container_type container_type;
  assert(b->size() <= 1 || (*b)[b->size() - 1]);
  assert(S->size() <= 1 || (*S)[S->size() - 1]);

  std::size_t n = S->size();
  assert(b->size() <= n);
  if (b->size() < n) {
    return 0;
  }
  container_type::iterator sx = S->begin();
  container_type::iterator sxe = sx + --n;

  container_type::iterator bx = b->begin();
  container_type::iterator bxe = bx + n;
  uint32_t q = *bxe / (*sxe + 1);
  assert(q <= 9);
  if (q) {
    uint64_t borrow = 0;
    uint64_t carry = 0;
    do {
      const uint64_t ys = *sx++ * static_cast<uint64_t>(q) + carry;
      carry = ys >> 32;
      const uint64_t y = *bx - (ys & 0xffffffffUL) - borrow;
      borrow = y >> 32 & static_cast<uint32_t>(1);
      *bx++ = static_cast<uint32_t>(y) & 0xffffffffUL;
    } while (sx <= sxe);
    if (!*bxe) {
      bx = b->begin();
      while (--bxe > bx && !*bxe) {
        --n;
      }
      b->resize(n);
    }
  }

  if (b->Compare(*S) >= 0) {
    ++q;
    uint64_t borrow = 0;
    uint64_t carry = 0;
    bx = b->begin();
    sx = S->begin();
    do {
      const uint64_t ys = *sx++ + carry;
      carry = ys >> 32;
      const uint64_t y = *bx - (ys & 0xffffffffUL) - borrow;
      borrow = y >> 32 & static_cast<uint32_t>(1);
      *bx++ = static_cast<uint32_t>(y) & 0xffffffffUL;
    } while (sx <= sxe);
    bx = b->begin();
    bxe = bx + n;
    if (!*bxe) {
      while (--bxe > bx && !*bxe) {
        --n;
      }
      b->resize(n);
    }
  }
  return q;
}

template<bool RoundingNone,
         bool RoundingSignificantFigures,
         bool RoundingDecimalPlaces,
         bool LeftRight,
         typename Buffer>
inline void DoubleToASCII(Buffer* buf,
                          double dd, int ndigits,
                          bool* sign_out,
                          int* exponent_out, unsigned* precision_out) {
  IV_STATIC_ASSERT(
      RoundingNone + RoundingSignificantFigures + RoundingDecimalPlaces == 1);
  IV_STATIC_ASSERT(!RoundingNone || LeftRight);
  assert(!IsNaN(dd) && !core::IsInf(dd));

  U u;
  u.d = dd;
  int32_t L;

  int m2, m5, special_case, dig;
  Buffer* s = buf;
  Buffer* const s0 = buf;
  int ieps;
  int j1;

  // reject Infinity or NaN
  assert((word0(&u) & Exp_mask) != Exp_mask);

  if (!dval(&u)) {
    *sign_out = false;
    *exponent_out = 0;
    *precision_out = 1;
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }

  if (word0(&u) & Sign_bit) {
    // set sign for everything, including 0's and NaNs
    *sign_out = true;
    word0(&u) &= ~Sign_bit;  // clear sign bit
  } else {
    *sign_out = false;
  }

  int be, bbits;
  BigInt b(&u, &be, &bbits);
  int i;
  int denorm;
  U d2, eps;
  if ((i =
       static_cast<int>(word0(&u) >> Exp_shift1 & (Exp_mask >> Exp_shift1)))) {
    dval(&d2) = dval(&u);
    word0(&d2) &= Frac_mask1;
    word0(&d2) |= Exp_11;

    //  log(x)  ~=~ log(1.5) + (x-1.5)/1.5
    //  log10(x)   =  log(x) / log(10)
    //     ~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
    //  log10(d) = (i-Bias)*log(2)/log(10) + log10(d2)
    //
    //  This suggests computing an approximation k to log10(d) by
    //
    //  k = (i - Bias)*0.301029995663981
    //   + ( (d2-1.5)*0.289529654602168 + 0.176091259055681 );
    //
    //  We want k to be too large rather than too small.
    //  The error in the first-order Taylor series approximation
    //  is in our favor, so we just round up the constant enough
    //  to compensate for any error in the multiplication of
    //  (i - Bias) by 0.301029995663981; since |i - Bias| <= 1077,
    //  and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
    //  adding 1e-13 to the constant term more than suffices.
    //  Hence we adjust the constant term to 0.1760912590558.
    //  (We could get a more accurate k by invoking log10,
    //   but this is probably not worthwhile.)

    i -= Bias;
    denorm = 0;
  } else {
    // d is denormalized
    i = bbits + be + (Bias + (P - 1) - 1);
    const uint32_t x = (i > 32) ?
        (word0(&u) << (64 - i)) | (word1(&u) >> (i - 32)) :
        word1(&u) << (32 - i);
    dval(&d2) = x;
    word0(&d2) -= 31 * Exp_msk1;  // adjust exponent
    i -= (Bias + (P - 1) - 1) + 1;
    denorm = 1;
  }
  double ds = (dval(&d2) - 1.5) *
      0.289529654602168 + 0.1760912590558 + i * 0.301029995663981;
  int k = static_cast<int>(ds);
  if (ds < 0.0 && ds != k) {
    --k;  // want k = floor(ds)
  }
  int k_check = 1;
  if (k >= 0 && k <= static_cast<int>(kTens.size())) {
    if (dval(&u) < kTens[k]) {
      --k;
    }
    k_check = 0;
  }

  int j = bbits - i - 1;
  int b2, s2;
  if (j >= 0) {
    b2 = 0;
    s2 = j;
  } else {
    b2 = -j;
    s2 = 0;
  }

  int b5, s5;
  if (k >= 0) {
    b5 = 0;
    s5 = k;
    s2 += k;
  } else {
    b2 -= k;
    b5 = -k;
    s5 = 0;
  }

  int ilim = 0, ilim0, ilim1 = 0;
  if (RoundingNone) {
    ilim = ilim1 = -1;
    i = 18;
    ndigits = 0;
  }

  if (RoundingSignificantFigures) {
    if (ndigits <= 0) {
      ndigits = 1;
    }
    ilim = ilim1 = i = ndigits;
  }

  if (RoundingDecimalPlaces) {
    i = ndigits + k + 1;
    ilim = i;
    ilim1 = i - 1;
    if (i <= 0) {
      i = 1;
    }
  }

  BigInt S, mhi, mlo, delta;
  if (ilim >= 0 && ilim <= Quick_max) {
    // Try to get by with floating-point arithmetic.
    i = 0;
    dval(&d2) = dval(&u);
    const int k0 = k;
    ilim0 = ilim;
    ieps = 2;  // conservative
    if (k > 0) {
      ds = kTens[k & 0xf];
      j = k >> 4;
      if (j & Bletch) {
        // prevent overflows
        j &= Bletch - 1;
        dval(&u) /= kBigTens[kBigTens.size() - 1];
        ++ieps;
      }
      for (; j; j >>= 1, ++i) {
        if (j & 1) {
          ++ieps;
          ds *= kBigTens[i];
        }
      }
      dval(&u) /= ds;
    } else if ((j1 = -k)) {
      dval(&u) *= kTens[j1 & 0xf];
      for (j = j1 >> 4; j; j >>= 1, ++i) {
        if (j & 1) {
          ++ieps;
          dval(&u) *= kBigTens[i];
        }
      }
    }

    if (k_check && dval(&u) < 1.0 && ilim > 0) {
      if (ilim1 <= 0) {
        goto fastFailed;
      }
      ilim = ilim1;
      --k;
      dval(&u) *= 10.0;
      ++ieps;
    }
    dval(&eps) = (ieps * dval(&u)) + 7.0;
    word0(&eps) -= (P - 1) * Exp_msk1;
    if (!ilim) {
      S.clear();
      mhi.clear();
      dval(&u) -= 5.0;
      if (dval(&u) > dval(&eps)) {
        goto oneDigit;
      }
      if (dval(&u) < -dval(&eps)) {
        goto noDigits;
      }
      goto fastFailed;
    }

    if (LeftRight) {
      // Use Steele & White method of only
      // generating digits needed.
      dval(&eps) = (0.5 / kTens[ilim - 1]) - dval(&eps);
      for (i = 0;;) {
        L = static_cast<int32_t>(dval(&u));
        dval(&u) -= L;
        *s++ = '0' + static_cast<int>(L);
        if (dval(&u) < dval(&eps)) {
          goto ret;
        }
        if (1.0 - dval(&u) < dval(&eps)) {
          goto bumpUp;
        }
        if (++i >= ilim) {
          break;
        }
        dval(&eps) *= 10.0;
        dval(&u) *= 10.0;
      }
    } else {
      // Generate ilim digits, then fix them up.
      dval(&eps) *= kTens[ilim - 1];
      for (i = 1;; ++i, dval(&u) *= 10.0) {
        L = static_cast<int32_t>(dval(&u));
        if (!(dval(&u) -= L)) {
          ilim = i;
        }
        *s++ = '0' + static_cast<int>(L);
        if (i == ilim) {
          if (dval(&u) > 0.5 + dval(&eps)) {
            goto bumpUp;
          }
          if (dval(&u) < 0.5 - dval(&eps)) {
            while (*--s == '0') { }
            ++s;
            goto ret;
          }
          break;
        }
      }
    }

 fastFailed:
    s = s0;
    dval(&u) = dval(&d2);
    k = k0;
    ilim = ilim0;
  }

  //  Do we have a "small" integer?

  if (be >= 0 && k <= Int_max) {
    // Yes.
    ds = kTens[k];
    if (ndigits < 0 && ilim <= 0) {
      S.clear();
      mhi.clear();
      if (ilim < 0 || dval(&u) <= 5 * ds) {
        goto noDigits;
      }
      goto oneDigit;
    }
    for (i = 1;; ++i, dval(&u) *= 10.0) {
      L = static_cast<int32_t>(dval(&u) / ds);
      dval(&u) -= L * ds;
      *s++ = '0' + static_cast<int>(L);
      if (!dval(&u)) {
        break;
      }
      if (i == ilim) {
        dval(&u) += dval(&u);
        if (dval(&u) > ds || (dval(&u) == ds && (L & 1))) {
 bumpUp:
          while (*--s == '9') {
            if (s == s0) {
              ++k;
              *s = '0';
              break;
            }
          }
          ++*s++;
        }
        break;
      }
    }
    goto ret;
  }

  m2 = b2;
  m5 = b5;
  mhi.clear();
  mlo.clear();
  if (LeftRight) {
    i = denorm ? be + (Bias + (P - 1) - 1 + 1) : 1 + P - bbits;
    b2 += i;
    s2 += i;
    mhi.SetValue(1);
  }
  if (m2 > 0 && s2 > 0) {
    i = m2 < s2 ? m2 : s2;
    b2 -= i;
    m2 -= i;
    s2 -= i;
  }
  if (b5 > 0) {
    if (LeftRight) {
      if (m5 > 0) {
        mhi.Pow5Multi(m5);
        b *= mhi;
      }
      if ((j = b5 - m5)) {
        b.Pow5Multi(j);
      }
    } else {
      b.Pow5Multi(b5);
    }
  }

  S.SetValue(1);
  if (s5 > 0) {
    S.Pow5Multi(s5);
  }

  // Check for special case that d is a normalized power of 2.
  special_case = 0;
  if ((RoundingNone || LeftRight) && (!word1(&u) && !(word0(&u) & Bndry_mask) &&
                                      word0(&u) & (Exp_mask & ~Exp_msk1))) {
    // The special case
    b2 += Log2P;
    s2 += Log2P;
    special_case = 1;
  }

  // Arrange for convenient computation of quotients:
  // shift left if necessary so divisor has 4 leading 0 bits.
  //
  // Perhaps we should just compute leading 28 bits of S once
  // and for all and pass them and a shift to quorem, so it
  // can do shifts and ors to compute the numerator for q.
  if ((i = ((s5 ? 32 - Hi0Bits(S[S.size() - 1]) : 1) + s2) & 0x1f)) {
    i = 32 - i;
  }
  if (i > 4) {
    i -= 4;
    b2 += i;
    m2 += i;
    s2 += i;
  } else if (i < 4) {
    i += 28;
    b2 += i;
    m2 += i;
    s2 += i;
  }

  if (b2 > 0) {
    b <<= b2;
  }
  if (s2 > 0) {
    S <<= s2;
  }
  if (k_check) {
    if (b.Compare(S) < 0) {
      --k;
      b.MultiAdd(10, 0);  // we botched the k estimate
      if (LeftRight) {
        mhi.MultiAdd(10, 0);
      }
      ilim = ilim1;
    }
  }

  if (ilim <= 0 && RoundingDecimalPlaces) {
    // no digits, fcvt style
    if (ilim < 0) {
      goto noDigits;
    }
    S.MultiAdd(5, 0);

    if (b.Compare(S) < 0) {
      goto noDigits;
    }
    goto oneDigit;
  }

  if (LeftRight) {
    if (m2 > 0) {
      mhi <<= m2;
    }

    // Compute mlo -- check for special case
    // that d is a normalized power of 2.

    mlo = mhi;
    if (special_case) {
      mhi <<= Log2P;
    }

    for (i = 1;; ++i) {
      dig = Quorem(&b, &S) + '0';

      // Do we yet have the shortest decimal string
      // that will round to d?

      j = b.Compare(mlo);
      delta = S.Diff(mhi);
      j1 = delta.sign() ? 1 : b.Compare(delta);

#ifdef DTOA_ROUND_BIASED
      if (j < 0 || !j) {
#else
      if (!j1 && !(word1(&u) & 1)) {
        if (dig == '9') {
          goto round9up;
        }
        if (j > 0) {
          ++dig;
        }
        *s++ = dig;
        goto ret;
      }
      if (j < 0 || (!j && !(word1(&u) & 1))) {
#endif  // DTOA_ROUND_BIASED
        if ((b[0] || b.size() > 1) && (j1 > 0)) {
          b <<= 1;
          j1 = b.Compare(S);
          if ((j1 > 0 || (j1 == 0 && dig & 1)) && dig++ == '9') {
            goto round9up;
          }
        }
        *s++ = dig;
        goto ret;
      }
      if (j1 > 0) {
        if (dig== '9') {  // possible if i == 1
 round9up:
          *s++ = '9';
          goto roundoff;
        }
        *s++ = dig + 1;
        goto ret;
      }
      *s++ = dig;
      if (i == ilim) {
        break;
      }
      b.MultiAdd(10, 0);
      mlo.MultiAdd(10, 0);
      mhi.MultiAdd(10, 0);
    }
  } else {
    for (i = 1;; ++i) {
      *s++ = dig = Quorem(&b, &S) + '0';
      if (!b[0] && b.size() <= 1) {
        goto ret;
      }
      if (i >= ilim) {
        break;
      }
      b.MultiAdd(10, 0);
    }
  }

  // Round off last digit

  b <<= 1;
  j = b.Compare(S);
  if (j >= 0) {
 roundoff:
    while (*--s == '9') {
      if (s == s0) {
        ++k;
        *s++ = '1';
        goto ret;
      }
    }
    ++*s++;
  } else {
    while (*--s == '0') { }
    ++s;
  }
  goto ret;

 noDigits:
  *exponent_out = 0;
  *precision_out = 1;
  buf[0] = '0';
  buf[1] = '\0';
  return;

 oneDigit:
  *s++ = '1';
  ++k;
  goto ret;

 ret:
  assert(s > buf);
  *s = '\0';
  *exponent_out = k;
  *precision_out = s - buf;
}

template<typename Buffer>
void dtoa(Buffer* buffer, double dd,
          bool* sign, int* exp, unsigned* precision) {
  DoubleToASCII<true, false, false, true>(buffer, dd, 0, sign, exp, precision);
}

// DToA not accept NaN or Infinity
template<typename Derived, typename ResultType>
class DToA {
 public:

  ResultType Build(double x) {
    std::array<char, kDToABufferSize> buf;
    char* begin = buf.data() + 3;
    DoubleToASCII<true, false, false, true>(
        begin, x, 0, &sign_, &exponent_, &precision_);
    if (exponent_ >= -6 && exponent_ < 21) {
      return ToStringDecimal(buf.data(), begin);
    } else {
      return ToStringExponential(buf.data(), begin);
    }
  }

  ResultType BuildStandard(double x) {
    std::array<char, kDToABufferSize> buf;
    char* begin = buf.data() + 3;
    DoubleToASCII<true, false, false, true>(
        begin, x, 0, &sign_, &exponent_, &precision_);
    return ToStringDecimal(buf.data(), begin);
  }

  ResultType BuildExponential(double x, int frac, int offset) {
    std::array<char, kDToABufferSize> buf;
    const int number_digits = frac + offset;
    char* begin = buf.data() + 3;
    DoubleToASCII<false, true, false, false>(
        begin, x, number_digits, &sign_, &exponent_, &precision_);
    // Precision
    const unsigned figures = number_digits;
    while (precision_ < figures) {
      begin[precision_++] = '0';
    }
    return ToStringExponential(buf.data(), begin);
  }

  ResultType BuildPrecision(double x, int frac, int offset) {
    std::array<char, kDToABufferSize> buf;
    const int number_digits = frac + offset;
    char* begin = buf.data() + 3;
    DoubleToASCII<false, true, false, false>(
        begin, x, number_digits, &sign_, &exponent_, &precision_);
    // Precision
    const unsigned figures = number_digits;
    while (precision_ < figures) {
      begin[precision_++] = '0';
    }
    return ToStringDecimal(buf.data(), begin);
  }

  ResultType BuildStandardExponential(double x) {
    std::array<char, kDToABufferSize> buf;
    char* begin = buf.data() + 3;
    DoubleToASCII<true, false, false, true>(
        begin, x, 0, &sign_, &exponent_, &precision_);
    return ToStringExponential(buf.data(), begin);
  }

  ResultType BuildFixed(double x, int frac, int offset) {
    if (x >= 1e21 || x <= -1e21) {
      return Build(x);
    }
    std::array<char, kDToABufferSize> buf;
    const int number_digits = frac + offset;
    char* begin = buf.data() + 3;
    DoubleToASCII<false, false, true, false>(
        begin, x, number_digits, &sign_, &exponent_, &precision_);

    // Fixed
    unsigned digits = 1 + exponent_ + number_digits;
    while (precision_ < digits) {
      begin[precision_++] = '0';
    }
    return ToStringDecimal(buf.data(), begin);
  }

 private:
  ResultType ToStringDecimal(char* buffer_begin, char* begin) {
    if (exponent_ < 0) {
      const unsigned zeros = -exponent_ - 1;
      char* start = buffer_begin;
      if (sign_) {
        buffer_begin[0] = '-';
      } else {
        ++start;
      }
      buffer_begin[1] = '0';
      buffer_begin[2] = '.';
      if (zeros) {
        std::memmove(begin + zeros, begin, precision_);
      }
      std::fill_n(begin, zeros, '0');
      *(begin + zeros + precision_) = '\0';
      return static_cast<Derived*>(this)->Create(start);
    }

    const unsigned digits_before_decimal_point = exponent_ + 1;

    if (precision_ <= digits_before_decimal_point) {
      char* start = begin;
      if (sign_) {
        buffer_begin[2] = '-';
        start = begin - 1;
      }
      char* starting_zeros = begin + precision_;
      const unsigned zeros = digits_before_decimal_point - precision_;
      std::fill_n(starting_zeros, zeros, '0');
      *(starting_zeros + zeros) = '\0';
      return static_cast<Derived*>(this)->Create(start);
    } else {
      char* start = begin;
      if (sign_) {
        buffer_begin[2] = '-';
        start = begin - 1;
      }
      char* floating_part = begin + digits_before_decimal_point;
      const unsigned floating_digits = precision_ - digits_before_decimal_point;
      std::memmove(floating_part + 1, floating_part, floating_digits);
      *floating_part = '.';
      *(floating_part + 1 + floating_digits) = '\0';
      return static_cast<Derived*>(this)->Create(start);
    }
  }

  ResultType ToStringExponential(char* buffer_begin, char* begin) {
    char* start = buffer_begin + 1;
    if (sign_) {
      start[0] = '-';
    } else {
      ++start;
    }
    buffer_begin[2] = begin[0];
    char* exponent_part = begin;
    if (precision_ > 1) {
      begin[0] = '.';
      exponent_part = begin + precision_;
    }
    const int len = snprintf(exponent_part, 6, "e%+d", exponent_);  // NOLINT
    *(exponent_part + len) = '\0';
    return static_cast<Derived*>(this)->Create(start);
  }

  bool sign_;
  int exponent_;
  unsigned precision_;
};

class StringDToA : public DToA<StringDToA, std::string> {
 public:
  friend class DToA<StringDToA, std::string>;

 private:
  std::string Create(const char* str) const {
    return std::string(str);
  }
};

class StringPieceDToA : public DToA<StringPieceDToA, void> {
 public:
  friend class DToA<StringPieceDToA, void>;
  typedef std::array<char, kDToABufferSize> buffer_type;

  StringPiece buffer() const {
    return StringPiece(buffer_.data(), size_);
  }

 private:
  void Create(const char* str) {
    size_ = std::strlen(str);
    std::copy(str, str + size_, buffer_.begin());
  }

  buffer_type buffer_;
  std::size_t size_;
};

#undef Exp_shift
#undef Exp_shift1
#undef Exp_msk1
#undef Exp_msk11
#undef Exp_mask
#undef P
#undef Bias
#undef Emin
#undef Exp_1
#undef Exp_11
#undef Ebits
#undef Frac_mask
#undef Frac_mask1
#undef Bletch
#undef Bndry_mask
#undef Bndry_mask1
#undef LSB
#undef Sign_bit
#undef Log2P
#undef Tiny0
#undef Tiny1
#undef Quick_max
#undef Int_max

#undef rounded_product
#undef rounded_quotient

#undef Big0
#undef Big1

#undef word0
#undef word1
#undef dval

#undef Scale_Bit
} } }  // namespace iv::core::dtoa
#endif  // IV_DTOA_BIGINT_H_
