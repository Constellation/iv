#ifndef _IV_UNICODE_H_
#define _IV_UNICODE_H_
#include <cassert>
#include <tr1/cstdint>
#include <tr1/array>
namespace iv {
namespace core {
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
//static const std::tr1::array<uint8_t, 0x40> kUTF8Table4 = { {
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
//  3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
//} };

}  // namespace iv::core::detail


inline int UCS4ToUTF8(uint32_t uc, uint8_t* buf) {
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

inline uint32_t UTF8ToUCS4(const uint8_t* buf, uint32_t size) {
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

inline uint32_t UTF8ToUCS4Strict(const uint8_t* buf, uint32_t size, bool* e) {
  static const std::tr1::array<uint32_t, 3> kMinTable = { {
    0x00000080, 0x00000800, 0x00010000
  } };
  // not accept size 5 or 6
  assert(size >= 1 && size <= 4);
  // 1st octet format is checked by Decode function in calculating size
  if (size == 1) {
    *e = IsUTF8Malformed(*buf);
    return *buf;
  } else {
    // remove size bits from 1st
    uint32_t uc = (*buf++) & ((1 << (7 - size)) - 1);
    // select minimum size code point for size
    const uint32_t min = kMinTable[size - 2];
    while (--size) {
      const uint8_t current = *buf++;
      // UTF8 String (not 1st) should be 10xxxxxx
      // 0xC0 => (11000000) 0x80 => (10000000)
      // and, not malformed utf8 octet
      if (((current & 0xC0) != 0x80) || IsUTF8Malformed(current)) {
        *e = true;
        return *buf;
      }
      uc = uc << 6 | (current & 0x3F);
    }
    *e = uc < min;
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

inline bool IsHighSurrogate(uint16_t uc) {
  return (static_cast<uint32_t>(uc) & ~kHighSurrogateMask) == kHighSurrogateMin;
}

inline bool IsLowSurrogate(uint16_t uc) {
  return (static_cast<uint32_t>(uc) & ~kLowSurrogateMask) == kLowSurrogateMin;
}

// This function takes an integer value in the range 0 - 0x7fffffff
// and encodes it as a UTF-8 character in 0 to 6 bytes.
//
// Arguments:
// cvalue     the character value
// buffer     pointer to buffer for result - at least 6 bytes long
//
// Returns:     number of characters placed in the buffer
inline int UTF16CodeToUTF8(int cvalue, uint8_t* buffer) {
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

} }  // namespace iv::core
#endif  // _IV_UNICODE_H_
