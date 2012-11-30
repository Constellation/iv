#ifndef IV_LV5_JSDATE_H_
#define IV_LV5_JSDATE_H_
#include <iv/lv5/jsobject.h>
namespace iv {
namespace lv5 {

class JSDate : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSDate, Date)

  virtual JSVal DefaultValue(Context* ctx, Hint::Object hint, Error* e) {
    return JSObject::DefaultValue(
        ctx,
        (hint == Hint::NONE) ? Hint::STRING : hint, e);
  }


  static JSDate* New(Context* ctx, double val) {
    JSDate* const date = new JSDate(ctx, val);
    date->set_cls(JSDate::GetClass());
    return date;
  }


  static JSDate* NewPlain(Context* ctx, Map* map, double val) {
    return new JSDate(ctx, map, val);
  }

  double value() const { return value_; }

  void set_value(double val) { value_ = val; }

 private:
  JSDate(Context* ctx, double val)
    : JSObject(ctx->global_data()->date_map()),
      value_(val) {
  }

  JSDate(Context* ctx, Map* map, double val)
    : JSObject(map),
      value_(val) {
  }

  double value_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSDATE_H_
