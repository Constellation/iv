#ifndef IV_LV5_UTILITY_H_
#define IV_LV5_UTILITY_H_
#include <iv/platform_math.h>
namespace iv {
namespace lv5 {

struct JSDoubleEquals {
  bool operator()(double lhs, double rhs) const {
    return lhs == rhs && (core::math::Signbit(lhs)) == core::math::Signbit(rhs);
  }
};

inline void Inspect(Context* ctx, JSVal val, Error* e) {
  const core::UString str = val.ToUString(ctx, IV_LV5_ERROR_VOID(e));
  core::unicode::FPutsUTF16(stdout, str);
  std::putchar('\n');
}

} }  // namespace iv::lv5
#endif  // IV_LV5_UTILITY_H_
