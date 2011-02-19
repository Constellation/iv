#ifndef _IV_LV5_RUNTIME_GLOBAL_H_
#define _IV_LV5_RUNTIME_GLOBAL_H_
#include <cmath>
#include <tr1/cstdint>
#include "noncopyable.h"
#include "character.h"
#include "conversions.h"
#include "lv5/lv5.h"
#include "lv5/arguments.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/jsscript.h"
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

class URIComponent : core::Noncopyable<URIComponent>::type {
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

class URI : core::Noncopyable<URI>::type {
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

inline int UCS4ToUC8(uint32_t uc, uint8_t* buf) {
  assert(uc <= 0x10FFFF);
  if (uc < 0x80) {
    // ASCII range
    *buf = static_cast<uint8_t>(uc);
    return 1;
  } else {
    uint32_t a = uc >> 11;
    int len = 2;
    while (a) {
      a >>= 5;
      ++len;
    }
    int i = len;
    while (--i) {
      buf[i] = static_cast<uint8_t>((uc & 0x3F) | 0x80);
      uc >>= 6;
    }
    *buf = static_cast<uint8_t>(0x100 - (1 << (8 - len)) + uc);
    return len;
  }
}

inline uint32_t UC8ToUCS4(const uint8_t* buf, uint32_t size) {
  static const std::tr1::array<uint32_t, 3> kMinTable = { {
    0x00000080, 0x00000800, 0x00010000
  } };
  assert(size >= 1 && size <= 4);
  if (size == 1) {
    assert(!(*buf & 0x80));
    return *buf;
  } else {
    assert((*buf & (0x100 - (1 << (7 - size)))) == (0x100 - (1 << (8 - size))));
    uint32_t uc = (*buf++) & ((1 << (7 - size)) - 1);
    const uint32_t min = kMinTable[size-2];
    while (--size) {
      assert((*buf & 0xC0) == 0x80);
      uc = uc << 6 | (*buf++ & 0x3F);
    }
    if (uc < min) {
      uc = UINT32_MAX;
    }
    return uc;
  }
}

template<typename URITraits>
JSVal Encode(Context* ctx, const JSString& str, Error* e) {
  static const char kHexDigits[17] = "0123456789ABCDEF";
  std::tr1::array<uint8_t, 4> uc8buf;
  std::tr1::array<uint16_t, 3> hexbuf;
  StringBuilder builder;
  hexbuf[0] = '%';
  for (JSString::const_iterator it = str.begin(),
       last = str.end(); it != last; ++it) {
    const uint16_t ch = *it;
    if (URITraits::ContainsInEncode(ch)) {
      builder.Append(ch);
    } else {
      uint32_t v;
      if ((ch >= 0xDC00) && (ch <= 0xDFFF)) {
        e->Report(Error::URI, "invalid uri char");
        return JSUndefined;
      }
      if (ch < 0xD800 || 0xDBFF < ch) {
        v = ch;
      } else {
        ++it;
        if (it == last) {
          e->Report(Error::URI, "invalid uri char");
          return JSUndefined;
        }
        const uint16_t k_char = *it;
        if (k_char < 0xDC00 || 0xDFFF < k_char) {
          e->Report(Error::URI, "invalid uri char");
          return JSUndefined;
        }
        v = (ch - 0xD800) * 0x400 + (k_char - 0xDC00) + 0x10000;
      }
      for (int len = UCS4ToUC8(v, uc8buf.data()), i = 0;
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
JSVal Decode(Context* ctx, const JSString& str, Error* e) {
  StringBuilder builder;
  const uint32_t length = str.size();
  std::tr1::array<uint16_t, 3> buf;
  std::tr1::array<uint8_t, 4> octets;
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
        if (URITraits::ContainsInDecode(b0)) {
          builder.Append(buf.begin(), 3);
        } else {
          builder.Append(static_cast<uint16_t>(b0));
        }
      } else {
        int n = 1;
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
        for (int j = 1; j < n; ++j) {
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
          if ((b1 & 0xC0) != 0x80) {
            e->Report(Error::URI, "invalid uri char");
            return JSUndefined;
          }
          octets[j] = b1;
        }
        uint32_t v = UC8ToUCS4(octets.begin(), n);
        if (v < 0x10000) {
          const uint16_t code = static_cast<uint16_t>(v);
          if (URITraits::ContainsInDecode(code)) {
            builder.Append(str.begin() + start, (k - start + 1));
          } else {
            builder.Append(code);
          }
        } else {
          v -= 0x10000;
          const uint16_t L = (v & 0x3FF) + 0xDC00;
          const uint16_t H = ((v >> 10) & 0x3FF) + 0xD800;
          builder.Append(H);
          builder.Append(L);
        }
      }
    }
  }
  return builder.Build(ctx);
}
}  // iv::lv5::runtime::detail

inline JSVal InDirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(), first.string(),
                                         false, ERROR(error));
                                         //  ctx->IsStrict(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        Interpreter::NewDeclarativeEnvironment(ctx, ctx->global_env());
    const Interpreter::ContextSwitcher switcher(ctx,
                                                env,
                                                env,
                                                ctx->global_obj(),
                                                true);
    ctx->Run(script);
  } else {
    const Interpreter::ContextSwitcher switcher(ctx,
                                                ctx->global_env(),
                                                ctx->global_env(),
                                                ctx->global_obj(),
                                                false);
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

inline JSVal DirectCallToEval(const Arguments& args, Error* error) {
  if (!args.size()) {
    return JSUndefined;
  }
  const JSVal& first = args[0];
  if (!first.IsString()) {
    return first;
  }
  Context* const ctx = args.ctx();
  JSScript* const script = CompileScript(args.ctx(), first.string(),
                                         ctx->IsStrict(), ERROR(error));
  if (script->function()->strict()) {
    JSDeclEnv* const env =
        Interpreter::NewDeclarativeEnvironment(ctx, ctx->lexical_env());
    const Interpreter::ContextSwitcher switcher(ctx,
                                                env,
                                                env,
                                                ctx->this_binding(),
                                                true);
    ctx->Run(script);
  } else {
    ctx->Run(script);
  }
  if (ctx->IsShouldGC()) {
    GC_gcollect();
  }
  return ctx->ret();
}

inline JSVal GlobalEval(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("eval", args, error);
  return InDirectCallToEval(args, error);
}

inline JSVal GlobalParseInt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("parseInt", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    int radix = 0;
    if (args.size() > 1) {
      const double ret = args[1].ToNumber(args.ctx(), ERROR(error));
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
    return core::StringToIntegerWithRadix(str->begin(), str->end(),
                                          radix,
                                          strip_prefix);
  } else {
    return JSNaN;
  }
}

inline JSVal GlobalParseFloat(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("parseFloat", args, error);
  if (args.size() > 0) {
    JSString* const str = args[0].ToString(args.ctx(), ERROR(error));
    return core::StringToDouble(str->value(), true);
  } else {
    return JSNaN;
  }
}

inline JSVal GlobalIsNaN(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("isNaN", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), ERROR(error));
    if (std::isnan(number)) {
      return JSTrue;
    } else {
      return JSFalse;
    }
  } else {
    return JSTrue;
  }
}

