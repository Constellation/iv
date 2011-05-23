#ifndef _IV_LV5_JSDTOA_H_
#define _IV_LV5_JSDTOA_H_
#include "dtoa.h"
#include "round.h"
#include "lv5/jsstring.h"
namespace iv {
namespace lv5 {

class JSStringDToA : public core::dtoa::DToA<JSStringDToA, JSString*> {
 public:
  friend class core::dtoa::DToA<JSStringDToA, JSString*>;

  explicit JSStringDToA(Context* ctx)
    : ctx_(ctx) { }

 private:
  JSString* Create(const char* str) const {
    return JSString::NewAsciiString(ctx_, str);
  }

  Context* ctx_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSDTOA_H_
