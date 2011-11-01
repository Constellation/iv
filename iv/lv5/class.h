#ifndef IV_LV5_CLASS_H_
#define IV_LV5_CLASS_H_
#include <iv/none.h>
#include <iv/lv5/symbol.h>
namespace iv {
namespace lv5 {

class JSObject;
class JSFunction;
class JSString;

#define IV_JS_CLASS_LIST(V)\
  V(Object, 0)\
  V(Function, 1)\
  V(Array, 2)\
  V(Date, 3)\
  V(String, 4)\
  V(Boolean, 5)\
  V(Number, 6)\
  V(RegExp, 7)\
  V(Math, 8)\
  V(JSON, 9)\
  V(Error, 10)\
  V(EvalError, 11)\
  V(RangeError, 12)\
  V(ReferenceError, 13)\
  V(SyntaxError, 14)\
  V(TypeError, 15)\
  V(URIError, 16)\
  V(global, 17)\
  V(Arguments, 18)\
  V(NOT_CACHED, 19)

struct Class {
  enum JSClassType {
#define V(name, num) name = num,
    IV_JS_CLASS_LIST(V)
#undef V
    NUM_OF_CLASS
  };
  const char* name;
  JSClassType type;
};

struct ClassSlot {
  const Class* cls;
  Symbol name;
  JSString* name_string;
  JSFunction* constructor;
  JSObject* prototype;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_CLASS_H_
