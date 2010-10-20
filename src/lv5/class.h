#ifndef _IV_LV5_CLASS_H_
#define _IV_LV5_CLASS_H_
namespace iv {
namespace lv5 {

class JSObject;
class JSFunction;
class JSString;

struct Class {
  JSString* name;
  JSFunction* constructor;
  JSObject* prototype;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_CLASS_H_
