#ifndef _IV_LV5_RUNTIME_STRING_H_
#define _IV_LV5_RUNTIME_STRING_H_
#include <cassert>
#include <vector>
#include <utility>
#include <tr1/tuple>
#include "ustring.h"
#include "ustringpiece.h"
#include "character.h"
#include "conversions.h"
#include "arguments.h"
#include "jsval.h"
#include "error.h"
#include "jsstring.h"
#include "jsregexp.h"
#include "runtime_regexp.h"

namespace iv {
namespace lv5 {
namespace runtime {
namespace detail {

typedef std::vector<std::pair<int, int> > PairVector;

static inline JSVal StringToStringValueOfImpl(const Arguments& args,
                                              Error* error,
                                              const char* msg) {
  const JSVal& obj = args.this_binding();
  if (!obj.IsString()) {
    if (obj.IsObject() &&
        args.ctx()->Cls("String").name == obj.object()->class_name()) {
      return static_cast<JSStringObject*>(obj.object())->value();
    } else {
      error->Report(Error::Type, msg);
      return JSUndefined;
    }
  } else {
    return obj.string();
  }
}

static inline bool IsTrimmed(uint16_t c) {
  return core::character::IsWhiteSpace(c) ||
         core::character::IsLineTerminator(c);
}

inline int64_t SplitMatch(const JSString& str,
                          uint32_t q, const JSString& rstr) {
  const std::size_t rs = rstr.size();
  const std::size_t s = str.size();
  if (q + rs > s) {
    return -1;
  }
  if (rstr.value().compare(0, rs, str.value().data() + q, rs) != 0) {
    return -1;
  }
  return q + rs;
}

inline JSVal StringSplit(Context* ctx, const JSString& str,
                         const JSString& rstr, uint32_t lim) {
  Error e;
  uint32_t length = 0;
  uint32_t p = 0;
  uint32_t q = p;
  const uint32_t size = str.size();
  JSArray* const ary = JSArray::New(ctx);
  if (size == 0) {
    if (detail::SplitMatch(str, q, rstr) != -1) {
      return ary;
    }
  }
  while (q != size) {
    const int64_t rs = detail::SplitMatch(str, q, rstr);
    if (rs == -1) {
      ++q;
    } else {
      const uint32_t end = static_cast<uint32_t>(rs);
      if (end == p) {
        ++q;
      } else {
        ary->DefineOwnPropertyWithIndex(
            ctx, length,
            DataDescriptor(
                JSString::New(ctx, str.begin() + p, str.begin() + q),
                PropertyDescriptor::WRITABLE |
                PropertyDescriptor::ENUMERABLE |
                PropertyDescriptor::CONFIGURABLE),
            false, &e);
        ++length;
        if (length == lim) {
          assert(!e);
          return ary;
        }
        q = p = end;
      }
    }
  }
  ary->DefineOwnPropertyWithIndex(
      ctx, length,
      DataDescriptor(
          JSString::New(ctx,
                        str.begin() + p,
                        str.begin() + size),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::ENUMERABLE |
          PropertyDescriptor::CONFIGURABLE),
      false, &e);
  assert(!e);
  return ary;
}

typedef std::tr1::tuple<uint32_t, uint32_t, bool> MatchResult;
inline MatchResult RegExpMatch(const JSString& str,
                               uint32_t q,
                               const JSRegExp& reg,
                               PairVector* vec) {
  vec->clear();
  return reg.Match(str.value(), q, vec);
}

struct Replace {
  enum State {
    kNormal,
    kDollar,
    kDigit,
    kDigitZero
  };
};

inline JSString* ReplaceOnce(Context* ctx, const JSString& str,
                             const JSString& search_str,
                             JSString::size_type loc,
                             const JSString& replace_str) {
  StringBuilder builder;
  builder.Append(str.begin(), str.begin() + loc);
  Replace::State state = Replace::kNormal;
  for (JSString::const_iterator it = replace_str.begin(),
       last = replace_str.end(); it != last; ++it) {
    const uc16 ch = *it;
    if (state == Replace::kNormal) {
      if (ch == '$') {
        state = Replace::kDollar;
      } else {
        builder.Append(ch);
      }
    } else {
      assert(state == Replace::kDollar);
      switch (ch) {
        case '$':  // $$ pattern
          state = Replace::kNormal;
          builder.Append('$');
          break;

        case '&':  // $& pattern
          state = Replace::kNormal;
          builder.Append(search_str);
          break;

        case '`':  // $` pattern
          state = Replace::kNormal;
          builder.Append(str.begin(), str.begin() + loc);
          break;

        case '\'':  // $' pattern
          state = Replace::kNormal;
          builder.Append(str.begin() + loc + search_str.size(), str.end());
          break;

        default:
          state = Replace::kNormal;
          builder.Append('$');
          builder.Append(ch);
      }
    }
  }
  if (state == Replace::kDollar) {
    builder.Append('$');
  }
  builder.Append(str.begin() + loc + search_str.size(), str.end());
  return builder.Build(ctx);
}

inline JSString* ReplaceRegExpOnce(Context* ctx,
                                   const JSString& str,
                                   const MatchResult& res,
                                   const PairVector& vec,
                                   const JSString& replace_str) {
  using std::tr1::get;
  StringBuilder builder;
  builder.Append(str.begin(), str.begin() + get<0>(res));
  Replace::State state = Replace::kNormal;
  uc16 upper_digit_char = '\0';
  for (JSString::const_iterator it = replace_str.begin(),
       last = replace_str.end(); it != last; ++it) {
    const uc16 ch = *it;
    if (state == Replace::kNormal) {
      if (ch == '$') {
        state = Replace::kDollar;
      } else {
        builder.Append(ch);
      }
    } else if (state == Replace::kDollar) {
      switch (ch) {
        case '$':  // $$ pattern
          state = Replace::kNormal;
          builder.Append('$');
          break;

        case '&':  // $& pattern
          state = Replace::kNormal;
          builder.Append(str.begin() + get<0>(res),
                         str.begin() + get<1>(res));
          break;

        case '`':  // $` pattern
          state = Replace::kNormal;
          builder.Append(str.begin(), str.begin() + get<0>(res));
          break;

        case '\'':  // $' pattern
          state = Replace::kNormal;
          builder.Append(str.begin() + get<1>(res), str.end());
          break;

        default:
          if (core::character::IsDecimalDigit(ch)) {
            if (ch == '0') {  // 0
              state = Replace::kDigitZero;
            } else {
              state = Replace::kDigit;
            }
            upper_digit_char = ch;
          } else {
            state = Replace::kNormal;
            builder.Append('$');
            builder.Append(ch);
          }
      }
    } else if (state == Replace::kDigit) {
      if (core::character::IsDecimalDigit(ch)) {  // twin digit
        const std::size_t n =
            core::Radix36Value(upper_digit_char) * 10 + core::Radix36Value(ch);
        if (vec.size() >= n) {
          const PairVector::value_type& pair = vec[n - 1];
          builder.Append(str.begin() + pair.first,
                         str.begin() + pair.second);
        } else {
          // put "undefined"
          builder.Append("undefined");
        }
      } else {
        const std::size_t n = core::Radix36Value(upper_digit_char);
        if (vec.size() >= n) {
          const PairVector::value_type& pair = vec[n - 1];
          builder.Append(str.begin() + pair.first,
                         str.begin() + pair.second);
        } else {
          // put "undefined"
          builder.Append("undefined");
        }
        builder.Append(ch);
      }
      state = Replace::kNormal;
    } else {
      assert(state == Replace::kDigitZero);
      if (core::character::IsDecimalDigit(ch)) {
        const std::size_t n =
            core::Radix36Value(upper_digit_char) * 10 + core::Radix36Value(ch);
        if (vec.size() >= n) {
          const PairVector::value_type& pair = vec[n - 1];
          builder.Append(str.begin() + pair.first,
                         str.begin() + pair.second);
        } else {
          // put "undefined"
          builder.Append("undefined");
        }
      } else {
        // $0 is not used
        builder.Append('$');
        builder.Append('0');
      }
      state = Replace::kNormal;
    }
  }

  if (state == Replace::kDollar) {
    builder.Append('$');
  } else if (state == Replace::kDigit ||
             state == Replace::kDigitZero) {
    builder.Append(upper_digit_char);
  }

  builder.Append(str.begin() + get<1>(res), str.end());
  return builder.Build(ctx);
}

}  // namespace iv::lv5::runtime::detail

// section 15.5.1
inline JSVal StringConstructor(const Arguments& args, Error* error) {
  if (args.IsConstructorCalled()) {
    JSString* str;
    if (args.size() > 0) {
      str = args[0].ToString(args.ctx(), ERROR(error));
    } else {
      str = JSString::NewEmptyString(args.ctx());
    }
    return JSStringObject::New(args.ctx(), str);
  } else {
    if (args.size() > 0) {
      return args[0].ToString(args.ctx(), error);
    } else {
      return JSString::NewEmptyString(args.ctx());
    }
  }
}

// section 15.5.3.2 String.fromCharCode([char0 [, char1[, ...]]])
inline JSVal StringFromCharCode(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.fromCharCode", args, error);
  StringBuilder builder;
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const double val = it->ToNumber(args.ctx(), ERROR(error));
    builder.Append(core::DoubleToUInt32(val));
  }
  return builder.Build(args.ctx());
}

// section 15.5.4.2 String.prototype.toString()
inline JSVal StringToString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.toString", args, error);
  return detail::StringToStringValueOfImpl(
      args, error,
      "String.prototype.toString is not generic function");
}

// section 15.5.4.3 String.prototype.valueOf()
inline JSVal StringValueOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.valueOf", args, error);
  return detail::StringToStringValueOfImpl(
      args, error,
      "String.prototype.valueOf is not generic function");
}

