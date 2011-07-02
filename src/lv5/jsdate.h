#ifndef _IV_LV5_JSDATE_H_
#define _IV_LV5_JSDATE_H_
#include "lv5/jsobject.h"
#include "lv5/context_utils.h"

namespace iv {
namespace lv5 {

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
    date->set_cls(JSDate::GetClass());
    date->set_prototype(context::GetClassSlot(ctx, Class::Date).prototype);
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

  static const Class* GetClass() {
    static const Class cls = {
      "Date",
      Class::Date
    };
    return &cls;
  }

 private:
  double value_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSDATE_H_
