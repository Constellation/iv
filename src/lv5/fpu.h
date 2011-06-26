#ifndef _IV_LV5_FPU_H_
#define _IV_LV5_FPU_H_
#include "noncopyable.h"
#if (!defined(IV_USE_SSE) && defined(__GNUC__) && defined(__i386__) && !defined(__CYGWIN__))
#include <fpu_control.h>
namespace iv {
namespace lv5 {

// i386 GNUC++'s double precision is extended
// so fix to double
// see https://bugzilla.mozilla.org/show_bug.cgi?id=264912
//     http://www.vinc17.org/research/extended.en.html
// 0x127f ?
class FPU : private core::Noncopyable<> {
 public:
  FPU() : prev_() {
    _FPU_GETCW(prev_);
    fpu_control_t cw = (prev_ & ~0x300) | _FPU_DOUBLE;
    _FPU_SETCW(cw);
  }

  ~FPU() {
    _FPU_SETCW(prev_);
  }

 private:
  fpu_control_t prev_;
};

} }  // namespace iv::lv5
#else
namespace iv {
namespace lv5 {

class FPU : private core::Noncopyable<> {
};

} }  // namespace iv::lv5
#endif
#endif  // _IV_LV5_FPU_H_