// section 15.5.4.4 String.prototype.charAt(pos)
inline JSVal StringCharAt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.charAt", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  double position;
  if (args.size() > 0) {
    position = args[0].ToNumber(args.ctx(), ERROR(error));
    position = core::DoubleToInteger(position);
  } else {
    // undefined -> NaN -> 0
    position = 0;
  }
  if (position < 0 || position >= str->size()) {
    return JSString::NewEmptyString(args.ctx());
  } else {
    return JSString::New(
        args.ctx(),
        core::UStringPiece(str->data() + core::DoubleToUInt32(position), 1));
  }
}

// section 15.5.4.5 String.prototype.charCodeAt(pos)
inline JSVal StringCharCodeAt(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.charCodeAt", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  double position;
  if (args.size() > 0) {
    position = args[0].ToNumber(args.ctx(), ERROR(error));
    position = core::DoubleToInteger(position);
  } else {
    // undefined -> NaN -> 0
    position = 0;
  }
  if (position < 0 || position >= str->size()) {
    return JSNaN;
  } else {
    return (*str)[core::DoubleToUInt32(position)];
  }
}

// section 15.5.4.6 String.prototype.concat([string1[, string2[, ...]]])
inline JSVal StringConcat(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.concat", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  core::UString result(str->begin(), str->end());
  for (Arguments::const_iterator it = args.begin(),
       last = args.end(); it != last; ++it) {
    const JSString* const r = it->ToString(args.ctx(), ERROR(error));
    result.append(r->begin(), r->end());
  }
  return JSString::New(args.ctx(), result);
}

