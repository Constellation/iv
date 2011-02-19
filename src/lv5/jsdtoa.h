// code from SpiderMonkey jsdtoa.cpp
// MPL 1.1
#ifndef _IV_LV5_JSDTOA_H_
#define _IV_LV5_JSDTOA_H_
#include <cstdlib>
#include <tr1/cstdio>
#include <tr1/array>
#include <tr1/cstdint>
#include "lv5/jsstring.h"

extern "C" {
extern char *dtoa(double d, int mode, int ndigits,
                  int *decpt, int *sign, char **rve);
extern void freedtoa(char* s);
}

namespace iv {
namespace lv5 {
namespace detail {
static const std::tr1::array<uint8_t, 5> kDTOAModeList = { {
  0,  // DTOA_STD
  0,  // DTOA_STD_EXPONENTIAL
  3,  // DTOA_FIXED
  2,  // DTOA_EXPONENTIAL
  2   // DTOA_PRECISION
} };
}  // namespace iv::lv5::detail

enum DTOAMode {
  DTOA_STD = 0,
  DTOA_STD_EXPONENTIAL,
  DTOA_FIXED,
  DTOA_EXPONENTIAL,
  DTOA_PRECISION
};

template<DTOAMode m>
inline JSString* DoubleToJSString(Context* ctx, double x, int frac, int offset) {
  std::tr1::array<char, 80> buf;
  const DTOAMode mode = (m == DTOA_FIXED && (x >= 1e21 || x <= -1e21))? DTOA_STD : m;
  int decpt;
  int sign;
  int precision = frac + offset;
  char* rev;
  char* res = dtoa(x, detail::kDTOAModeList[mode],
                   precision, &decpt, &sign, &rev);
  if (!res) {
    return JSString::NewAsciiString(ctx, "NaN");
  }
  int digits = rev - res;
  char* begin = buf.data() + 2;
  std::memcpy(begin, res, digits);
  freedtoa(res);
  char* end = begin + digits;
  *end = '\0';
  if (decpt != 9999) {
    bool exp_notation = false;
    int min = 0;
    switch (mode) {
      case DTOA_STD:
        if (decpt < -5 || decpt > 21) {
          exp_notation = true;
        } else {
          min = decpt;
        }
        break;

      case DTOA_FIXED:
        if (precision >= 0) {
          min = decpt + precision;
        } else {
          min = decpt;
        }
        break;

      case DTOA_EXPONENTIAL:
        min = precision;
        // fall through

      case DTOA_STD_EXPONENTIAL:
        exp_notation = true;
        break;

      case DTOA_PRECISION:
        min = precision;
        if (decpt < -5 || decpt > 21) {
          exp_notation = true;
        }
        break;
    }
    if (digits < min) {
      const char* p = begin + min;
      digits = min;
      do {
        *end++ = '0';
      } while (end != p);
      *end = '\0';
    }
    if (exp_notation) {
      if (digits != 1) {
        // insert point (.)
        --begin;
        begin[0] = begin[1];
        begin[1] = '.';
      }
      std::tr1::snprintf(end, buf.size() - (end - buf.data()),
                         "e%+d", decpt - 1);
    } else if (decpt != digits) {
      if (decpt > 0) {
        char* p = --begin;
        do {
          *p = p[1];
          ++p;
        } while (--decpt);
        *p = '.';
      } else {
        char* p = end;
        end += (1 - decpt);
        char* q = end;
        *end = '\0';
        while (p != end) {
          *--p = *--q;
        }
        for (p = begin + 1; p != q; ++p) {
          *p = '0';
        }
        *begin = '.';
        *--begin = '0';
      }
    }
  }
  if (x < 0) {
    *--begin = '-';
  }
  return JSString::NewAsciiString(ctx, begin);
}

} }  // namespace iv::lv5
#endif  // _IV_LV5_JSDTOA_H_
