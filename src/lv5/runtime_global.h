#ifndef _IV_LV5_RUNTIME_GLOBAL_H_
#define _IV_LV5_RUNTIME_GLOBAL_H_
#include <cmath>
#include "detail/cstdint.h"
#include "noncopyable.h"
#include "character.h"
#include "conversions.h"
#include "unicode.h"
#include "platform_math.h"
#include "lv5/error_check.h"
#include "lv5/constructor_check.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/error.h"
#include "lv5/context.h"
#include "lv5/internal.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

inline bool IsURIMark(uint16_t ch) {
  return
      ch <= 126 &&
      (ch == 33 || ((39 <= ch) &&
                    ((ch <= 42) || ((45 <= ch) &&
                                    ((ch <= 46) || ch == 95 || ch == 126)))));
}

inline bool IsURIReserved(uint16_t ch) {
  return
      ch <= 64 &&
      (ch == 36 || ch == 38 || ((43 <= ch) &&
                                ((ch <= 44) || ch == 47 ||
                                 ((58 <= ch &&
                                   ((ch <= 59) || ch == 61 || (63 <= ch)))))));
}

inline bool IsEscapeTarget(uint16_t ch) {
  return
      '@' == ch ||
      '*' == ch ||
      '_' == ch ||
      '+' == ch ||
      '-' == ch ||
      '.' == ch ||
      '/' == ch;
}

class URIComponent : core::Noncopyable<> {
 public:
  static bool ContainsInEncode(uint16_t ch) {
    return core::character::IsASCII(ch) &&
        (core::character::IsASCIIAlphanumeric(ch) || IsURIMark(ch));
  }
  static bool ContainsInDecode(uint16_t ch) {
    // Empty String
    return false;
  }
};

class URI : core::Noncopyable<> {
 public:
  static bool ContainsInEncode(uint16_t ch) {
    return core::character::IsASCII(ch) &&
        (core::character::IsASCIIAlphanumeric(ch) ||
         IsURIMark(ch) ||
         ch == '#' ||
         IsURIReserved(ch));
  }
  static bool ContainsInDecode(uint16_t ch) {
    return IsURIReserved(ch) || ch == '#';
  }
};

class Escape : core::Noncopyable<> {
 public:
  static bool ContainsInEncode(uint16_t ch) {
    return core::character::IsASCII(ch) &&
        (core::character::IsASCIIAlphanumeric(ch) ||
         IsEscapeTarget(ch));
  }
  static bool ContainsInDecode(uint16_t ch) {
    return IsURIReserved(ch) || ch == '#';
  }
};

template<typename URITraits>
JSVal Encode(Context* ctx, const JSString& str, Error* e) {
  static const char kHexDigits[17] = "0123456789ABCDEF";
  std::array<uint8_t, 4> uc8buf;
  std::array<uint16_t, 3> hexbuf;
  StringBuilder builder;
  hexbuf[0] = '%';
  const JSString::Fiber* fiber = str.Flatten();
  for (JSString::Fiber::const_iterator it = fiber->begin(),
       last = fiber->end(); it != last; ++it) {
    const uint16_t ch = *it;
    if (URITraits::ContainsInEncode(ch)) {
      builder.Append(ch);
    } else {
      uint32_t v;
      if (core::unicode::IsLowSurrogate(ch)) {
        // ch is low surrogate. but high is not found.
        e->Report(Error::URI, "invalid uri char");
        return JSUndefined;
      }
      if (!core::unicode::IsHighSurrogate(ch)) {
        // ch is not surrogate pair code point
        v = ch;
      } else {
        // ch is high surrogate
        ++it;
        if (it == last) {
          // high surrogate only is invalid
          e->Report(Error::URI, "invalid uri char");
          return JSUndefined;
        }
        const uint16_t k_char = *it;
        if (!core::unicode::IsLowSurrogate(k_char)) {
          // k_char is not low surrogate
          e->Report(Error::URI, "invalid uri char");
          return JSUndefined;
        }
        // construct surrogate pair to ucs4
        v = (ch - core::unicode::kHighSurrogateMin) * 0x400 +
            (k_char - core::unicode::kLowSurrogateMin) + 0x10000;
      }
      for (std::size_t len = core::unicode::UCS4OneCharToUTF8(v, uc8buf.begin()), i = 0;
           i < len; ++i) {
        hexbuf[1] = kHexDigits[uc8buf[i] >> 4];
        hexbuf[2] = kHexDigits[uc8buf[i] & 0xf];
        builder.Append(hexbuf.begin(), 3);
      }
    }
  }
  return builder.Build(ctx);
}

