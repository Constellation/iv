#ifndef IV_AERO_CODE_H_
#define IV_AERO_CODE_H_
#include <iv/platform.h>
namespace iv {
namespace aero {

#if defined(IV_ENABLE_JIT)
template<typename CharT>
class JIT;
#endif

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
      one_char_(one_char)
#if defined(IV_ENABLE_JIT)
      ,
      jit8bit_(NULL),
      jit16bit_(NULL)
#endif
      { }

#if defined(IV_ENABLE_JIT)
  // TODO(Constellation) remot destructor
  ~Code() {
    delete jit8bit_;
    delete jit16bit_;
  }
#endif

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
  JIT<char>* jit8bit_;
  JIT<uint16_t>* jit16bit_;
 public:
  void Compiled(JIT<char>* j8) { jit8bit_ = j8; }
  JIT<char>* Compiled8Bit() { return jit8bit_; }
  void Compiled(JIT<uint16_t>* j16) { jit16bit_ = j16; }
  JIT<uint16_t>* Compiled16Bit() { return jit16bit_; }
#endif
};

#if defined(IV_ENABLE_JIT)
template<typename CharT = char>
struct Compiled {
};
#endif

} }  // namespace iv::aero
#endif  // IV_AERO_CODE_H_
