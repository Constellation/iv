#include "jsval.h"
namespace iv {
namespace lv5 {

JSVal::JSVal()
  : value_() {
  set_undefined();
}

JSVal::JSVal(const double& val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSObject* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSString* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSReference* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSEnv* val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(bool val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(const this_type& rhs)
  : value_(rhs.value_) {
}

JSVal& JSVal::operator=(const this_type& rhs) {
  if (this != &rhs) {
    this_type(rhs).swap(*this);
  }
  return *this;
}

JSVal JSVal::Undefined() {
  return JSVal();
}

JSVal JSVal::Null() {
  JSVal v;
  v.set_null();
  return v;
}

JSVal JSVal::Boolean(bool val) {
  return val;
}

JSVal JSVal::Number(double val) {
  return val;
}

JSVal JSVal::String(JSString* str) {
  return str;
}

JSVal JSVal::Object(JSObject* obj) {
  return obj;
}

} }  // namespace iv::lv5
