#ifndef _IV_ICU_USTREAM_H_
#define _IV_ICU_USTREAM_H_
#include <iostream>  // NOLINT
#include <tr1/array>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/uchar.h>
#include "uchar.h"

inline std::ostream& operator<<(std::ostream& os, const iv::uc16* str) {
  if (str) {
    std::tr1::array<char, 200> buffer;
    UErrorCode error = U_ZERO_ERROR;
    UConverter* conv = ucnv_open(NULL, &error);
    if (U_FAILURE(error)) {
      ucnv_close(conv);
    } else {
      const iv::uc16* limit = str + std::char_traits<iv::uc16>::length(str);
      char* s;
      const char* slimit = buffer.data() + buffer.size();
      do {
        error = U_ZERO_ERROR;
        s = buffer.data();
        ucnv_fromUnicode(conv, &s, slimit, &str, limit, 0, false, &error);
        if (s > buffer.data()) {
          os.write(buffer.data(), std::distance(buffer.data(), s));
        }
      } while (error == U_BUFFER_OVERFLOW_ERROR);
      ucnv_close(conv);
    }
  }
  return os;
}

#endif  // _IV_ICU_USTREAM_H_
