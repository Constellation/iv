#ifndef _IV_XORSHIFT_H_
#define _IV_XORSHIFT_H_
#include <ctime>
#include <tr1/cstdint>
#include <tr1/random>

namespace iv {
namespace core {

class Xor128 {
 public:
  typedef std::tr1::uint32_t result_type;
  Xor128()
    : x_(123456789),
      y_(362436069),
      z_(521266629),
      w_(88675123) {
    seed();
  }
  explicit Xor128(result_type x0)
    : x_(123456789),
      y_(362436069),
      z_(521266629),
      w_(88675123) {
    seed(x0);
  }
  static result_type min() {
    return limits::min();
  }
  static result_type max() {
    return limits::max();
  }
  void seed() {
    seed(static_cast<result_type>(std::time(NULL)));
  }
  void seed(result_type x0) {
    x_ ^= x0;
    y_ ^= RotateLeft<17>(x0);
    z_ ^= RotateLeft<31>(x0);
    w_ ^= RotateLeft<18>(x0);
  }
  void discard(unsigned long long count) {  // NOLINT
    for (unsigned long long i = 0; i < count; ++i) {  // NOLINT
      (*this)();
    }
  }
  result_type operator()() {
    const int_t t = x_ ^ (x_ << 11);
    x_ = y_;
    y_ = z_;
    z_ = w_;
    w_ = (w_ ^ (w_ >> 19)) ^ (t ^ (t >> 8));
    return w_;
  }
 private:
  typedef std::tr1::uint32_t int_t;
  typedef std::numeric_limits<result_type> limits;
  static const std::size_t kIntBits =
      limits::digits + (limits::is_signed ? 1 : 0);
  static const result_type kMask= limits::digits + (limits::is_signed ? 1 : 0);

  template<std::size_t N>
  static inline result_type RotateLeft(result_type x) {
    return (x << N) | (x >> (kIntBits - N));
  }

  int_t x_;
  int_t y_;
  int_t z_;
  int_t w_;
};

} }  // namespace iv::core
#endif  // _IV_XORSHIFT_H_
