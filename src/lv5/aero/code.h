#ifndef IV_LV5_AERO_CODE_H_
#define IV_LV5_AERO_CODE_H_
namespace iv {
namespace lv5 {
namespace aero {

class Code {
 public:
  const uint8_t* data() const {
    return bytes_.data();
  }
  std::vector<uint8_t> bytes_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_CODE_H_
