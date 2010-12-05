#ifndef _IV_LV5_JSDATE_H_
#define _IV_LV5_JSDATE_H_
#include "jsobject.h"

namespace iv {
namespace lv5 {

class Context;

class JSDate : public JSObject {
 public:
  JSDate(Context* ctx, double val);

  JSVal DefaultValue(Context* ctx,
                     Hint::Object hint, Error* res);

  static JSDate* New(Context* ctx, double val);

  const double& value() const {
    return value_;
  }

 private:
  double value_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSDATE_H_
