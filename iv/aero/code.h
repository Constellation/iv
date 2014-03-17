#ifndef IV_AERO_CODE_H_
#define IV_AERO_CODE_H_
#include <memory>
#include <string>
#include <iv/platform.h>
#include <iv/aero/jit_fwd.h>
#include <iv/aero/flags.h>
namespace iv {
namespace aero {

static const int kUndefined = -1;

class SimpleCase;

class Code {
 public:
  friend class VM;
  typedef std::vector<uint8_t> Data;
  Code(const std::vector<uint8_t>& vec,
       int flags,
       int captures, int counters, uint16_t filter, bool one_char,
       std::shared_ptr<SimpleCase> simple)
    : bytes_(vec),
      flags_(flags),
      captures_(captures),
      counters_(counters),
      filter_(filter),
      one_char_(one_char),
      simple_(simple)
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

  const SimpleCase* simple() const { return simple_.get(); }

  bool IsIgnoreCase() const { return flags_ & IGNORE_CASE; }

  bool IsMultiline() const { return flags_ & MULTILINE; }

 private:
  Data bytes_;
  int flags_;
  int captures_;
  int counters_;
  uint16_t filter_;
  bool one_char_;
  std::shared_ptr<SimpleCase> simple_;
#if defined(IV_ENABLE_JIT)
  std::unique_ptr<JITCode> jit_;
 public:
  JITCode* jit() const { return jit_.get(); }
#endif
};

} }  // namespace iv::aero
#endif  // IV_AERO_CODE_H_
