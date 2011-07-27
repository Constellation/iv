#ifndef _IV_LV5_JSON_PARSER_H_
#define _IV_LV5_JSON_PARSER_H_
#include <cassert>
#include <cstdlib>
#include <vector>
#include "token.h"
#include "noncopyable.h"
#include "conversions.h"
#include "lv5/property.h"
#include "lv5/jsval.h"
#include "lv5/jsarray.h"
#include "lv5/jsstring.h"
#include "lv5/jsobject.h"
#include "lv5/context.h"
namespace iv {
namespace lv5 {

#define CHECK  e);\
  if (*e) {\
    return JSUndefined;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define IS(token)\
  do {\
    if (token_ != token) {\
      e->Report(Error::Syntax, "invalid JSON");\
      return JSUndefined;\
    }\
  } while (0)

#define EXPECT(token)\
  do {\
    if (token_ != token) {\
      e->Report(Error::Syntax, "invalid JSON");\
      return JSUndefined;\
    }\
    Next();\
  } while (0)

#define RAISE()\
  do {\
    e->Report(Error::Syntax, "invalid JSON");\
    return JSUndefined;\
  } while (0)

template<typename Source, bool AcceptLineTerminator = true>
class JSONParser : private core::Noncopyable<> {
 public:
  typedef JSONLexer<Source, AcceptLineTerminator> lexer_type;
  typedef core::Token Token;

  JSONParser(Context* ctx, const Source& src)
    : ctx_(ctx),
      lexer_(src),
      token_() {
  }

  JSVal Parse(Error* e) {
    Next();
    const JSVal result = ParseJSONValue(CHECK);
    IS(Token::TK_EOS);
    return result;
  }

 private:

  Token::Type Next() {
    return token_ = lexer_.Next();
  }

  JSVal ParseJSONValue(Error* e) {
    switch (token_) {
      case Token::TK_NULL_LITERAL:
        Next();
        return JSNull;

      case Token::TK_TRUE_LITERAL:
        Next();
        return JSTrue;

      case Token::TK_FALSE_LITERAL:
        Next();
        return JSFalse;

      case Token::TK_LBRACE:
        return ParseJSONObject(CHECK);

      case Token::TK_LBRACK:
        return ParseJSONArray(CHECK);

      case Token::TK_STRING:
        return ParseJSONString();

      case Token::TK_NUMBER:
        return ParseJSONNumber();

      default:
        RAISE();
    }
  }

  JSVal ParseJSONObject(Error* e) {
    assert(token_ == Token::TK_LBRACE);
    Next();
    JSObject* const obj = JSObject::New(ctx_);
    bool trailing_comma = false;
    while (token_ != Token::TK_RBRACE) {
      IS(Token::TK_STRING);
      const Symbol key = ParseSymbol(lexer_.Buffer());
      EXPECT(Token::TK_COLON);
      const JSVal target = ParseJSONValue(CHECK);
      obj->DefineOwnProperty(
          ctx_, key,
          DataDescriptor(target,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, CHECK);
      if (token_ != Token::TK_RBRACE) {
        EXPECT(Token::TK_COMMA);
        trailing_comma = true;
      } else {
        trailing_comma = false;
      }
    }
    assert(token_ == Token::TK_RBRACE);
    if (trailing_comma) {
      RAISE();
    }
    Next();
    return obj;
  }

  JSVal ParseJSONArray(Error* e) {
    assert(token_ == Token::TK_LBRACK);
    JSArray* const ary = JSArray::New(ctx_);
    uint32_t current = 0;
    Next();
    bool trailing_comma = false;
    while (token_ != Token::TK_RBRACK) {
      const JSVal target = ParseJSONValue(CHECK);
      ary->DefineOwnProperty(
          ctx_, symbol::MakeSymbolFromIndex(current),
          DataDescriptor(target, PropertyDescriptor::WRITABLE |
                                 PropertyDescriptor::ENUMERABLE |
                                 PropertyDescriptor::CONFIGURABLE),
          false, CHECK);
      if (token_ != Token::TK_RBRACK) {
        EXPECT(Token::TK_COMMA);
        trailing_comma = true;
      } else {
        trailing_comma = false;
      }
      ++current;
    }
    assert(token_ == Token::TK_RBRACK);
    if (trailing_comma) {
      RAISE();
    }
    Next();
    ary->Put(ctx_, symbol::length(),
             JSVal::UInt32(current), false, CHECK);
    return ary;
  }

  JSVal ParseJSONString() {
    assert(token_ == Token::TK_STRING);
    const std::vector<uint16_t>& vec = lexer_.Buffer();
    JSString* string = JSString::New(ctx_, vec.begin(), vec.end());
    Next();
    return string;
  }

  JSVal ParseJSONNumber() {
    assert(token_ == Token::TK_NUMBER);
    const double number = lexer_.Numeric();
    Next();
    return number;
  }

  Symbol ParseSymbol(const std::vector<uint16_t>& range) {
    const Symbol res = context::Intern(ctx_, core::UStringPiece(range.data(), range.size()));
    Next();
    return res;
  }

  Context* ctx_;
  lexer_type lexer_;
  Token::Type token_;
};

#undef CHECK
#undef IS
#undef EXPECT
#undef UNEXPECT
#undef RAISE
} }  // namespace iv::lv5
#endif  // _IV_LV5_JSON_PARSER_H_
