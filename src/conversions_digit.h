#ifndef _IV_CONVERSIONS_DIGIT_H_
#define _IV_CONVERSIONS_DIGIT_H_
namespace iv {
namespace core {

inline int OctalValue(const int c) {
  if ('0' <= c && c <= '8') {
    return c - '0';
  }
  return -1;
}

inline int HexValue(const int c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  }
  if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

inline int Radix36Value(const int c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  if ('a' <= c && c <= 'z') {
    return c - 'a' + 10;
  }
  if ('A' <= c && c <= 'Z') {
    return c - 'A' + 10;
  }
  return -1;
}

} }  // namespace iv::core
#endif  // _IV_CONVERSIONS_DIGIT_H_
