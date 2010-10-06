#ifndef _IV_LV5_CLASS_H_
#define _IV_LV5_CLASS_H_
#include <string>
namespace iv {
namespace lv5 {
class JSObject;
class JSFunction;
struct Class {
  JSString* name;
  JSFunction* constructor;
  JSObject* prototype;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_CLASS_H_
