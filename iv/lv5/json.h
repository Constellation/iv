#ifndef IV_LV5_JSON_H_
#define IV_LV5_JSON_H_
#include <iv/string_view.h>
#include <iv/lv5/json_lexer.h>
#include <iv/lv5/json_parser.h>
#include <iv/lv5/json_stringifier.h>
namespace iv {
namespace lv5 {

template<bool AcceptLineTerminator>
inline JSVal ParseJSON(Context* ctx, const core::u16string_view& str, Error* e) {
  JSONParser<core::u16string_view, AcceptLineTerminator> parser(ctx, str);
  return parser.Parse(e);
}

template<bool AcceptLineTerminator>
inline JSVal ParseJSON(Context* ctx, const core::string_view& str, Error* e) {
  JSONParser<core::string_view, AcceptLineTerminator> parser(ctx, str);
  return parser.Parse(e);
}

class MaybeJSONParser : private core::Noncopyable<MaybeJSONParser> {
 public:
  explicit MaybeJSONParser(JSString* str) : str_(str) { }

  bool IsParsable() {
    return (str_->At(0) == '(' && str_->At(str_->size() - 1) == ')');
  }

  JSVal Parse(Context* ctx, Error* e) {
    if (str_->Is8Bit()) {
      const JSAsciiFlatString* fiber = str_->Flatten8();
      return ParseJSON<false>(
          ctx,
          core::string_view(*fiber).substr(1, fiber->size() - 2), e);
    } else {
      const JSUTF16FlatString* fiber = str_->Flatten16();
      return ParseJSON<false>(
          ctx,
          core::u16string_view(*fiber).substr(1, fiber->size() - 2), e);
    }
  }

 private:
  JSString* str_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSON_H_
