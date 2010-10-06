#ifndef IV_CONVERSIONS_INL_H_
#define IV_CONVERSIONS_INL_H_
#include <tr1/array>
#include <cstdio>
#include "conversions.h"
#include "char.h"
#include "dtoa.h"
namespace iv {
namespace core {

template<typename Iter>
inline double StringToDouble(Iter it, Iter last) {
  bool is_decimal = true;
  bool is_signed = false;
  std::size_t pos = 0;
  int significant_digits = 0;
  int insignificant_digits = 0;
  std::tr1::array<char, Conversions::kMaxSignificantDigits+10> buffer;

  // empty string ""
  if (it == last) {
    return 0;
  }

  while (it != last && (ICU::IsWhiteSpace(*it) || ICU::IsLineTerminator(*it))) {
    ++it;
  }

  // white space only "  "
  if (it == last) {
    return 0;
  }

  if (*it == '-') {
    buffer[pos++] = '-';
    ++it;
    is_signed = true;
  } else if (*it == '+') {
    ++it;
  }

  if (it == last) {
    return Conversions::kNaN;
  }

  if (ICU::IsDecimalDigit(*it)) {
    if (*it == '0') {
      ++it;
      if (it == last) {
        return 0;
      }
      if (*it == 'x' || *it == 'X') {
        is_decimal = false;
        buffer[pos++] = '0';
        buffer[pos++] = *it;
        ++it;
        ++significant_digits;
        if (it == last || !ICU::IsHexDigit(*it)) {
          return Conversions::kNaN;
        }
        // waste leading zero
        while (it != last && *it == '0') {
          ++it;
        }
        while (it != last && ICU::IsHexDigit(*it)) {
          if (significant_digits < Conversions::kMaxSignificantDigits) {
            buffer[pos++] = *it;
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
      while (it != last &&
             ICU::IsDecimalDigit(*it)) {
        if (significant_digits < Conversions::kMaxSignificantDigits) {
          buffer[pos++] = *it;
          ++significant_digits;
        } else {
          ++insignificant_digits;
        }
        ++it;
      }
      if (it != last && *it == '.') {
        buffer[pos++] = '.';
        ++it;
        while (it != last &&
               ICU::IsDecimalDigit(*it)) {
          if (significant_digits < Conversions::kMaxSignificantDigits) {
            buffer[pos++] = *it;
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
      while (it != last &&
             ICU::IsDecimalDigit(*it)) {
        if (significant_digits < Conversions::kMaxSignificantDigits) {
          buffer[pos++] = *it;
          ++significant_digits;
        }
        ++it;
      }
    } else {
      for (std::string::const_iterator inf_it = Conversions::kInfinity.begin(),
           inf_last = Conversions::kInfinity.end();
           inf_it != inf_last; ++inf_it, ++it) {
        if (it == last ||
            (*inf_it) != (*it)) {
          return Conversions::kNaN;
        }
      }
      // infinity
      while (it != last &&
             (ICU::IsWhiteSpace(*it) || ICU::IsLineTerminator(*it))) {
        ++it;
      }
      if (it == last) {
        if (is_signed) {
          return -std::numeric_limits<double>::infinity();
        } else {
          return std::numeric_limits<double>::infinity();
        }
      } else {
        return Conversions::kNaN;
      }
    }
  }

  // exponent part
  if (it != last && (*it == 'e' || *it == 'E')) {
    if (!is_decimal) {
      return Conversions::kNaN;
    }
    buffer[pos++] = *it;
    ++it;
    if (it == last) {
      return Conversions::kNaN;
    }
    if (*it == '+' || *it == '-') {
      buffer[pos++] = *it;
      ++it;
    }
    if (it == last) {
      return Conversions::kNaN;
    }
    // more than 1 decimal digit required
    if (!ICU::IsDecimalDigit(*it)) {
      return Conversions::kNaN;
    }
    int exponent = 0;
    do {
      if (exponent > 9999) {
        exponent = 9999;
      } else {
        exponent = exponent * 10 + (*it - '0');
      }
      ++it;
    } while (it != last && ICU::IsDecimalDigit(*it));
    exponent+=insignificant_digits;
    if (exponent > 9999) {
      exponent = 9999;
    }
    std::snprintf(buffer.data()+pos, 5, "%d", exponent);
    pos+=4;
  }

  while (it != last && (ICU::IsWhiteSpace(*it) || ICU::IsLineTerminator(*it))) {
    ++it;
  }

  if (it == last) {
    if (pos == 0) {
      // empty
      return 0;
    } else {
      buffer[pos++] = '\0';
      return std::strtod(buffer.data(), NULL);
    }
  } else {
    return Conversions::kNaN;
  }
}

inline double StringToDouble(const StringPiece& str) {
  return StringToDouble(str.begin(), str.end());
}

inline double StringToDouble(const UStringPiece& str) {
  return StringToDouble(str.begin(), str.end());
}

} }  // namespace iv::core
#endif  // IV_CONVERSIONS_INL_H_
