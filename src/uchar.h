#ifndef _IV_UCHAR_H_
#define _IV_UCHAR_H_
#include <tr1/cstdint>
#include "config.h"

#if USE_ICU == 1
#include <unicode/uchar.h>
#else
typedef std::tr1::uint16_t UChar;
#endif

#endif  // _IV_UTILS_H_
