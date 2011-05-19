#ifndef _IV_UNICODE_H_
#define _IV_UNICODE_H_
#include <cassert>
#include <iterator>
#include <tr1/cstdint>
#include <tr1/array>
namespace iv {
namespace core {
namespace unicode {
namespace detail {

static const std::tr1::array<int, 6> kUTF8Table1 = { {
  0x7f, 0x7ff, 0xffff, 0x1fffff, 0x3ffffff, 0x7fffffff
} };

static const std::tr1::array<int, 6> kUTF8Table2 = { {
  0,    0xc0, 0xe0, 0xf0, 0xf8, 0xfc
} };

static const std::tr1::array<int, 6> kUTF8Table3 = { {
  0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01
} };

// length table
static const std::tr1::array<uint8_t, 0x40> kUTF8Table4 = { {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
} };

}  // namespace iv::core::detail

template<typename OutputIter>
inline int UCS4ToUTF8(uint32_t uc, OutputIter buf) {
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

template<typename InputIter>
inline uint32_t UTF8ToUCS4(InputIter buf, uint32_t size) {
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

inline bool IsUTF8Malformed(uint8_t c) {
  return c == 0xc0 || c == 0xc1 || c >= 0xf5;
}

template<typename InputIter>
inline uint32_t UTF8ToUCS4Strict(InputIter buf, uint32_t size, bool* e) {
  static const std::tr1::array<uint32_t, 3> kMinTable = { {
    0x00000080,  // 2 bytes => 0000000010000000
    0x00000800,  // 3 bytes => 0000100000000000
    0x00010000   // 4 bytes => surrogate pair only
  } };
  // not accept size 5 or 6
  assert(size >= 1 && size <= 4);
  // 1st octet format is checked by Decode function in calculating size
  if (size == 1) {
    *e = (*buf & 0x80);
    return *buf;
  } else {
    // remove size bits from 1st
    uint32_t uc = (*buf++) & ((1 << (7 - size)) - 1);
    // select minimum size code point for size
    const uint32_t min = kMinTable[size - 2];
    while (--size) {
      const uint8_t current = *buf++;
      // UTF8 String (not 1st) should be 10xxxxxx (UTF8-tail)
      // 0xC0 => (11000000) 0x80 => (10000000)
      if ((current & 0xC0) != 0x80) {
        *e = true;
        return current;
      }
      uc = uc << 6 | (current & 0x3F);
    }
    *e = uc < min || uc > 0x10FFFF;
    return uc;
  }
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
  return BitMask<N, CharT>::lower & ch;
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

// This function takes an integer value in the range 0 - 0x7fffffff
// and encodes it as a UTF-8 character in 0 to 6 bytes.
//
// Arguments:
// cvalue     the character value
// buffer     pointer to buffer for result - at least 6 bytes long
//
// Returns:     number of characters placed in the buffer
template<typename OutputIter>
inline int UTF16CodeToUTF8(int cvalue, OutputIter buffer) {
  int i = 0;
  for (const int len = detail::kUTF8Table1.size(); i < len; i++) {
    if (cvalue <= detail::kUTF8Table1[i]) {
      break;
    }
  }
  buffer += i;
  for (int j = i; j > 0; j--) {
    *buffer-- = 0x80 | (cvalue & 0x3f);
    cvalue >>= 6;
  }
  *buffer = detail::kUTF8Table2[i] | cvalue;
  return i + 1;
}

} } }  // namespace iv::core::unicode
#endif  // _IV_UNICODE_H_
