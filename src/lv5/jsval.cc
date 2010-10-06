#include "jsval.h"
namespace iv {
namespace lv5 {
JSVal::JSVal()
  : vptr_(NULL),
    value_() {
  set_undefined();
}

JSVal::JSVal(const double& val)
  : vptr_(NULL),
    value_() {
  set_value(val);
}

JSVal::JSVal(JSObject* val)
  : vptr_(NULL),
    value_() {
  set_value(val);
}

JSVal::JSVal(JSString* val)
  : vptr_(NULL),
    value_() {
  set_value(val);
}

JSVal::JSVal(JSReference* val)
  : vptr_(NULL),
    value_() {
  set_value(val);
}

JSVal::JSVal(JSEnv* val)
  : vptr_(NULL),
    value_() {
  set_value(val);
}

JSVal::JSVal(bool val)
  : vptr_(NULL),
    value_() {
  set_value(val);
}

JSVal::JSVal(const this_type& rhs)
  : vptr_(rhs.vptr_),
    value_(rhs.value_) {
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
  return JSVal(val);
}

JSVal JSVal::Number(double val) {
  return JSVal(val);
}

JSVal JSVal::String(JSString* str) {
  return JSVal(str);
}

JSVal JSVal::Object(JSObject* obj) {
  return JSVal(obj);
}

} }  // namespace iv::lv5
