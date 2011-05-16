#ifndef _IV_LV5_JSON_H_
#define _IV_LV5_JSON_H_
#include "ustringpiece.h"
#include "json_lexer.h"
#include "json_parser.h"
#include "json_stringifier.h"
namespace iv {
namespace lv5 {

template<bool AcceptLineTerminator>
inline JSVal ParseJSON(Context* ctx, const core::UStringPiece& str, Error* e) {
  JSONParser<core::UStringPiece, AcceptLineTerminator> parser(ctx, str);
  return parser.Parse(e);
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSON_H_
