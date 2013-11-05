#ifndef IV_AERO_CODE_H_
#define IV_AERO_CODE_H_
#include <memory>
#include <iv/platform.h>
#include <iv/aero/jit_fwd.h>
namespace iv {
namespace aero {

static const int kUndefined = -1;

class Code {
 public:
  friend class VM;
  typedef std::vector<uint8_t> Data;
  Code(const std::vector<uint8_t>& vec,
       int captures, int counters, uint16_t filter, bool one_char)
    : bytes_(vec),
      captures_(captures),
      counters_(counters),
      filter_(filter),
      one_char_(one_char)
#if defined(IV_ENABLE_JIT)
      ,
      jit_(new JITCode())
#endif
      { }

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
#if defined(IV_ENABLE_JIT)
  std::unique_ptr<JITCode> jit_;
 public:
  JITCode* jit() const { return jit_.get(); }
#endif
};

} }  // namespace iv::aero
#endif  // IV_AERO_CODE_H_