// section 15.5.4.7 String.prototype.indexOf(searchString, position)
inline JSVal StringIndexOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.indexOf", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  const JSString* search_str;
  // undefined -> NaN -> 0
  double position = 0;
  if (args.size() > 0) {
    search_str = args[0].ToString(args.ctx(), ERROR(error));
    if (args.size() > 1) {
      position = args[1].ToNumber(args.ctx(), ERROR(error));
      position = core::DoubleToInteger(position);
    }
  } else {
    // undefined -> "undefined"
    search_str = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  const std::size_t start = std::min(
      static_cast<std::size_t>(std::max(position, 0.0)), str->size());
  const GCUString::size_type loc =
      str->value().find(search_str->value(), start);
  return (loc == GCUString::npos) ? -1.0 : loc;
}

// section 15.5.4.8 String.prototype.lastIndexOf(searchString, position)
inline JSVal StringLastIndexOf(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.lastIndexOf", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  const JSString* search_str;
  std::size_t target = str->size();
  if (args.size() > 0) {
    search_str = args[0].ToString(args.ctx(), ERROR(error));
    // undefined -> NaN
    if (args.size() > 1) {
      const double position = args[1].ToNumber(args.ctx(), ERROR(error));
      if (!std::isnan(position)) {
        const double integer = core::DoubleToInteger(position);
        if (integer < 0) {
          target = 0;
        } else if (integer < target) {
          target = static_cast<std::size_t>(integer);
        }
      }
    }
  } else {
    // undefined -> "undefined"
    search_str = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  const GCUString::size_type loc =
      str->value().rfind(search_str->value(), target);
  return (loc == GCUString::npos) ? -1.0 : loc;
}

// section 15.5.4.9 String.prototype.localeCompare(that)
inline JSVal StringLocaleCompare(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.localeCompare", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  const JSString* that;
  if (args.size() > 0) {
    that = args[0].ToString(args.ctx(), ERROR(error));
  } else {
    that = JSString::NewAsciiString(args.ctx(), "undefined");
  }
  return str->value().compare(that->value());
}


// section 15.5.4.10 String.prototype.match(regexp)
inline JSVal StringMatch(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("String.prototype.match", args, e);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(e));
  Context* const ctx = args.ctx();
  JSString* const str = val.ToString(ctx, ERROR(e));
  const uint32_t args_count = args.size();
  JSRegExp* regexp;
  if (args_count == 0 ||
      !args[0].IsObject() ||
      (args[0].object()->class_name() != ctx->Intern("RegExp"))) {
    Arguments a(ctx, 1);
    if (args_count == 0) {
      a[0] = JSUndefined;
    } else {
      a[0] = args[0];
    }
    const JSVal res = RegExpConstructor(a, ERROR(e));
    assert(res.IsObject());
    regexp = static_cast<JSRegExp*>(res.object());
  } else {
    regexp = static_cast<JSRegExp*>(args[0].object());
  }
  const bool global = regexp->global();
  if (!global) {
    return regexp->Exec(ctx, str, e);
  }
  // step 8
  return regexp->ExecGlobal(ctx, str, e);
}

