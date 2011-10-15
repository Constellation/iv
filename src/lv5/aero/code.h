#ifndef IV_LV5_AERO_CODE_H_
#define IV_LV5_AERO_CODE_H_
namespace iv {
namespace lv5 {
namespace aero {

static const int kUndefined = -1;

class Code {
 public:
  Code(const std::vector<uint8_t>& vec, int captures, int counters)
    : bytes_(vec),
      captures_(captures),
      counters_(counters) { }

  const std::vector<uint8_t> bytes() const { return bytes_; }

  const uint8_t* data() const { return bytes_.data(); }

  int counters() const { return counters_; }

  int captures() const { return captures_; }

 private:
  std::vector<uint8_t> bytes_;
  int captures_;
  int counters_;
};

} } }  // namespace iv::lv5::aero
#endif  // IV_LV5_AERO_CODE_H_
