#ifndef IV_AERO_JIT_FWD_H_
#define IV_AERO_JIT_FWD_H_
#include <iv/platform.h>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/xbyak/xbyak.h>
namespace iv {
namespace aero {

class VM;

template<typename CharT>
struct JITExecutable {
  typedef int(*Executable)(VM* vm, const CharT* subject, uint32_t size, int* captures, uint32_t cp);  // NOLINT
};

class Code;

class JITCode {
 public:
  JITCode()
    : jit8_(NULL),
      jit16_(NULL),
      jit8_match_only_(NULL),
      jit16_match_only_(NULL),
      exec8_(NULL),
      exec16_(NULL),
      exec8_match_only_(NULL),
      exec16_match_only_(NULL) {
  }

  ~JITCode() {
    delete jit8_;
    delete jit16_;
  }

  JITExecutable<char>::Executable Compile8(const Code* code);
  JITExecutable<uint16_t>::Executable Compile16(const Code* code);
  JITExecutable<char>::Executable Compile8MatchOnly(const Code* code);
  JITExecutable<uint16_t>::Executable Compile16MatchOnly(const Code* code);

  int Execute(VM* vm, Code* code, const core::StringPiece& subject,
              int* captures, std::size_t current_position);
  int Execute(VM* vm, Code* code, const core::UStringPiece& subject,
              int* captures, std::size_t current_position);
  int ExecuteMatchOnly(VM* vm, Code* code, const core::StringPiece& subject,
                       int* captures, std::size_t current_position);
  int ExecuteMatchOnly(VM* vm, Code* code, const core::UStringPiece& subject,
                       int* captures, std::size_t current_position);

 private:
  Xbyak::CodeGenerator* jit8_;
  Xbyak::CodeGenerator* jit16_;
  Xbyak::CodeGenerator* jit8_match_only_;
  Xbyak::CodeGenerator* jit16_match_only_;
  JITExecutable<char>::Executable exec8_;
  JITExecutable<uint16_t>::Executable exec16_;
  JITExecutable<char>::Executable exec8_match_only_;
  JITExecutable<uint16_t>::Executable exec16_match_only_;
};

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_CODE_H_
