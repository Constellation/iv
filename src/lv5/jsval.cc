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

JSVal::JSVal(JSTrueKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSFalseKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSNullKeywordType val)
  : value_() {
  set_value(val);
}

JSVal::JSVal(JSUndefinedKeywordType val)
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

} }  // namespace iv::lv5