// section 15.5.4.11 String.prototype.replace(searchValue, replaceValue)
inline JSVal StringReplace(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("String.prototype.replace", args, e);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(e));
  Context* const ctx = args.ctx();
  JSString* const str = val.ToString(ctx, ERROR(e));
  const uint32_t args_count = args.size();
  const bool search_value_is_regexp =
      args_count != 0 &&
      args[0].IsObject() &&
      (args[0].object()->class_name() == ctx->Intern("RegExp"));

  JSString* search_str = NULL;

  if (search_value_is_regexp) {
    // searchValue is RegExp
    using std::tr1::get;
    const JSRegExp* reg = static_cast<JSRegExp*>(args[0].object());
    detail::PairVector cap;
    if (reg->global()) {
    } else {
      const detail::MatchResult res = detail::RegExpMatch(*str, 0, *reg, &cap);
      if (!get<2>(res)) {  // not match
        return str;
      }
      if (args_count > 1 && args[1].IsCallable()) {
      } else {
        const JSString* replace_value;
        if (args_count > 1) {
          replace_value = args[1].ToString(ctx, ERROR(e));
        } else {
          replace_value = JSString::NewAsciiString(args.ctx(), "undefined");
        }
        return detail::ReplaceRegExpOnce(ctx, *str, res, cap, *replace_value);
      }
    }
  } else {
    if (args_count == 0) {
      search_str = JSString::NewAsciiString(args.ctx(), "undefined");
    } else {
      search_str = args[0].ToString(ctx, ERROR(e));
    }
    const GCUString::size_type loc = str->value().find(search_str->value(), 0);
    if (loc == GCUString::npos) {
      // not found
      return str;
    }
    // found pattern
    if (args_count > 1 && args[1].IsCallable()) {
      JSFunction* const callable = args[1].object()->AsCallable();
      Arguments a(ctx, 3);
      a[0] = search_str;
      a[1] = static_cast<double>(loc);
      a[2] = str;
      const JSVal result = callable->Call(a, JSUndefined, ERROR(e));
      StringBuilder builder;
      const JSString* const res = result.ToString(ctx, ERROR(e));
      builder.Append(str->begin(), str->begin() + loc);
      builder.Append(*res);
      builder.Append(str->begin() + loc + search_str->size(), str->end());
      return builder.Build(ctx);
    } else {
      const JSString* replace_value;
      if (args_count > 1) {
        replace_value = args[1].ToString(ctx, ERROR(e));
      } else {
        replace_value = JSString::NewAsciiString(args.ctx(), "undefined");
      }
      return detail::ReplaceOnce(ctx, *str, *search_str, loc, *replace_value);
    }
  }
  return str;
}