inline JSVal GlobalIsFinite(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("isFinite", args, error);
  if (args.size() > 0) {
    const double number = args[0].ToNumber(args.ctx(), ERROR(error));
    return JSVal::Bool(std::isfinite(number));
  } else {
    return JSFalse;
  }
}

// 15.1.3 URI Handling Function Properties
// 15.1.3.1 decodeURI(encodedURI)
inline JSVal GlobalDecodeURI(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("decodeURI", args, error);
  const JSString* uri_string;
  if (args.size() > 0) {
    uri_string = args[0].ToString(args.ctx(), ERROR(error));
  } else {
    uri_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Decode<detail::URI>(args.ctx(), *uri_string, error);
}

// 15.1.3.2 decodeURIComponent(encodedURIComponent)
inline JSVal GlobalDecodeURIComponent(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("decodeURIComponent", args, error);
  const JSString* component_string;
  if (args.size() > 0) {
    component_string = args[0].ToString(args.ctx(), ERROR(error));
  } else {
    component_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Decode<detail::URIComponent>(args.ctx(),
                                              *component_string, error);
}

// 15.1.3.3 encodeURI(uri)
inline JSVal GlobalEncodeURI(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("encodeURIComponent", args, error);
  const JSString* uri_string;
  if (args.size() > 0) {
    uri_string = args[0].ToString(args.ctx(), ERROR(error));
  } else {
    uri_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Encode<detail::URI>(args.ctx(), *uri_string, error);
}

// 15.1.3.4 encodeURIComponent(uriComponent)
inline JSVal GlobalEncodeURIComponent(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("encodeURI", args, error);
  const JSString* component_string;
  if (args.size() > 0) {
    component_string = args[0].ToString(args.ctx(), ERROR(error));
  } else {
    component_string = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return detail::Encode<detail::URIComponent>(args.ctx(),
                                              *component_string, error);
}

inline JSVal ThrowTypeError(const Arguments& args, Error* error) {
  error->Report(Error::Type,
                "[[ThrowTypeError]] called");
  return JSUndefined;
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_GLOBAL_H_
