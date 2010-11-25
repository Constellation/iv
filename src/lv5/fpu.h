#ifndef _IV_LV5_FPU_H_
#define _IV_LV5_FPU_H_
#if (defined __GNUC__ && defined __i386__)
#include <fpu_control.h>
namespace iv {
namespace lv5 {

// i386 GNUC++'s double precision is extended
// so fix to double
// see http://code.google.com/p/v8/issues/detail?id=144
//     https://bugzilla.mozilla.org/show_bug.cgi?id=264912
inline void FixFPU() {
  fpu_control_t cw = 0x127f;
  _FPU_SETCW(cw);
}

} }  // namespace iv::lv5
#else
namespace iv {
namespace lv5 {

inline void FixFPU() {
}

} }  // namespace iv::lv5
#endif

#endif  // _IV_LV5_LV5_H_
