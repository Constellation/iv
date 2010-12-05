#include "jsdate.h"
#include "context.h"

namespace iv {
namespace lv5 {

JSDate::JSDate(Context* ctx, double val)
  : value_(val) {
}

JSDate* JSDate::New(Context* ctx, double val) {
  JSDate* const date = new JSDate(ctx, val);
  const Class& cls = ctx->Cls("Date");
  date->set_cls(cls.name);
  date->set_prototype(cls.prototype);
  return date;
}

JSVal JSDate::DefaultValue(Context* ctx,
                           Hint::Object hint, Error* res) {
  return JSObject::DefaultValue(
      ctx,
      (hint == Hint::NONE) ? Hint::STRING : hint, res);
}

} }  // namespace iv::lv5
