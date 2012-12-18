#ifndef IV_LV5_JSMATH_H_
#define IV_LV5_JSMATH_H_
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

// only class holder
class JSMath : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSMath, Math)
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSMATH_H_
