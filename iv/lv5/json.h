#ifndef IV_LV5_JSON_H_
#define IV_LV5_JSON_H_
#include <iv/stringpiece.h>
#include <iv/lv5/json_lexer.h>
#include <iv/lv5/json_parser.h>
#include <iv/lv5/json_stringifier.h>
namespace iv {
namespace lv5 {

template<bool AcceptLineTerminator>
inline JSVal ParseJSON(Context* ctx, const core::U16StringPiece& str, Error* e) {
  JSONParser<core::U16StringPiece, AcceptLineTerminator> parser(ctx, str);
  return parser.Parse(e);
}

template<bool AcceptLineTerminator>
inline JSVal ParseJSON(Context* ctx, const core::StringPiece& str, Error* e) {
  JSONParser<core::StringPiece, AcceptLineTerminator> parser(ctx, str);
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
      const Fiber8* fiber = str_->Get8Bit();
      return ParseJSON<false>(
          ctx,
          core::StringPiece(*fiber).substr(1, fiber->size() - 2), e);
    } else {
      const Fiber16* fiber = str_->Get16Bit();
      return ParseJSON<false>(
          ctx,
          core::U16StringPiece(*fiber).substr(1, fiber->size() - 2), e);
    }
  }

 private:
  JSString* str_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSON_H_