// section 15.5.4.12 String.prototype.search(regexp)
inline JSVal StringSearch(const Arguments& args, Error* e) {
  CONSTRUCTOR_CHECK("String.prototype.search", args, e);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(e));
  Context* const ctx = args.ctx();
  JSString* const str = val.ToString(ctx, ERROR(e));
  const uint32_t args_count = args.size();
  JSRegExp* regexp;
  if (args_count == 0) {
    regexp = JSRegExp::New(ctx);
  } else if (args[0].IsObject() &&
             (args[0].object()->class_name() == ctx->Intern("RegExp"))) {
    regexp = static_cast<JSRegExp*>(args[0].object());
  } else {
    Arguments a(ctx, 1);
    a[0] = args[0];
    const JSVal res = RegExpConstructor(a, ERROR(e));
    assert(res.IsObject());
    regexp = static_cast<JSRegExp*>(res.object());
  }
  // TODO(Constellation) using raw regexp
  const int last_index = regexp->LastIndex(ctx, ERROR(e));
  regexp->SetLastIndex(ctx, 0, ERROR(e));
  const JSVal result = regexp->Exec(ctx, str, e);
  regexp->SetLastIndex(ctx, last_index, ERROR(e));
  if (!e) {
    return JSUndefined;
  }
  if (result.IsNull()) {
    return static_cast<double>(-1);
  } else {
    assert(result.IsObject());
    return result.object()->Get(ctx, ctx->Intern("index"), e);
  }
}

// section 15.5.4.13 String.prototype.slice(start, end)
inline JSVal StringSlice(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.slice", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  Context* const ctx = args.ctx();
  const JSString* const str = val.ToString(ctx, ERROR(error));
  const uint32_t len = str->size();
  uint32_t start;
  if (args.size() > 0) {
    double relative_start = args[0].ToNumber(ctx, ERROR(error));
    relative_start = core::DoubleToInteger(relative_start);
    if (relative_start < 0) {
      start = core::DoubleToUInt32(std::max<double>(relative_start + len, 0.0));
    } else {
      start = core::DoubleToUInt32(std::min<double>(relative_start, len));
    }
  } else {
    start = 0;
  }
  uint32_t end;
  if (args.size() > 1) {
    if (args[1].IsUndefined()) {
      end = len;
    } else {
      double relative_end = args[1].ToNumber(ctx, ERROR(error));
      relative_end = core::DoubleToInteger(relative_end);
      if (relative_end < 0) {
        end = core::DoubleToUInt32(std::max<double>(relative_end + len, 0.0));
      } else {
        end = core::DoubleToUInt32(std::min<double>(relative_end, len));
      }
    }
  } else {
    end = len;
  }
  const uint32_t span = (end < start) ? 0 : end - start;
  return JSString::New(ctx,
                       str->begin() + start,
                       str->begin() + start + span);
}

