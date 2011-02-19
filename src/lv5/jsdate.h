#ifndef _IV_LV5_JSDATE_H_
#define _IV_LV5_JSDATE_H_
#include "lv5/jsobject.h"
#include "lv5/context_utils.h"

namespace iv {
namespace lv5 {

class Context;

class JSDate : public JSObject {
 public:
  JSDate(Context* ctx, double val)
    : value_(val) {
  }


  JSVal DefaultValue(Context* ctx,
                     Hint::Object hint, Error* res) {
    return JSObject::DefaultValue(
        ctx,
        (hint == Hint::NONE) ? Hint::STRING : hint, res);
  }


  static JSDate* New(Context* ctx, double val) {
    JSDate* const date = new JSDate(ctx, val);
    const Class& cls = context::Cls(ctx, "Date");
    date->set_class_name(cls.name);
    date->set_prototype(cls.prototype);
    return date;
  }


  static JSDate* NewPlain(Context* ctx, double val) {
    return new JSDate(ctx, val);
  }


  const double& value() const {
    return value_;
  }


  void set_value(double val) {
    value_ = val;
  }


 private:
  double value_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSDATE_H_
