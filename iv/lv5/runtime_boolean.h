#ifndef IV_LV5_RUNTIME_BOOLEAN_H_
#define IV_LV5_RUNTIME_BOOLEAN_H_
#include <iv/lv5/error_check.h>
#include <iv/lv5/constructor_check.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/error.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/jsstring.h>
namespace iv {
namespace lv5 {
namespace runtime {

// section 15.6.1.1 Boolean(value)
// section 15.6.2.1 new Boolean(value)
inline JSVal BooleanConstructor(const Arguments& args, Error* e) {
  if (args.IsConstructorCalled()) {
    bool res = false;
    if (!args.empty()) {
      res = args[0].ToBoolean();
    }
    return JSBooleanObject::New(args.ctx(), res);
  } else {
    return JSVal::Bool(!args.empty() && args.front().ToBoolean());
  }
}

// section 15.6.4.2 Boolean.prototype.toString()
inline JSVal BooleanToString(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Boolean.prototype.toString", args, e);
  const JSVal& obj = args.this_binding();
  bool b;
  if (!obj.IsBoolean()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Boolean>()) {
      b = static_cast<JSBooleanObject*>(obj.object())->value();
    } else {
      e->Report(Error::Type,
                "Boolean.prototype.toString is not generic function");
      return JSEmpty;
    }
  } else {
    b = obj.boolean();
  }
  return b ?
      args.ctx()->global_data()->string_true() :
      args.ctx()->global_data()->string_false();
}

// section 15.6.4.3 Boolean.prototype.valueOf()
inline JSVal BooleanValueOf(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("Boolean.prototype.valueOf", args, e);
  const JSVal& obj = args.this_binding();
  if (!obj.IsBoolean()) {
    if (obj.IsObject() && obj.object()->IsClass<Class::Boolean>()) {
      return JSVal::Bool(static_cast<JSBooleanObject*>(obj.object())->value());
    } else {
      e->Report(Error::Type,
                "Boolean.prototype.valueOf is not generic function");
      return JSEmpty;
    }
  } else {
    return JSVal::Bool(obj.boolean());
  }
}

} } }  // namespace iv::lv5::runtime
#endif  // IV_LV5_RUNTIME_STRING_H_
