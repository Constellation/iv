#ifndef _IV_PHONIC_RPARSER_H_
#define _IV_PHONIC_RPARSER_H_
#include <cstdlib>
#include <cstring>
#include <ruby.h>
#include <ruby/encoding.h>
#include <ruby/intern.h>
#include <iv/parser.h>
#include "encoding.h"
#include "factory.h"
#include "parser.h"
#include "creator.h"
#include "source.h"
namespace iv {
namespace phonic {

static VALUE cParseError;

class RParser {
 public:
  static VALUE Parse(VALUE self, VALUE rb_str) {
    AstFactory factory;
    Check_Type(rb_str, T_STRING);
    VALUE encoded_rb_str = rb_str_encode(rb_str,
                                         Encoding::UTF16Encoding(),
                                         0,
                                         Qnil);
    const char* str = StringValuePtr(encoded_rb_str);
    const std::size_t len = RSTRING_LEN(encoded_rb_str);
    UTF16Source source(str, len);
    Parser parser(&factory, &source);
    const FunctionLiteral* global = parser.ParseProgram();
    if (!global) {
      rb_raise(cParseError, "%s", parser.error().c_str());
    } else {
      Creator creator;
      return creator.Result(global);
    }
  }

  static void Init(VALUE mIVPhonic) {
    cParseError = rb_define_class_under(mIVPhonic, "ParseError", rb_eStandardError);
  }
};

} }  // namespace iv::phonic
#endif  // _IV_PHONIC_RPARSER_H_
