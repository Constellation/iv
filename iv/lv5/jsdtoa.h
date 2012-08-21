#ifndef IV_LV5_JSDTOA_H_
#define IV_LV5_JSDTOA_H_
#include <iv/dtoa.h>
#include <iv/platform_math.h>
#include <iv/lv5/jsstring.h>
namespace iv {
namespace lv5 {

class JSStringDToA : public core::dtoa::DToA<JSStringDToA, JSString*> {
 public:
  friend class core::dtoa::DToA<JSStringDToA, JSString*>;

  explicit JSStringDToA(Context* ctx)
    : ctx_(ctx) { }

 private:
  JSString* Create(const char* str) const {
    // Because result string length always smaller than JSString max length
    Error::Dummy dummy;
    return JSString::NewAsciiString(ctx_, str, &dummy);
  }

  Context* ctx_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSDTOA_H_