template<typename URITraits>
JSVal Decode(Context* ctx, const JSString& arg, Error* e) {
  StringBuilder builder;
  const JSString::Fiber& str = *arg.Flatten();
  const uint32_t length = str.size();
  std::array<uint16_t, 3> buf;
  std::array<uint8_t, 4> octets;
  buf[0] = '%';
  for (uint32_t k = 0; k < length; ++k) {
    const uint16_t ch = str[k];
    if (ch != '%') {
      builder.Append(ch);
    } else {
      const uint32_t start = k;
      if (k + 2 >= length) {
        e->Report(Error::URI, "invalid uri char");
        return JSUndefined;
      }
      buf[1] = str[k+1];
      buf[2] = str[k+2];
      k += 2;
      if ((!core::character::IsHexDigit(buf[1])) ||
          (!core::character::IsHexDigit(buf[2]))) {
        e->Report(Error::URI, "invalid uri char");
        return JSUndefined;
      }
      const uint8_t b0 = core::HexValue(buf[1]) * 16 + core::HexValue(buf[2]);
      if (!(b0 & 0x80)) {
        // b0 is 0xxxxxxx, in ascii range
        if (URITraits::ContainsInDecode(b0)) {
          builder.Append(buf.begin(), 3);
        } else {
          builder.Append(static_cast<uint16_t>(b0));
        }
      } else {
        // b0 is 1xxxxxxx
        std::size_t n = 1;
        while (b0 & (0x80 >> n)) {
          ++n;
        }
        if (n == 1 || n > 4) {
          e->Report(Error::URI, "invalid uri char");
          return JSUndefined;
        }
        octets[0] = b0;
        if (k + (3 * (n - 1)) >= length) {
          e->Report(Error::URI, "invalid uri char");
          return JSUndefined;
        }
        for (std::size_t j = 1; j < n; ++j) {
          ++k;
          if (str[k] != '%') {
            e->Report(Error::URI, "invalid uri char");
            return JSUndefined;
          }
          buf[1] = str[k+1];
          buf[2] = str[k+2];
          k += 2;
          if ((!core::character::IsHexDigit(buf[1])) ||
              (!core::character::IsHexDigit(buf[2]))) {
            e->Report(Error::URI, "invalid uri char");
            return JSUndefined;
          }
          const uint8_t b1 =
              core::HexValue(buf[1]) * 16 + core::HexValue(buf[2]);
          // section 15.1.3 decoding 4-b-7-7-e
          // code point 10xxxxxx check is moved to core::UTF8ToUCS4Strict
          octets[j] = b1;
        }
        core::unicode::UTF8Error err = core::unicode::NO_ERROR;
        uint32_t v;
        core::unicode::NextUCS4FromUTF8(octets.begin(), octets.end(), &v, &err);
        // if octets utf8 sequence is not valid,
        if (err != core::unicode::NO_ERROR) {
          e->Report(Error::URI, "invalid uri char");
          return JSUndefined;
        }
        if (v < 0x10000) {
          // not surrogate pair
          const uint16_t code = static_cast<uint16_t>(v);
          if (URITraits::ContainsInDecode(code)) {
            builder.Append(str.begin() + start, (k - start + 1));
          } else {
            builder.Append(code);
          }
        } else {
          // surrogate pair
          v -= 0x10000;
          const uint16_t L =
              (v & core::unicode::kLowSurrogateMask) +
              core::unicode::kLowSurrogateMin;
          const uint16_t H =
              ((v >> core::unicode::kSurrogateBits) &
               core::unicode::kHighSurrogateMask) +
              core::unicode::kHighSurrogateMin;
          builder.Append(H);
          builder.Append(L);
        }
      }
    }
  }
  return builder.Build(ctx);
}

}  // namespace detail

inline JSVal GlobalParseInt(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("parseInt", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), IV_LV5_ERROR(error));
    int radix = 0;
    if (args.size() > 1) {
      const double ret = args[1].ToNumber(args.ctx(), IV_LV5_ERROR(error));
      radix = core::DoubleToInt32(ret);
    }
    bool strip_prefix = true;
    if (radix != 0) {
      if (radix < 2 || radix > 36) {
        return JSNaN;
      }
      if (radix != 16) {
        strip_prefix = false;
      }
    } else {
      radix = 10;
    }
    const JSString::Fiber* fiber = str->Flatten();
    return core::StringToIntegerWithRadix(fiber->begin(), fiber->end(),
                                          radix,
                                          strip_prefix);
  } else {
    return JSNaN;
  }
}

// section 15.1.2.3 parseFloat(string)
inline JSVal GlobalParseFloat(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("parseFloat", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), IV_LV5_ERROR(error));
    return core::StringToDouble(*str->Flatten(), true);
  } else {
    return JSNaN;
  }
}

// section 15.1.2.4 isNaN(number)
inline JSVal GlobalIsNaN(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("isNaN", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    if (core::IsNaN(number)) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSTrue;
  }
}

// section 15.1.2.5 isFinite(number)
inline JSVal GlobalIsFinite(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("isFinite", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), IV_LV5_ERROR(error));
    return JSVal::Bool(core::IsFinite(number));
  } else {
    return JSFalse;
  }
}