// section 15.5.4.14 String.prototype.split(separator, limit)
inline JSVal StringSplit(const Arguments& args, Error* e) {
  using std::tr1::get;
  CONSTRUCTOR_CHECK("String.prototype.split", args, e);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(e));
  Context* const ctx = args.ctx();
  JSString* const str = val.ToString(ctx, ERROR(e));
  const uint32_t args_count = args.size();
  uint32_t lim;
  if (args_count < 2 || args[1].IsUndefined()) {
    lim = 4294967295UL;  // (1 << 32) - 1
  } else {
    const double temp = args[1].ToNumber(ctx, ERROR(e));
    lim = core::DoubleToUInt32(temp);
  }

  bool regexp = false;
  JSVal target;
  JSVal separator;
  if (args_count > 0) {
    separator = args[0];
    if (separator.IsObject() &&
        (ctx->Intern("RegExp") == separator.object()->class_name())) {
      target = separator;
      regexp = true;
    } else {
      target = separator.ToString(ctx, ERROR(e));
    }
  } else {
    separator = JSUndefined;
  }

  if (lim == 0) {
    return JSArray::New(ctx);
  }

  if (separator.IsUndefined()) {
    JSArray* const a = JSArray::New(ctx);
    a->DefineOwnPropertyWithIndex(
      ctx, 0, DataDescriptor(str,
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE |
                             PropertyDescriptor::CONFIGURABLE),
      false, ERROR(e));
    return a;
  }

  if (!regexp) {
    return detail::StringSplit(ctx, *str, *target.string(), lim);
  }

  JSRegExp* const reg = static_cast<JSRegExp*>(target.object());
  JSArray* const ary = JSArray::New(ctx);
  detail::PairVector cap;
  const uint32_t size = str->size();
  if (size == 0) {
    if (get<2>(detail::RegExpMatch(*str, 0, *reg, &cap))) {
      return ary;
    }
    ary->DefineOwnPropertyWithIndex(
        ctx, 0,
        DataDescriptor(str,
                       PropertyDescriptor::WRITABLE |
                       PropertyDescriptor::ENUMERABLE |
                       PropertyDescriptor::CONFIGURABLE),
        false, ERROR(e));
    return ary;
  }

  uint32_t p = 0;
  uint32_t q = p;
  uint32_t start_match = 0;
  uint32_t length = 0;
  while (q != size) {
    const detail::MatchResult rs = detail::RegExpMatch(*str, q, *reg, &cap);
    if (!get<2>(rs) ||
        size == (start_match = get<0>(rs))) {
        break;
    }
    const uint32_t end = get<1>(rs);
    if (q == end && end == p) {
      ++q;
    } else {
      ary->DefineOwnPropertyWithIndex(
          ctx, length,
          DataDescriptor(JSString::New(ctx,
                                       str->begin() + p,
                                       str->begin() + start_match),
                         PropertyDescriptor::WRITABLE |
                         PropertyDescriptor::ENUMERABLE |
                         PropertyDescriptor::CONFIGURABLE),
          false, ERROR(e));
      ++length;
      if (length == lim) {
        return ary;
      }

      uint32_t i = 0;
      for (detail::PairVector::const_iterator it = cap.begin(),
           last = cap.end(); it != last; ++it) {
        ++i;
        if (it->first != -1 && it->second != -1) {
          ary->DefineOwnPropertyWithIndex(
              ctx, length,
              DataDescriptor(
                  JSString::New(ctx,
                                str->begin() + it->first,
                                str->begin() + it->second),
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE |
                             PropertyDescriptor::CONFIGURABLE),
              false, ERROR(e));
        } else {
          ary->DefineOwnPropertyWithIndex(
              ctx, length,
              DataDescriptor(JSUndefined,
                             PropertyDescriptor::WRITABLE |
                             PropertyDescriptor::ENUMERABLE |
                             PropertyDescriptor::CONFIGURABLE),
              false, ERROR(e));
        }
        ++length;
        if (length == lim) {
          return ary;
        }
      }
      q = p = end;
    }
  }
  ary->DefineOwnPropertyWithIndex(
      ctx, length,
      DataDescriptor(
          JSString::New(ctx,
                        str->begin() + p,
                        str->begin() + size),
          PropertyDescriptor::WRITABLE |
          PropertyDescriptor::ENUMERABLE |
          PropertyDescriptor::CONFIGURABLE),
      false, ERROR(e));
  return ary;
}

