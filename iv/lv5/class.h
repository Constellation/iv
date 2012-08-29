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
  V(Map, 19)\
  V(Set, 20)\
  /* i18n */\
  V(Collator, 21)\
  V(NumberFormat, 22)\
  V(DateTimeFormat, 23)\
  V(Name, 24)\
  /* added by lv5 */\
  V(Iterator, 25)\
  V(NOT_CACHED, 26)

struct Class {
  enum JSClassType {
#define V(name, num) name = num,
    IV_JS_CLASS_LIST(V)
#undef V
    NUM_OF_CLASS
  };
  const char* name;
  uint32_t type;
};

struct ClassSlot {
  const Class* cls;
  Symbol name;
  JSString* name_string;
  JSFunction* constructor;
  JSObject* prototype;
};

#define IV_LV5_DEFINE_JSCLASS(name)\
  static const Class* GetClass() {\
    static const Class cls = {\
      #name,\
      Class::name\
    };\
    return &cls;\
  }

} }  // namespace iv::lv5
#endif  // IV_LV5_CLASS_H_
