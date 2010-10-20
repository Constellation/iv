#ifndef _IV_LV5_HINT_H_
#define _IV_LV5_HINT_H_
namespace iv {
namespace lv5 {
struct Hint {
  enum Object {
    NONE = 0,
    STRING = 1,
    NUMBER = 2
  };
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_HINT_H_
