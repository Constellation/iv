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
  int counters() const {
    return counters_;
  }
  int captures() const {
    return captures_;
  }

 private:
  std::vector<uint8_t> bytes_;
  int captures_;
  int counters_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_CODE_H_
