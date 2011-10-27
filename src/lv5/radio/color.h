#ifndef IV_LV5_RADIO_COLOR_H_
#define IV_LV5_RADIO_COLOR_H_
namespace iv {
namespace lv5 {
namespace radio {

struct Color {
  enum Type {
    WHITE = 0,
    BLACK = 1,
    GRAY  = 2
  };
  static const uintptr_t kMask = 3;  // (11)2
};

} } }  // namespace iv::lv5::radio
#endif  // IV_LV5_RADIO_COLOR_H_
