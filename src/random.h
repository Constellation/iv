// @file
// @brief normal random generator
//
// Copyright (C) 2010 Cybozu Inc., all rights reserved.
// @author MITSUNARI Shigeo
#ifndef IV_RANDOM_H_
#define IV_RANDOM_H_
#include "xorshift.h"
namespace iv {
namespace core {

// normal random generator
template<typename EngineType>
class NormalRandomGenerator {
 public:
  NormalRandomGenerator(double u = 0, double s = 1, int seed = 0)
    : engine_(seed),
      u_(u),
      s_(s) {
    engine_.discard(20);
  }

  void init(int seed = 0) {
    engine_.seed(seed);
    engine_.discard(20);
  }

  double get() {
    double sum = -6;
    for (int i = 0; i < 12; i++) {
      sum += engine_() / static_cast<double>(1ULL << 32);
    }
    return sum * s_ + u_;
  }

 private:
  EngineType engine_;
  double u_;
  double s_;
};

// uniform random generator
// reference
//   http://www.mathworks.com/matlabcentral/answers/13216-random-number-generation
//   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/faq.html
template<typename EngineType>
class UniformRandomGenerator {
 public:
  UniformRandomGenerator(double min = 0.0, double max = 1.0, int seed = 0)
    : engine_(seed),
      min_(min),
      max_(max) {
    engine_.discard(20);
  }

  void init(int seed = 0) {
    engine_.seed(seed);
    engine_.discard(20);
  }

  // returns random number in range [0.0, 1.0)
  double get() {
    // double mantissa => 53bits (including economized form)
    //
    // a is 27bits, b is 26bits
    // 67108844 is 1 << 26
    // make 53bits by a << 26 | b
    // 9007199254740992.0 is 1 << 53
    //
    // (a * 67108864.0 + b) * (1.0 / 9007199254740992.0) is [0.0, 1.0)
    const uint32_t a = engine_() >> 5;
    const uint32_t b = engine_() >> 6;
    return
        (a * 67108864.0 + b) *
        (1.0 / 9007199254740992.0) * (max_ - min_) + min_;
  }

 private:
  EngineType engine_;
  double min_;
  double max_;
};

template<typename EngineType, typename IntType = int>
class Uniform32RandomGenerator {
 public:
  Uniform32RandomGenerator(IntType min = 0, IntType max = 10, int seed = 0)
    : engine_(seed),
      min_(min),
      max_(max) {
    engine_.discard(20);
  }

  void init(int seed = 0) {
    engine_.seed(seed);
    engine_.discard(20);
  }

  IntType get() {
    return (engine_() % (max_ - min_ + 1)) + min_;
  }

 private:
  EngineType engine_;
  const IntType min_;
  const IntType max_;
};

} }  // namespace iv::core
#endif  // IV_RANDOM_H_
