#ifndef IV_LV5_JSREFLECT_H_
#define IV_LV5_JSREFLECT_H_
#include <iv/lv5/jsobject_fwd.h>
namespace iv {
namespace lv5 {

// only class holder
class JSReflect : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSReflect, Reflect)
};


} }  // namespace iv::lv5
#endif  // IV_LV5_JSREFLECT_H_
