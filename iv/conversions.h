#ifndef IV_CONVERSIONS_H_
#define IV_CONVERSIONS_H_
#include <cstdio>
#include <cmath>
#include <string>
#include <limits>
#include <numeric>
#include <vector>
#include <iv/detail/array.h>
#include <iv/detail/cstdint.h>
#include <iv/detail/cinttypes.h>
#include <iv/platform_math.h>
#include <iv/canonicalized_nan.h>
#include <iv/character.h>
#include <iv/conversions_digit.h>
#include <iv/digit_iterator.h>
#include <iv/ustringpiece.h>
#include <iv/none.h>
namespace iv {
namespace core {
namespace detail {

static const std::string kInfinityString = "Infinity";
static const double kDoubleToInt32_Two32 = 4294967296.0;
static const double kDoubleToInt32_Two31 = 2147483648.0;
static const int kMaxSignificantDigits = 772;

}  // namespace iv::core::detail

static const char* kHexDigits = "0123456789abcdefghijklmnopqrstuvwxyz";

static const double kDoubleIntegralPrecisionLimit =
  static_cast<uint64_t>(1) << 53;

template<typename CharT>
inline double ParseIntegerOverflow(const CharT* it,
                                   const CharT* last, int radix) {
  double number = 0.0;
  double multiplier = 1.0;
  for (--it, --last; last != it; --last) {
    if (multiplier == kInfinity) {
      if (*last != '0') {
        number = kInfinity;
        break;
      }
    } else {
      const int digit = Radix36Value(*last);
      number += digit * multiplier;
    }
    multiplier *= radix;
  }
  return number;
}

template<typename CharT>
inline double StringToIntegerWithRadix(const CharT* it, const CharT* last,
                                       int radix, bool strip_prefix) {
  // remove leading white space
  while (it != last &&
         (character::IsWhiteSpace(*it) || character::IsLineTerminator(*it))) {
    ++it;
  }

  // empty string ""
  if (it == last) {
    return kNaN;
  }

  int sign = 1;
  if (*it == '-') {
    sign = -1;
    ++it;
  } else if (*it == '+') {
    ++it;
  }

  if (it == last) {
    return kNaN;
  }

  if (strip_prefix) {
    if (*it == '0') {
      ++it;
      if (it != last && (*it == 'x' || *it == 'X')) {
        // strip_prefix
        ++it;
        radix = 16;
      } else {
        --it;
      }
    }
  }

  if (it == last) {
    return kNaN;
  }

  double result = 0.0;
  const CharT* start = it;
  for (; it != last; ++it) {
    const int val = Radix36Value(*it);
    if (val != -1 && val < radix) {
      result = result * radix + val;
    } else {
      return (start == it) ? kNaN : sign * result;
    }
  }

  if (result < kDoubleIntegralPrecisionLimit) {
    // result is precise.
    return sign * result;
  }

  if (radix == 10) {
    std::vector<char> buffer(start, last);
    buffer.push_back('\0');
    return sign * std::atof(buffer.data());
  } else if ((radix & (radix - 1)) == 0) {
    // binary radix
    return sign * ParseIntegerOverflow(start, last, radix);
  }

  return sign * result;
}

inline double StringToIntegerWithRadix(const StringPiece& piece,
                                       int radix, bool strip_prefix) {
  return StringToIntegerWithRadix(piece.data(),
                                  piece.data() + piece.size(),
                                  radix, strip_prefix);
}

inline double StringToIntegerWithRadix(const UStringPiece& piece,
                                       int radix, bool strip_prefix) {
  return StringToIntegerWithRadix(piece.data(),
                                  piece.data() + piece.size(),
                                  radix, strip_prefix);
}

inline std::size_t StringToHash(const UStringPiece& x) {
  std::size_t len = x.size();
  std::size_t step = (len >> 5) + 1;
  std::size_t h = 0;
  for (std::size_t l1 = len; l1 >= step; l1 -= step) {
    h = h ^ ((h << 5) + (h >> 2) + x[l1-1]);
  }
  return h;
}

inline std::size_t StringToHash(const StringPiece& x) {
  std::size_t len = x.size();
  std::size_t step = (len >> 5) + 1;
  std::size_t h = 0;
  for (std::size_t l1 = len; l1 >= step; l1 -= step) {
    h = h ^ ((h << 5) + (h >> 2) + x[l1-1]);
  }
  return h;
}

inline int32_t DoubleToInt32(double d) {
  int32_t i = static_cast<int32_t>(d);
  if (static_cast<double>(i) == d) {
    return i;
  }
  if (!IsFinite(d) || d == 0) {
    return 0;
  }
  if (d < 0 || d >= detail::kDoubleToInt32_Two32) {
    d = Modulo(d, detail::kDoubleToInt32_Two32);
  }
  d = (d >= 0) ?
      std::floor(d) : std::ceil(d) + detail::kDoubleToInt32_Two32;
  return static_cast<int32_t>(d >= detail::kDoubleToInt32_Two31 ?
                              d - detail::kDoubleToInt32_Two32 : d);
}

inline uint32_t DoubleToUInt32(double d) {
  return static_cast<uint32_t>(DoubleToInt32(d));
}

inline int64_t DoubleToInt64(double d) {
  int64_t i = static_cast<int64_t>(d);
  if (static_cast<double>(i) == d) {
    return i;
  }
  if (!IsFinite(d) || d == 0) {
    return 0;
  }
  if (detail::kDoubleToInt32_Two32 >= d) {
    return static_cast<int64_t>(DoubleToInt32(d));
  }
  const int32_t lo = DoubleToInt32(
      Modulo(d, detail::kDoubleToInt32_Two32));
  const int32_t hi = DoubleToInt32(d / detail::kDoubleToInt32_Two32);
  return hi * INT64_C(4294967296) + lo;
}

inline uint64_t DoubleToUInt64(double d) {
  return static_cast<uint64_t>(DoubleToInt64(d));
}

inline double DoubleToInteger(double d) {
  if (IsNaN(d)) {
    return 0;
  }
  if (!IsFinite(d) || d == 0) {
    return d;
  }
  return std::floor(std::abs(d)) * (Signbit(d) ? -1 : 1);
}

template<typename Iter>
inline bool ConvertToUInt32(Iter it, const Iter last, uint32_t* value) {
  static const uint32_t uint32_t_max = std::numeric_limits<uint32_t>::max();
  uint16_t ch;
  *value = 0;
  if (it != last && *it == '0') {
    if (it + 1 != last) {
      return false;
    } else {
      *value = 0;
      return true;
    }
  }
  if (it != last && character::IsDecimalDigit(*it)) {
    ch = *it - '0';
    *value = ch;
  } else {
    return false;
  }
  ++it;
  uint32_t prev = *value;
  for (;it != last; ++it) {
    prev = *value;
    if (character::IsDecimalDigit(*it)) {
      ch = *it - '0';
      *value = ch + (prev * 10);
    } else {
      return false;
    }
  }
  return (prev < (uint32_t_max / 10) ||
          ((prev == (uint32_t_max / 10)) && (ch <= (uint32_t_max % 10))));
}

inline bool ConvertToUInt32(const UStringPiece& str, uint32_t* value) {
  return ConvertToUInt32(str.begin(), str.end(), value);
}

inline bool ConvertToUInt32(const StringPiece& str, uint32_t* value) {
  return ConvertToUInt32(str.begin(), str.end(), value);
}

template<typename OutputIter>
inline OutputIter Int32ToString(int32_t integer, OutputIter res) {
  std::array<char, 10> buf;
  // INT32_MAX => 2147483647
  // INT32_MIN => -2147483648
  //
  // -INT32_MIN is overflowed, treat negative / positive separately
  int integer_pos = buf.size();
  if (integer >= 0) {
    do {
      buf[--integer_pos] = (integer % 10) + '0';
      integer /= 10;
    } while (integer > 0);
  } else {
    do {
      buf[--integer_pos] = (-(integer % 10)) + '0';
      integer /= 10;
    } while (integer < 0);
    *res++ = '-';
  }
  assert(integer_pos >= 0);
  return std::copy(buf.begin() + integer_pos, buf.end(), res);
}

template<typename OutputIter>
inline OutputIter UInt32ToString(uint32_t integer, OutputIter res) {
  // UINT32_MAX => 4294967295 length: 10
  std::array<char, 10> buf;
  int integer_pos = buf.size();
  do {
    buf[--integer_pos] = (integer % 10) + '0';
    integer /= 10;
  } while (integer > 0);
  assert(integer_pos >= 0);
  return std::copy(buf.begin() + integer_pos, buf.end(), res);
}

template<typename OutputIter>
inline OutputIter DoubleToStringWithRadix(double v, int radix, OutputIter res) {
  const int kMaxBufSize = 1100;
  const int kMaxDoubleToStringWithRadixBufferSize = 2200;
  std::array<char, kMaxDoubleToStringWithRadixBufferSize> buffer;
  const bool is_negative = v < 0.0;
  if (is_negative) {
    v = -v;
  }
  double integer = std::floor(v);
  double decimal = v - integer;

  // integer part
  int integer_pos = kMaxBufSize - 1;
  do {
    buffer[integer_pos--] =
        kHexDigits[static_cast<std::size_t>(Modulo(integer, radix))];
    integer /= radix;
  } while (integer >= 1.0);
  if (is_negative) {
    buffer[integer_pos--] = '-';
  }
  assert(integer_pos >= 0);

  // decimal part
  int decimal_pos = kMaxBufSize;
  if (decimal) {
    buffer[decimal_pos++] = '.';
    while ((decimal > 0.0) && (decimal_pos < (kMaxBufSize * 2))) {
      decimal *= radix;
      const std::size_t res = static_cast<std::size_t>(std::floor(decimal));
      buffer[decimal_pos++] = kHexDigits[res];
      decimal -= res;
    }
  }
  return std::copy(buffer.data() + integer_pos + 1,
                   buffer.data() + decimal_pos, res);
}

inline std::string DoubleToStringWithRadix(double v, int radix) {
  std::string str;
  DoubleToStringWithRadix(v, radix, std::back_inserter(str));
  return str;
}

template<typename Iter>
inline double StringToDouble(Iter it, Iter last, bool parse_float) {
  bool is_decimal = true;
  bool is_signed = false;
  bool is_sign_found = false;
  bool is_found_zero = false;
  std::size_t pos = 0;
  int significant_digits = 0;
  int insignificant_digits = 0;
  std::array<char, detail::kMaxSignificantDigits+10> buffer;

  // empty string ""
  if (it == last) {
    return (parse_float) ? kNaN : 0;
  }

  while (it != last &&
         (character::IsWhiteSpace(*it) || character::IsLineTerminator(*it))) {
    ++it;
  }

  // white space only "  "
  if (it == last) {
    return (parse_float) ? kNaN : 0;
  }

  if (*it == '-') {
    ++it;
    is_signed = true;
    is_sign_found = true;
  } else if (*it == '+') {
    ++it;
    is_sign_found = true;
  }
  const int sign = (is_signed) ? -1 : 1;

  if (it == last) {
    return kNaN;
  }

  if (character::IsDecimalDigit(*it)) {
    if (*it == '0') {
      is_found_zero = true;
      ++it;
      if (it == last) {
        return sign * 0.0;
      }
      if (!parse_float && (*it == 'x' || *it == 'X')) {
        if (is_sign_found) {
          return kNaN;
        }
        assert(pos == 0);
        is_decimal = false;
        buffer[pos++] = '0';
        buffer[pos++] = static_cast<char>(*it);
        ++it;
        ++significant_digits;
        if (it == last || !character::IsHexDigit(*it)) {
          return kNaN;
        }
        // waste leading zero
        while (it != last && *it == '0') {
          ++it;
        }
        while (it != last && character::IsHexDigit(*it)) {
          if (significant_digits < detail::kMaxSignificantDigits) {
            buffer[pos++] = static_cast<char>(*it);
            ++it;
            ++significant_digits;
          } else {
            ++it;
          }
        }
      } else {
        // waste leading zero
        while (it != last && *it == '0') {
          ++it;
        }
      }
    }
    if (is_decimal) {
      while (it != last && character::IsDecimalDigit(*it)) {
        if (significant_digits < detail::kMaxSignificantDigits) {
          buffer[pos++] = static_cast<char>(*it);
          ++significant_digits;
        } else {
          ++insignificant_digits;
        }
        ++it;
      }
      if (it != last && *it == '.') {
        buffer[pos++] = '.';
        ++it;
        while (it != last && character::IsDecimalDigit(*it)) {
          if (significant_digits < detail::kMaxSignificantDigits) {
            buffer[pos++] = static_cast<char>(*it);
            ++significant_digits;
          }
          ++it;
        }
      }
    }
  } else {
    if (*it == '.') {
      buffer[pos++] = '.';
      ++it;
      const Iter start = it;
      while (it != last &&
             character::IsDecimalDigit(*it)) {
        if (significant_digits < detail::kMaxSignificantDigits) {
          buffer[pos++] = static_cast<char>(*it);
          ++significant_digits;
        }
        ++it;
      }
      if (start == it) {
        return kNaN;
      }
    } else {
      for (std::string::const_iterator inf_it = detail::kInfinityString.begin(),
           inf_last = detail::kInfinityString.end();
           inf_it != inf_last; ++inf_it, ++it) {
        if (it == last || (*inf_it) != (*it)) {
          return kNaN;
        }
      }
      // infinity
      while (it != last &&
             (character::IsWhiteSpace(*it) ||
              character::IsLineTerminator(*it))) {
        ++it;
      }
      if (it == last || parse_float) {
        return sign * std::numeric_limits<double>::infinity();
      } else {
        return kNaN;
      }
    }
  }

  // exponent part
  if (it != last && (*it == 'e' || *it == 'E')) {
    if (!is_decimal) {
      return kNaN;
    }
    buffer[pos++] = static_cast<char>(*it);
    ++it;
    if (it == last) {
      if (parse_float) {
        --it;
        --pos;
        goto exponent_pasing_done;
      }
      return kNaN;
    }
    bool is_signed_exp = false;
    if (*it == '+' || *it == '-') {
      buffer[pos++] = static_cast<char>(*it);
      ++it;
      is_signed_exp = true;
    }
    if (it == last || !character::IsDecimalDigit(*it)) {
      if (parse_float) {
        --it;
        --pos;
        if (is_signed_exp) {
          --it;
          --pos;
        }
        goto exponent_pasing_done;
      }
      return kNaN;
    }
    int exponent = 0;
    do {
      if (exponent > 9999) {
        exponent = 9999;
      } else {
        exponent = exponent * 10 + (*it - '0');
      }
      ++it;
    } while (it != last && character::IsDecimalDigit(*it));
    exponent+=insignificant_digits;
    if (exponent > 9999) {
      exponent = 9999;
    }
    pos = Int32ToString(exponent, buffer.data() + pos) - buffer.data();
  }

  // exponent_pasing_done label
  exponent_pasing_done:

  while (it != last &&
         (character::IsWhiteSpace(*it) || character::IsLineTerminator(*it))) {
    ++it;
  }

  if (it == last || parse_float) {
    if (pos == 0) {
      // empty
      return (parse_float && !is_found_zero) ? kNaN : (sign * 0);
    } else if (is_decimal) {
      buffer[pos++] = '\0';
      return sign * std::atof(buffer.data());
    } else {
      // hex values
      return sign* ParseIntegerOverflow(buffer.data() + 2,
                                        buffer.data() + pos, 16);
    }
  } else {
    return kNaN;
  }
}

inline double StringToDouble(const StringPiece& str, bool parse_float) {
  return StringToDouble(str.begin(), str.end(), parse_float);
}

inline double StringToDouble(const UStringPiece& str, bool parse_float) {
  return StringToDouble(str.begin(), str.end(), parse_float);
}

template<typename U16OutputIter>
inline U16OutputIter UnicodeSequenceEscape(U16OutputIter out,
                                           uint16_t val,
                                           const StringPiece& prefix = "\\u") {
  std::array<char, 4> buf = { { } };
  out = std::copy(prefix.begin(), prefix.end(), out);
  for (int i = 0; i < 4; ++i) {
    buf[3 - i] = core::kHexDigits[val % 16];
    val /= 16;
  }
  return std::copy(buf.begin(), buf.end(), out);
}

template<typename U16OutputIter>
inline U16OutputIter JSONQuote(U16OutputIter out, uint16_t ch) {
  if (ch == '"' || ch == '\\') {
    *out++ = '\\';
    *out++ = ch;
  } else if (ch == '\b' ||
             ch == '\f' ||
             ch == '\n' ||
             ch == '\r' ||
             ch == '\t') {
    *out++ = '\\';
    switch (ch) {
      case '\b':
        *out++ = 'b';
        break;

      case '\f':
        *out++ = 'f';
        break;

      case '\n':
        *out++ = 'n';
        break;

      case '\r':
        *out++ = 'r';
        break;

      case '\t':
        *out++ = 't';
        break;
    }
  } else if (ch < ' ') {
    return UnicodeSequenceEscape(out, ch);
  } else {
    *out++ = ch;
  }
  return out;
}

struct JSONQuoteFunctor {
  template<typename MemoType, typename CharType>
  MemoType operator()(MemoType memo, CharType ch) const {
    return JSONQuote(memo, ch);
  }
};

template<typename U8OrU16InputIter, typename U16OutputIter>
inline U16OutputIter JSONQuote(U8OrU16InputIter it,
                               U8OrU16InputIter last, U16OutputIter out) {
  return std::accumulate(it, last, out, JSONQuoteFunctor());
}


template<typename U16OutputIter>
inline U16OutputIter RegExpEscape(U16OutputIter out,
                                  uint16_t ch, bool previous_is_backslash) {
  // not handling '\' and handling \u2028 or \u2029 to unicode escape sequence
  if (character::IsLineOrParagraphSeparator(ch)) {
    return UnicodeSequenceEscape(out,
                                 ch, (previous_is_backslash) ? "u" : "\\u");
  } else if (ch == '\n' || ch == '\r') {
    // these are LineTerminator
    if (!previous_is_backslash) {
      *out++ = '\\';
    }
    if (ch == '\n') {
      *out++ = 'n';
    } else {
      *out++ = 'r';
    }
  } else {
    *out++ = ch;
  }
  return out;
}

// If provided string is passed in RegExp parser, this function provides valid
// escaped string. But, if provided string is invalid for RegExp, result of this
// function isn't guaranteed.
template<typename U8OrU16InputIter, typename U16OutputIter>
inline U16OutputIter RegExpEscape(U8OrU16InputIter it,
                                  U8OrU16InputIter last, U16OutputIter out) {
  // allow / in [] (see Lexer#ScanRegExpLiteral)
  // and, LineTerminator is not allowed in RegExpLiteral (see grammar), so
  // escape it.
  bool character_in_brack = false;
  bool previous_is_backslash = false;
  for (; it != last; ++it) {
    const uint16_t ch = *it;
    if (!previous_is_backslash) {
      if (character_in_brack) {
        if (ch == ']') {
          character_in_brack = false;
        }
      } else {
        if (ch == '/') {
          *out++ = '\\';
        } else if (ch == '[') {
          character_in_brack = true;
        }
      }
      out = RegExpEscape(out, ch, previous_is_backslash);
      previous_is_backslash = ch == '\\';
    } else {
      // if new RegExp("\\\n") is provided, create /\n/
      out = RegExpEscape(out, ch, previous_is_backslash);
      // prevent like /\\[/]/
      previous_is_backslash = false;
    }
  }
  return out;
}

} }  // namespace iv::core
#endif  // IV_CONVERSIONS_H_
