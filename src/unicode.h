#ifndef _IV_UNICODE_H_
#define _IV_UNICODE_H_
#include <cassert>
#include <cstdio>
#include <iosfwd>
#include <iterator>
#include <string>
#include <algorithm>
#include <tr1/cstdint>
#include <tr1/array>
namespace iv {
namespace core {
namespace unicode {

inline bool IsUTF8Malformed(uint8_t c) {
  return c == 0xc0 || c == 0xc1 || c >= 0xf5;
}

static const uint32_t kSurrogateBits = 10;

static const uint32_t kHighSurrogateMin = 0xD800;
static const uint32_t kHighSurrogateMax = 0xDBFF;
static const uint32_t kHighSurrogateMask = (1 << kSurrogateBits) - 1;

static const uint32_t kLowSurrogateMin = 0xDC00;
static const uint32_t kLowSurrogateMax = 0xDFFF;
static const uint32_t kLowSurrogateMask = (1 << kSurrogateBits) - 1;

static const uint32_t kSurrogateMin = kHighSurrogateMin;
static const uint32_t kSurrogateMax = kLowSurrogateMax;
static const uint32_t kSurrogateMask = (1 << (kSurrogateBits + 1)) - 1;

static const uint32_t kHighSurrogateOffset = kHighSurrogateMin - (0x10000 >> 10);
static const uint32_t kSurrogateOffset = 0x10000 - (kHighSurrogateMin << 10) - kLowSurrogateMin;

static const uint32_t kUnicodeMin = 0x000000;
static const uint32_t kUnicodeMax = 0x10FFFF;

static const uint32_t kUTF16Min = 0x0000;
static const uint32_t kUTF16Max = 0xFFFF;

static const uint32_t kUCS2Min = 0x0000;
static const uint32_t kUCS2Max = 0xFFFF;

static const uint32_t kUCS4Min = 0x00000000;
static const uint32_t kUCS4Max = 0x7FFFFFFF;

// \uFEFF Byte Order Mark UTF-8 representation
static const std::tr1::array<uint8_t, 3> kUTF8BOM = { {
  0xEF, 0xBB, 0xBF
} };

// bit mask template
// see http://d.hatena.ne.jp/tt_clown/20090616/p1
template<std::size_t LowerBits,
         class Type = unsigned long>
struct BitMask {
  static const Type full = ~(Type(0));
  static const Type upper = ~((Type(1) << LowerBits) - 1);
  static const Type lower = (Type(1) << LowerBits) - 1;
};

template<std::size_t N, typename CharT>
inline CharT Mask(CharT ch) {
  return BitMask<N, uint32_t>::lower & ch;
}

template<typename UC16>
inline bool IsHighSurrogate(UC16 uc) {
  return (static_cast<uint32_t>(uc) & ~kHighSurrogateMask) == kHighSurrogateMin;
}

template<typename UC16>
inline bool IsLowSurrogate(UC16 uc) {
  return (static_cast<uint32_t>(uc) & ~kLowSurrogateMask) == kLowSurrogateMin;
}

template<typename UC16>
inline bool IsSurrogate(UC16 uc) {
  return (uc & ~kSurrogateMask) == kSurrogateMin;
}

template<typename UC32>
inline bool IsValidUnicode(UC32 uc) {
  return (uc <= kUnicodeMax) && !IsSurrogate(uc);
}

template<typename UC8InputIter>
inline bool IsBOM(UC8InputIter it, UC8InputIter end) {
  return (
      (it != end) && (Mask<8>(*it++) == kUTF8BOM[0]) &&
      (it != end) && (Mask<8>(*it++) == kUTF8BOM[1]) &&
      (it != end) && (Mask<8>(*it++) == kUTF8BOM[2]));
}

template<typename UC8>
inline bool IsTrail(UC8 ch) {
  // UTF8 String (not 1st) should be 10xxxxxx (UTF8-tail)
  // 0xC0 => (11000000) 0x80 => (10000000)
  return (ch & 0xC0) == 0x80;
}

enum UTF8Error {
  NO_ERROR = 0,
  NOT_ENOUGH_SPACE = 1,
  INVALID_SEQUENCE = 2
};

// see RFC3629
//
// Char. number range  |        UTF-8 octet sequence
//    (hexadecimal)    |              (binary)
// --------------------+-------------------------------------
// 0000 0000-0000 007F | 0xxxxxxx
// 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
// 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
// 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
//
// comment: use 256 table is more faster ?
template<typename UC8InputIter>
inline typename std::iterator_traits<UC8InputIter>::difference_type
UTF8ByteCount(UC8InputIter it) {
  // not use loop
  const uint8_t ch = Mask<8>(*it);
  if (ch < 0x80) {
    // ASCII range
    return 1;
  } else if ((ch >> 5) == 0x6) {
    // 110xxxxx
    return 2;
  } else if ((ch >> 4) == 0xE) {
    // 1110xxxx
    return 3;
  } else if ((ch >> 3) == 0x1E) {
    // 1110xxx
    return 4;
  }
  // invalid utf8
  return 0;
}

// no check. but scan UTF-8 Bytes and returns length of code point
template<typename UC8InputIter>
inline typename std::iterator_traits<UC8InputIter>::difference_type
UTF8ByteCharLength(UC8InputIter it, UC8InputIter last, UTF8Error* e) {
  typedef typename std::iterator_traits<UC8InputIter>::difference_type diff_type;
  diff_type sum = 0;
  while (it != last) {
    const diff_type len = UTF8ByteCount(it);
    if (len == 0) {
      *e = INVALID_SEQUENCE;
      return sum;
    }
    std::advance(it, len);
    ++sum;
  }
  return sum;
}

template<std::size_t N>
struct UTF8ToCodePoint { };

template<>
struct UTF8ToCodePoint<1> {
  template<typename UC8InputIter>
  static inline uint32_t Get(UC8InputIter it, UC8InputIter last, UTF8Error* e) {
    if (it != last) {
      return Mask<8>(*it);
    } else {
      *e = NOT_ENOUGH_SPACE;
    }
    return 0;
  }
};

template<>
struct UTF8ToCodePoint<2> {
  template<typename UC8InputIter>
  static inline uint32_t Get(UC8InputIter it, UC8InputIter last, UTF8Error* e) {
    static const uint32_t kMin = 0x00000080;  // 2 bytes => 0000000010000000
    if (it != last) {
      // remove size bits from 1st byte
      // 110xxxxx
      uint32_t res = Mask<5>(*it++) << 6;
      if (it != last) {
        if (IsTrail(*it)) {
          // max value is 11111111111 => 2047 => 0x7FF
          // so no check out of range: max
          res = res | Mask<6>(*it);
          if (res >= kMin) {
            return res;
          } else {
            *e = INVALID_SEQUENCE;
          }
        } else {
          *e = INVALID_SEQUENCE;
        }
      } else {
        *e = NOT_ENOUGH_SPACE;
      }
    } else {
      *e = NOT_ENOUGH_SPACE;
    }
    return 0;
  }
};

template<>
struct UTF8ToCodePoint<3> {
  template<typename UC8InputIter>
  static inline uint32_t Get(UC8InputIter it, UC8InputIter last, UTF8Error* e) {
    static const uint32_t kMin = 0x00000800;  // 3 bytes => 0000100000000000
    if (it != last) {
      // remove size bits from 1st byte
      // 1110xxxx
      uint32_t res = Mask<4>(*it++) << 12;
      if (it != last) {
        if (IsTrail(*it)) {
          res = res | Mask<6>(*it++) << 6;
          if (it != last) {
            if (IsTrail(*it)) {
              // max value is 1111111111111111 => 0xFFFF
              // so no check out of range: max
              res = res | Mask<6>(*it);
              if (res >= kMin) {
                return res;
              } else {
                *e = INVALID_SEQUENCE;
              }
            } else {
              *e = INVALID_SEQUENCE;
            }
          } else {
            *e = NOT_ENOUGH_SPACE;
          }
        } else {
          *e = INVALID_SEQUENCE;
        }
      } else {
        *e = NOT_ENOUGH_SPACE;
      }
    } else {
      *e = NOT_ENOUGH_SPACE;
    }
    return 0;
  }
};

template<>
struct UTF8ToCodePoint<4> {
  template<typename UC8InputIter>
  static inline uint32_t Get(UC8InputIter it, UC8InputIter last, UTF8Error* e) {
    static const uint32_t kMin = 0x00010000;  // 4 bytes => surrogate pair only
    if (it != last) {
      // remove size bits from 1st byte
      // 11110xxx
      uint32_t res = Mask<3>(*it++) << 18;
      if (it != last) {
        if (IsTrail(*it)) {
          res = res | Mask<6>(*it++) << 12;
          if (it != last) {
            if (IsTrail(*it)) {
              res = res | Mask<6>(*it++) << 6;
              if (it != last) {
                if (IsTrail(*it)) {
                  // max value is 111111111111111111111 => 0x1FFFFF
                  // so must check this value is in surrogate pair range
                  res = res | Mask<6>(*it);
                  if (res >= kMin) {
                    if (res <= 0x10FFFF) {
                      return res;
                    } else {
                      *e = INVALID_SEQUENCE;
                    }
                  } else {
                    *e = INVALID_SEQUENCE;
                  }
                } else {
                  *e = INVALID_SEQUENCE;
                }
              } else {
                *e = NOT_ENOUGH_SPACE;
              }
            } else {
              *e = INVALID_SEQUENCE;
            }
          } else {
            *e = NOT_ENOUGH_SPACE;
          }
        } else {
          *e = INVALID_SEQUENCE;
        }
      } else {
        *e = NOT_ENOUGH_SPACE;
      }
    } else {
      *e = NOT_ENOUGH_SPACE;
    }
    return 0;
  }
};

template<typename UC32OutputIter>
inline UC32OutputIter Append(uint32_t uc, UC32OutputIter result) {
  if (uc < 0x80) {
    // 0000 0000-0000 007F | 0xxxxxxx
    *result++ = static_cast<uint8_t>(uc);
  } else if (uc < 0x800) {
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    *result++ = static_cast<uint8_t>((uc >> 6) | 0xC0);
    *result++ = static_cast<uint8_t>((uc & 0x3F) | 0x80);
  } else if (uc < 0x10000) {
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    *result++ = static_cast<uint8_t>((uc >> 12) | 0xE0);
    *result++ = static_cast<uint8_t>(((uc >> 6) & 0x3F) | 0x80);
    *result++ = static_cast<uint8_t>((uc & 0x3F) | 0x80);
  } else {
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    *result++ = static_cast<uint8_t>((uc >> 18) | 0xF0);
    *result++ = static_cast<uint8_t>(((uc >> 12) & 0x3F) | 0x80);
    *result++ = static_cast<uint8_t>(((uc >> 6) & 0x3F) | 0x80);
    *result++ = static_cast<uint8_t>((uc & 0x3F) | 0x80);
  }
  return result;
}

template<typename UC8InputIter>
inline UC8InputIter NextUCS4FromUTF8(UC8InputIter it, UC8InputIter last, uint32_t* out, UTF8Error* e) {
  typedef typename std::iterator_traits<UC8InputIter>::difference_type diff_type;
  const diff_type len = UTF8ByteCount(it);
  if (len == 0) {
    *e = INVALID_SEQUENCE;
    return it;
  }
  uint32_t res;
  switch (len) {
    case 1:
      res = UTF8ToCodePoint<1>::Get(it, last, e);
      break;

    case 2:
      res = UTF8ToCodePoint<2>::Get(it, last, e);
      break;

    case 3:
      res = UTF8ToCodePoint<3>::Get(it, last, e);
      break;

    case 4:
      res = UTF8ToCodePoint<4>::Get(it, last, e);
      break;
  };
  if (*e == NO_ERROR) {
    std::advance(it, len);
  }
  *out = res;
  return it;
}

template<typename UC8InputIter, typename OutputIter>
inline UTF8Error UTF8ToUCS4(UC8InputIter it, UC8InputIter last, OutputIter result) {
  UTF8Error error = NO_ERROR;
  uint32_t res;
  while (it != last) {
    it = NextUCS4FromUTF8(it, last, &res, &error);
    if (error != NO_ERROR) {
      return error;
    } else {
      *result++ = res;
    }
  }
  return NO_ERROR;
}

template<typename UC8InputIter, typename OutputIter>
inline UTF8Error UTF8ToUTF16(UC8InputIter it, UC8InputIter last, OutputIter result) {
  UTF8Error error = NO_ERROR;
  uint32_t res;
  while (it != last) {
    it = NextUCS4FromUTF8(it, last, &res, &error);
    if (error != NO_ERROR) {
      return error;
    } else {
      if (res > 0xFFFF) {
        // surrogate pair
        *result++ = static_cast<uint16_t>((res >> kSurrogateBits) + kHighSurrogateOffset);
        *result++ = static_cast<uint16_t>((res & 0x3FF) + kLowSurrogateMin);
      } else {
        *result++ = static_cast<uint16_t>(res);
      }
    }
  }
  return NO_ERROR;
}

// return wirte length
template<typename OutputIter>
inline std::size_t UCS4OneCharToUTF8(uint32_t uc, OutputIter result) {
  OutputIter tmp = Append(uc, result);
  return std::distance(result, tmp);
}

template<typename UC32InputIter, typename OutputIter>
inline UTF8Error UCS4ToUTF8(UC32InputIter it, UC32InputIter last, OutputIter result) {
  while (it != last) {
    result = Append(*it++, result);
  }
  return NO_ERROR;
}

template<typename UTF16InputIter, typename OutputIter>
inline UTF8Error UTF16ToUTF8(UTF16InputIter it, UTF16InputIter last, OutputIter result) {
  while (it != last) {
    uint32_t res = Mask<16>(*it++);
    if (IsHighSurrogate(res)) {
      ++it;
      if (it == last) {
        return INVALID_SEQUENCE;
      }
      const uint32_t low = Mask<16>(*it++);
      if (!IsLowSurrogate(low)) {
        return INVALID_SEQUENCE;
      }
      res = (res << kSurrogateBits) + low + kSurrogateOffset;
    }
    result = Append(res, result);
  }
  return NO_ERROR;
}

template<typename UTF16InputIter>
inline int FPutsUTF16(FILE* file, UTF16InputIter it, UTF16InputIter last) {
  std::string str;
  str.reserve(std::distance(it, last));
  UTF16ToUTF8(it, last, std::back_inserter(str));
  return std::fputs(str.c_str(), file);
}

template<typename UTF16InputIter>
inline std::ostream& OutputUTF16(std::ostream& os, UTF16InputIter it, UTF16InputIter last) {
  std::string str;
  str.reserve(std::distance(it, last));
  UTF16ToUTF8(it, last, std::back_inserter(str));
  return os << str;
}

} } }  // namespace iv::core::unicode
#endif  // _IV_UNICODE_H_