// section 15.5.4.15 String.prototype.substring(start, end)
inline JSVal StringSubString(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.substring", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  Context* const ctx = args.ctx();
  const JSString* const str = val.ToString(ctx, ERROR(error));
  const uint32_t len = str->size();
  uint32_t start;
  if (args.size() > 0) {
    double integer = args[0].ToNumber(ctx, ERROR(error));
    integer = core::DoubleToInteger(integer);
    start = core::DoubleToUInt32(
        std::min<double>(std::max<double>(integer, 0.0), len));
  } else {
    start = 0;
  }

  uint32_t end;
  if (args.size() > 1) {
    if (args[1].IsUndefined()) {
      end = len;
    } else {
      double integer = args[1].ToNumber(ctx, ERROR(error));
      integer = core::DoubleToInteger(integer);
      end = core::DoubleToUInt32(
          std::min<double>(std::max<double>(integer, 0.0), len));
    }
  } else {
    end = len;
  }
  const uint32_t from = std::min<uint32_t>(start, end);
  const uint32_t to = std::max<uint32_t>(start, end);
  return JSString::New(ctx,
                       str->begin() + from,
                       str->begin() + to);
}

// section 15.5.4.16 String.prototype.toLowerCase()
inline JSVal StringToLowerCase(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.toLowerCase", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  StringBuilder builder;
  for (JSString::const_iterator it = str->begin(),
       last = str->end(); it != last; ++it) {
    builder.Append(core::character::ToLowerCase(*it));
  }
  return builder.Build(args.ctx());
}

// section 15.5.4.17 String.prototype.toLocaleLowerCase()
inline JSVal StringToLocaleLowerCase(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.toLocaleLowerCase", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  StringBuilder builder;
  for (JSString::const_iterator it = str->begin(),
       last = str->end(); it != last; ++it) {
    builder.Append(core::character::ToLowerCase(*it));
  }
  return builder.Build(args.ctx());
}

// section 15.5.4.18 String.prototype.toUpperCase()
inline JSVal StringToUpperCase(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.toUpperCase", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  StringBuilder builder;
  for (JSString::const_iterator it = str->begin(),
       last = str->end(); it != last; ++it) {
    builder.Append(core::character::ToUpperCase(*it));
  }
  return builder.Build(args.ctx());
}

// section 15.5.4.19 String.prototype.toLocaleUpperCase()
inline JSVal StringToLocaleUpperCase(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.toLocaleUpperCase", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  StringBuilder builder;
  for (JSString::const_iterator it = str->begin(),
       last = str->end(); it != last; ++it) {
    builder.Append(core::character::ToUpperCase(*it));
  }
  return builder.Build(args.ctx());
}

// section 15.5.4.20 String.prototype.trim()
inline JSVal StringTrim(const Arguments& args, Error* error) {
  CONSTRUCTOR_CHECK("String.prototype.trim", args, error);
  const JSVal& val = args.this_binding();
  val.CheckObjectCoercible(ERROR(error));
  const JSString* const str = val.ToString(args.ctx(), ERROR(error));
  JSString::const_iterator lit = str->begin();
  const JSString::const_iterator last = str->end();
  // trim leading space
  bool empty = true;
  for (; lit != last; ++lit) {
    if (!detail::IsTrimmed(*lit)) {
      empty = false;
      break;
    }
  }
  if (empty) {
    return JSString::NewEmptyString(args.ctx());
  }
  // trim tailing space
  JSString::const_iterator rit = str->end() - 1;
  for (; rit != lit; --rit) {
    if (!detail::IsTrimmed(*rit)) {
      break;
    }
  }
  return JSString::New(args.ctx(), lit, rit + 1);
}

} } }  // namespace iv::lv5::runtime
#endif  // _IV_LV5_RUNTIME_STRING_H_
