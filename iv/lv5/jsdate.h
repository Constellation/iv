#ifndef IV_LV5_JSDATE_H_
#define IV_LV5_JSDATE_H_
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/jsval_fwd.h>
namespace iv {
namespace lv5 {

class JSDate : public JSObject {
 public:
  IV_LV5_DEFINE_JSCLASS(JSDate, Date)

  IV_LV5_INTERNAL_METHOD JSVal DefaultValueMethod(JSObject* obj, Context* ctx, Hint::Object hint, Error* e) {
    return JSObject::DefaultValueMethod(obj, ctx, (hint == Hint::NONE) ? Hint::STRING : hint, e);
  }

  static JSDate* New(Context* ctx, double val) {
    JSDate* const date = new JSDate(ctx, val);
    date->set_cls(JSDate::GetClass());
    return date;
  }

  static JSDate* NewPlain(Context* ctx, Map* map, double val) {
    return new JSDate(ctx, map, val);
  }

  double value() const { return utc_.value(); }

  double timezone() const { return timezone_; }

  void set_value(double utc) {
    utc_.SetValue(utc);
    const double local = core::date::LocalTime(utc);
    local_.SetValue(local);
    timezone_ = (utc - local) / core::date::kMsPerMinute;
  }

  const core::date::DateInstance& local() const { return local_; }
  const core::date::DateInstance& utc() const { return utc_; }

 private:
  JSDate(Context* ctx, double val)
    : JSObject(ctx->global_data()->date_map()) {
    set_value(val);
  }

  JSDate(Context* ctx, Map* map, double val)
    : JSObject(map) {
    set_value(val);
  }

  core::date::DateInstance local_;
  core::date::DateInstance utc_;
  double timezone_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSDATE_H_
