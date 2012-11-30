#ifndef IV_LV5_CLASS_H_
#define IV_LV5_CLASS_H_
#include <iv/none.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/enumeration_mode.h>
#include <iv/lv5/property_fwd.h>
#include <iv/lv5/method_table.h>
namespace iv {
namespace lv5 {

class Error;
class JSObject;
class JSFunction;
class JSString;
class PropertyNamesCollector;

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
  V(Set, 19)\
  /* i18n */\
  V(Collator, 20)\
  V(NumberFormat, 21)\
  V(DateTimeFormat, 22)\
  V(Name, 23)\
  /* iterator */\
  V(Iterator, 24)\
  /* Binary Blocks */\
  V(ArrayBuffer, 25)\
  V(DataView, 26)\
  V(Int8Array, 27)\
  V(Uint8Array, 28)\
  V(Int16Array, 29)\
  V(Uint16Array, 30)\
  V(Int32Array, 31)\
  V(Uint32Array, 32)\
  V(Float32Array, 33)\
  V(Float64Array, 34)\
  V(Uint8ClampedArray, 35)\
  V(NOT_CACHED, 36)

struct Class {
  enum JSClassType {
#define V(name, num) name = num,
    IV_JS_CLASS_LIST(V)
#undef V
    NUM_OF_CLASS
  };
  const char* name;
  uint32_t type;
  MethodTable* method;
};

struct ClassSlot {
  const Class* cls;
  Symbol name;
  JSString* name_string;
  JSFunction* constructor;
  JSObject* prototype;
};

#define IV_LV5_DEFINE_JSCLASS(CLASS, name)\
  static const Class* GetClass() {\
    static const Class cls = {\
      #name,\
      Class::name,\
      /*IV_LV5_METHOD_TABLE(CLASS)*/\
    };\
    return &cls;\
  }

} }  // namespace iv::lv5
#endif  // IV_LV5_CLASS_H_
