#include "jsval.h"
namespace iv {
namespace lv5 {
#define VTABLE(TYPE)\
{\
    JSVal:: TYPE, \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::IsPrimitive, \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::TypeOf, \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::ToObject, \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::ToBoolean, \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::ToString, \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::ToNumber , \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::ToPrimitive , \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::CheckObjectCoercible, \
    &JSVal::vtable_initializer< \
      JSVal:: TYPE, boost::enable_if_c<true>::type>::IsCallable\
}

JSVal::vtable JSVal::vtables[JSVal::kVtables] = {
  VTABLE(NUMBER),
  VTABLE(OBJECT),
  VTABLE(STRING),
  VTABLE(BOOLEAN),
  VTABLE(NULLVALUE),
  VTABLE(UNDEFINED),
  VTABLE(REFERENCE),
  VTABLE(ENVIRONMENT),
};

#undef VTABLE

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