// section 15.1.3 URI Handling Function Properties
// section 15.1.3.1 decodeURI(encodedURI)
inline JSVal GlobalDecodeURI(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("decodeURI", args, error);
  const JSString* uri_string;
  if (args.size() > 0) {
    uri_string = args[0].ToString(args.ctx(), IV_LV5_ERROR(error));
  } else {
    uri_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Decode<detail::URI>(args.ctx(), *uri_string, error);
}

// section 15.1.3.2 decodeURIComponent(encodedURIComponent)
inline JSVal GlobalDecodeURIComponent(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("decodeURIComponent", args, error);
  const JSString* component_string;
  if (args.size() > 0) {
    component_string = args[0].ToString(args.ctx(), IV_LV5_ERROR(error));
  } else {
    component_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Decode<detail::URIComponent>(args.ctx(),
                                              *component_string, error);
}

// section 15.1.3.3 encodeURI(uri)
inline JSVal GlobalEncodeURI(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("encodeURIComponent", args, error);
  const JSString* uri_string;
  if (args.size() > 0) {
    uri_string = args[0].ToString(args.ctx(), IV_LV5_ERROR(error));
  } else {
    uri_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Encode<detail::URI>(args.ctx(), *uri_string, error);
}

// section 15.1.3.4 encodeURIComponent(uriComponent)
inline JSVal GlobalEncodeURIComponent(const Arguments& args, Error* error) {
  IV_LV5_CONSTRUCTOR_CHECK("encodeURI", args, error);
  const JSString* component_string;
  if (args.size() > 0) {
    component_string = args[0].ToString(args.ctx(), IV_LV5_ERROR(error));
  } else {
    component_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Encode<detail::URIComponent>(args.ctx(),
                                              *component_string, error);
}

inline JSVal ThrowTypeError(const Arguments& args, Error* e) {
  e->Report(Error::Type, "[[ThrowTypeError]] called");
  return JSUndefined;
}

// section B.2.1 escape(string)
// this method is deprecated.
inline JSVal GlobalEscape(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("escape", args, e);
  Context* const ctx = args.ctx();
  JSString* str = args.At(0).ToString(ctx, IV_LV5_ERROR(e));
  const std::size_t len = str->size();
  static const char kHexDigits[17] = "0123456789ABCDEF";
  std::array<uint16_t, 6> ubuf;
  std::array<uint16_t, 3> hexbuf;
  ubuf[0] = '%';
  ubuf[1] = 'u';
  hexbuf[0] = '%';
  StringBuilder builder;
  if (len == 0) {
    return str;  // empty string
  }
  const JSString::Fiber* fiber = str->Flatten();
  for (JSString::Fiber::const_iterator it = fiber->begin(),
       last = it + len; it != last; ++it) {
    const uint16_t ch = *it;
    if (detail::Escape::ContainsInEncode(ch)) {
      builder.Append(ch);
    } else {
      if (ch < 256) {
        hexbuf[1] = kHexDigits[ch / 16];
        hexbuf[2] = kHexDigits[ch % 16];
        builder.Append(hexbuf.begin(), hexbuf.size());
      } else {
        uint16_t val = ch;
        for (int i = 0; i < 4; i++) {
          ubuf[5 - i] = kHexDigits[val % 16];
          val /= 16;
        }
        builder.Append(ubuf.begin(), ubuf.size());
      }
    }
  }
  return builder.Build(ctx);
}

// section B.2.2 unescape(string)
// this method is deprecated.
inline JSVal GlobalUnescape(const Arguments& args, Error* e) {
  IV_LV5_CONSTRUCTOR_CHECK("unescape", args, e);
  Context* const ctx = args.ctx();
  JSString* s = args.At(0).ToString(ctx, IV_LV5_ERROR(e));
  const std::size_t len = s->size();
  if (len == 0) {
    return s;  // empty string
  }
  StringBuilder builder;
  const JSString::Fiber* str = s->Flatten();
  std::size_t k = 0;
  while (k != len) {
    const uint16_t ch = (*str)[k];
    if (ch == '%') {
      if (k <= (len - 6) &&
          (*str)[k + 1] == 'u' &&
          core::character::IsHexDigit((*str)[k + 2]) &&
          core::character::IsHexDigit((*str)[k + 3]) &&
          core::character::IsHexDigit((*str)[k + 4]) &&
          core::character::IsHexDigit((*str)[k + 5])) {
        uint16_t uc = '\0';
        for (int i = k + 2, last = k + 6; i < last; ++i) {
          const int d = core::HexValue((*str)[i]);
          uc = uc * 16 + d;
        }
        builder.Append(uc);
        k += 6;
      } else if (k <= (len - 3) &&
                 core::character::IsHexDigit((*str)[k + 1]) &&
                 core::character::IsHexDigit((*str)[k + 2])) {
        // step 14
        builder.Append(
            core::HexValue((*str)[k + 1]) * 16 + core::HexValue((*str)[k + 2]));
        k += 3;
      } else {
        // step 18
        builder.Append(ch);
        ++k;
      }
    } else {
      // step 18
      builder.Append(ch);
      ++k;
    }
  }
  return builder.Build(ctx);
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_GLOBAL_H_
