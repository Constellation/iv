#ifndef _IV_LV5_JSON_PARSER_H_
#define _IV_LV5_JSON_PARSER_H_
#include <cassert>
#include <cstdlib>
#include <vector>
#include "uchar.h"
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

template<typename Source>
class JSONParser : private core::Noncopyable<JSONParser<Source> >::type {
 public:
  typedef JSONLexer<Source> lexer_type;
  typedef core::Token Token;

  explicit JSONParser(Context* ctx,
                      const Source& src)
    : ctx_(ctx),
      lexer_(src),
      token_() {
  }

  JSVal Parse(Error* e) {
    Next();
    const JSVal result = ParseJSONValue(CHECK);
    IS(Token::EOS);
    return result;
  }
 private:

  Token::Type Next() {
    return token_ = lexer_.Next();
  }

  JSVal ParseJSONValue(Error* e) {
    switch (token_) {
      case Token::NULL_LITERAL:
        Next();
        return JSNull;

      case Token::TRUE_LITERAL:
        Next();
        return JSTrue;

      case Token::FALSE_LITERAL:
        Next();
        return JSFalse;

      case Token::LBRACE:
        return ParseJSONObject(CHECK);

      case Token::LBRACK:
        return ParseJSONArray(CHECK);

      case Token::STRING:
        return ParseJSONString();

      case Token::NUMBER:
        return ParseJSONNumber();

      default:
        RAISE();
    }
  }

  JSVal ParseJSONObject(Error* e) {
    assert(token_ == Token::LBRACE);
    Next();
    JSObject* const obj = JSObject::New(ctx_);
    bool trailing_comma = false;
    while (token_ != Token::RBRACE) {
      IS(Token::STRING);
      const Symbol key = ParseSymbol(lexer_.Buffer());
      EXPECT(Token::COLON);
      const JSVal target = ParseJSONValue(CHECK);
      obj->DefineOwnProperty(
          ctx_, key,
          DataDescriptor(target,
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, CHECK);
      if (token_ != Token::RBRACE) {
        EXPECT(Token::COMMA);
        trailing_comma = true;
      } else {
        trailing_comma = false;
      }
    }
    assert(token_ == Token::RBRACE);
    if (trailing_comma) {
      RAISE();
    }
    Next();
    return obj;
  }

  JSVal ParseJSONArray(Error* e) {
    assert(token_ == Token::LBRACK);
    JSArray* const ary = JSArray::New(ctx_);
    uint32_t current = 0;
    Next();
    bool trailing_comma = false;
    while (token_ != Token::RBRACK) {
      const JSVal target = ParseJSONValue(CHECK);
      ary->DefineOwnPropertyWithIndex(
          ctx_, current,
          DataDescriptor(target, PropertyDescriptor::WRITABLE |
                                 PropertyDescriptor::ENUMERABLE |
                                 PropertyDescriptor::CONFIGURABLE),
          false, CHECK);
      if (token_ != Token::RBRACK) {
        EXPECT(Token::COMMA);
        trailing_comma = true;
      } else {
        trailing_comma = false;
      }
      ++current;
    }
    assert(token_ == Token::RBRACK);
    if (trailing_comma) {
      RAISE();
    }
    Next();
    ary->Put(ctx_, ctx_->length_symbol(),
             current, false, CHECK);
    return ary;
  }

  JSVal ParseJSONString() {
    assert(token_ == Token::STRING);
    const std::vector<uc16>& vec = lexer_.Buffer();
    JSString* string = JSString::New(ctx_, vec.begin(), vec.end());
    Next();
    return string;
  }

  JSVal ParseJSONNumber() {
    assert(token_ == Token::NUMBER);
    const double number = lexer_.Numeric();
    Next();
    return number;
  }

  Symbol ParseSymbol(const std::vector<uc16>& range) {
    const Symbol res = ctx_->Intern(core::UStringPiece(range.data(), range.size()));
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
