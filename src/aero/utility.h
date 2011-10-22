#ifndef IV_AERO_UTILITY_H_
#define IV_AERO_UTILITY_H_
namespace iv {
namespace aero {

template<typename Iter>
inline uint8_t Load1Bytes(Iter ptr) {
  return *ptr;
}

template<typename Iter>
inline uint16_t Load2Bytes(Iter ptr) {
  return (static_cast<uint16_t>(*ptr) << 8) | *(ptr + 1);
}

template<typename Iter>
inline uint32_t Load4Bytes(Iter ptr) {
  return
      (static_cast<uint32_t>(*ptr) << 24) |
      (static_cast<uint32_t>(*(ptr + 1)) << 16) |
      (static_cast<uint32_t>(*(ptr + 2)) << 8) | *(ptr + 3);
}

} }  // namespace iv::aero
#endif  // IV_AERO_UTILITY_H_
