#ifndef IV_AERO_CODE_H_
#define IV_AERO_CODE_H_
namespace iv {
namespace aero {

static const int kUndefined = -1;

class Code {
 public:
  typedef std::vector<uint8_t> Data;
  Code(const std::vector<uint8_t>& vec,
       int captures, int counters, uint16_t filter, bool one_char)
    : bytes_(vec),
      captures_(captures),
      counters_(counters),
      filter_(filter),
      one_char_(one_char) {
  }

  const std::vector<uint8_t>& bytes() const { return bytes_; }

  const uint8_t* data() const { return bytes_.data(); }

  int counters() const { return counters_; }

  int captures() const { return captures_; }

  uint16_t filter() const { return filter_; }

  bool IsQuickCheckOneChar() const { return one_char_; }

 private:
  Data bytes_;
  int captures_;
  int counters_;
  uint16_t filter_;
  bool one_char_;
};

} }  // namespace iv::aero
#endif  // IV_AERO_CODE_H_
