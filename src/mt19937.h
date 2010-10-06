// Copyright (c) Constellation <utatane.tea@gmail.com> All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Algorithm Lisence
// Mersenne Twister 19937
// BSD Lisence
// Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
// All rights reserved.
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/mt.html

#ifndef _IV_MT19937_H_
#define _IV_MT19937_H_
#include <tr1/cstdint>
#include <vector>

namespace iv {
namespace core {

class MT19937 {
 public:
  explicit MT19937(uint32_t s)
     : mti_(N+1) {
    InitGenRand(s);
  }

  MT19937(const uint32_t* vec, int len)
     : mti_(N+1) {
    InitByArray(vec, len);
  }

  explicit MT19937(const std::vector<uint32_t>& vec)
     : mti_(N+1) {
    InitByArray(vec.begin(), vec.size());
  }

  uint32_t GenInt32() {
    uint32_t y;
    if (mti_ >= N) {
      int kk;
      for (kk = 0; kk < N-M; ++kk) {
        y = (mt_[kk] & UPPER_MASK) | (mt_[kk+1] & LOWER_MASK);
        mt_[kk] = mt_[kk+M] ^ (y >> 1) ^ ((y & 0x1UL) ? MATRIX_A : 0x0UL);
      }
      for (; kk < N-1; ++kk) {
        y = (mt_[kk] & UPPER_MASK) | (mt_[kk+1] & LOWER_MASK);
        mt_[kk] = mt_[kk+(M-N)] ^ (y >> 1) ^ ((y & 0x1UL) ? MATRIX_A : 0x0UL);
      }
      y = (mt_[N-1] & UPPER_MASK) | (mt_[0] & LOWER_MASK);
      mt_[N-1] = mt_[M-1] ^ (y >> 1) ^ ((y & 0x1UL) ? MATRIX_A : 0x0UL);
      mti_ = 0;
    }

    y = mt_[mti_++];

    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);
    return y;
  }

  uint32_t operator()() {
    return GenInt32();
  }

  int32_t GenInt31() {
    return static_cast<int32_t>(GenInt32() >> 1);
  }

  double GenReal1() {
    return GenInt32() * (1.0 / 4294967295.0);
  }

  double GenReal2() {
    return GenInt32() * (1.0 / 4294967296.0);
  }

  double GenReal3() {
    return (static_cast<double>(GenInt32()) + 0.5) * (1.0 / 4294967296.0);
  }

  double GenReal53() {
    const uint32_t a = GenInt32() >> 5;
    const uint32_t b = GenInt32() >> 6;
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
  }

 private:
  void InitGenRand(uint32_t s) {
    mt_[0] = s & 0xffffffffUL;
    for (mti_ = 1; mti_ < N; ++mti_) {
      mt_[mti_] = (1812433253UL * (mt_[mti_-1] ^ (mt_[mti_-1] >> 30)) + mti_);
      mt_[mti_] &= 0xffffffffUL;
    }
  }

  template<typename I>
  void InitByArray(const I vec, const int len) {
    InitGenRand(19650218UL);
    int i = 1, j = 0;
    for (int k = (N > len ? N : len); k; k--) {
      mt_[i] = (mt_[i] ^ ((mt_[i-1] ^ (mt_[i-1] >> 30)) * 1664525UL))
        + *(vec+j) + j; /* non linear */
      mt_[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
      ++i;
      ++j;
      if (i >= N) {
        mt_[0] = mt_[N-1];
        i = 1;
      }
      if (j >= len) {
        j = 0;
      }
    }
    for (int k = N-1; k; k--) {
      mt_[i] = (mt_[i] ^ ((mt_[i-1] ^ (mt_[i-1] >> 30)) * 1566083941UL))
        - i;  // non linear
      mt_[i] &= 0xffffffffUL;  // for WORDSIZE > 32 machines
      i++;
      if (i >= N) {
        mt_[0] = mt_[N-1];
        i = 1;
      }
    }
    mt_[0] = 0x80000000UL;  // MSB is 1; assuring non-zero initial array
  }

  static const int N = 624;
  static const int M = 397;
  static const uint32_t MATRIX_A = 0x9908b0dfUL;    // constant vector a
  static const uint32_t UPPER_MASK = 0x80000000UL;  // most significant w-r bits
  static const uint32_t LOWER_MASK = 0x7fffffffUL;  // least significant r bits

  uint32_t mt_[N];
  int mti_;
};

template<typename T>
class UniformIntDistribution {
 public:
  UniformIntDistribution(T min, T max)
    : min_(min),
      max_(max) {
  }
  template<typename EngineType>
  T operator()(EngineType& engine) {  // NOLINT
    return engine() % (max_ - min_ + 1) + min_;
  }
 private:
  const T min_;
  const T max_;
};

template<typename T>
class UniformRealDistribution {
 public:
  UniformRealDistribution(T min, T max)
    : min_(min),
      max_(max) {
  }
  template<typename EngineType>
  T operator()(EngineType& engine) {  // NOLINT
    const uint32_t a = engine() >> 5;
    const uint32_t b = engine() >> 6;
    return (a * 67108864.0 + b)
        * (1.0 / 9007199254740992.0) * (max_ - min_) + min_;
  }
 private:
  const T min_;
  const T max_;
};

} }  // namespace iv::core
#endif  // _IV_MT19937_H_
