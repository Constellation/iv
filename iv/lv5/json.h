#ifndef IV_LV5_JSON_H_
#define IV_LV5_JSON_H_
#include <iv/ustringpiece.h>
#include <iv/lv5/json_lexer.h>
#include <iv/lv5/json_parser.h>
#include <iv/lv5/json_stringifier.h>
namespace iv {
namespace lv5 {

template<bool AcceptLineTerminator>
inline JSVal ParseJSON(Context* ctx, const core::UStringPiece& str, Error* e) {
  JSONParser<core::UStringPiece, AcceptLineTerminator> parser(ctx, str);
  return parser.Parse(e);
}

template<bool AcceptLineTerminator>
inline JSVal ParseJSON(Context* ctx, const core::StringPiece& str, Error* e) {
  JSONParser<core::StringPiece, AcceptLineTerminator> parser(ctx, str);
  return parser.Parse(e);
}

class MaybeJSONParser : private core::Noncopyable<MaybeJSONParser> {
 public:
  MaybeJSONParser(JSString* str) : str_(str) { }

  bool IsParsable() {
    const FiberBase* fiber = str_->GetFiber();
    return ((*fiber)[0] == '(' && (*fiber)[fiber->size() - 1] == ')');
  }

  JSVal Parse(Context* ctx, Error* e) {
    if (str_->Is8Bit()) {
      const Fiber8* fiber = str_->Get8Bit();
      return ParseJSON<false>(
          ctx,
          core::StringPiece(fiber->data() + 1, fiber->size() - 2), e);
    } else {
      const Fiber16* fiber = str_->Get16Bit();
      return ParseJSON<false>(
          ctx,
          core::UStringPiece(fiber->data() + 1, fiber->size() - 2), e);
    }
  }

 private:
  JSString* str_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSON_H_
