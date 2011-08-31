#ifndef IV_LV5_JSMATH_H_
#define IV_LV5_JSMATH_H_
#include "lv5/jsobject.h"
#include "lv5/class.h"
namespace iv {
namespace lv5 {

// only class holder
class JSMath : public JSObject {
 public:
  static const Class* GetClass() {
    static const Class cls = {
      "Math",
      Class::Math
    };
    return &cls;
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSMATH_H_
