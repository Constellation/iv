#ifndef _IV_PHONIC_ENCODING_H_
#define _IV_PHONIC_ENCODING_H_
extern "C" {
#include <ruby.h>
#include <ruby/encoding.h>
#include <ruby/intern.h>
}
namespace iv {
namespace phonic {

static VALUE CEncoding_UTF_8;
static VALUE CEncoding_UTF_16BE;
static VALUE CEncoding_UTF_16LE;
static VALUE CEncoding_UTF_16;
static VALUE CEncoding_UTF_32BE;
static VALUE CEncoding_UTF_32LE;
static VALUE CEncoding_UTF_32;
static VALUE CEncoding_ASCII_8BIT;
static ID kForceEncoding;
static ID kEncodeBang;

class Encoding {
 public:
  static void Init() {
    CEncoding_UTF_8 = rb_funcall(
        rb_path2class("Encoding"), rb_intern("find"), 1, rb_str_new2("utf-8"));
    CEncoding_UTF_16BE = rb_funcall(
        rb_path2class("Encoding"), rb_intern("find"),
        1, rb_str_new2("utf-16be"));
    CEncoding_UTF_16LE = rb_funcall(
        rb_path2class("Encoding"), rb_intern("find"),
        1, rb_str_new2("utf-16le"));
    CEncoding_UTF_32BE = rb_funcall(
        rb_path2class("Encoding"), rb_intern("find"),
        1, rb_str_new2("utf-32be"));
    CEncoding_UTF_32LE = rb_funcall(
        rb_path2class("Encoding"), rb_intern("find"),
        1, rb_str_new2("utf-32le"));
    if (IsLittle()) {
      CEncoding_UTF_16 = CEncoding_UTF_16LE;
      CEncoding_UTF_32 = CEncoding_UTF_32LE;
    } else {
      CEncoding_UTF_16 = CEncoding_UTF_16BE;
      CEncoding_UTF_32 = CEncoding_UTF_32BE;
    }
    CEncoding_ASCII_8BIT = rb_funcall(
        rb_path2class("Encoding"), rb_intern("find"),
        1, rb_str_new2("ascii-8bit"));
    kForceEncoding = rb_intern("force_encoding");
    kEncodeBang = rb_intern("encode!");
  }

  static inline VALUE UTF8Encoding() {
    return CEncoding_UTF_8;
  }

  static inline VALUE ConvertToDefaultInternal(VALUE str) {
    VALUE source = rb_str_dup(str);
    rb_funcall(source, kEncodeBang, 1, rb_enc_default_external());
    return source;
  }

  static inline VALUE UTF16BEEncoding() {
    return CEncoding_UTF_16BE;
  }

  static inline VALUE UTF16LEEncoding() {
    return CEncoding_UTF_16LE;
  }

  static inline VALUE UTF32BEEncoding() {
    return CEncoding_UTF_32BE;
  }

  static inline VALUE UTF32LEEncoding() {
    return CEncoding_UTF_32LE;
  }

  static inline VALUE AsciiEncoding() {
    return CEncoding_ASCII_8BIT;
  }

  static inline VALUE UTF16Encoding() {
    return CEncoding_UTF_16;
  }
  static inline VALUE UTF32Encoding() {
    return CEncoding_UTF_32;
  }

  static inline bool IsLittle() {
    return Which() == LITTLE;
  }
  static inline bool IsBig() {
    return Which() == BIG;
  }
 private:
  enum {
    BIG = 0,
    LITTLE = 1
  };
  static inline int Which() {
    int x = 0x00000001;
    if (*reinterpret_cast<char*>(&x)) {
      return LITTLE;
    } else {
      return BIG;
    }
  }
};

} }  // namespace iv::phonic
#endif  // _IV_PHONIC_ENCODING_H_
