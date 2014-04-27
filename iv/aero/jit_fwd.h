#ifndef IV_AERO_JIT_FWD_H_
#define IV_AERO_JIT_FWD_H_
#include <memory>
#include <iv/platform.h>
#include <iv/string_view.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/xbyak/xbyak.h>
namespace iv {
namespace aero {

class VM;

template<typename CharT>
struct JITExecutable {
  typedef int(*Executable)(VM* vm, const CharT* subject, uint32_t size, int* captures, uint32_t cp);  // NOLINT
};

template<typename CharT>
class JIT;

class Code;

class JITCode {
 public:
  JITCode()
    : jit8_()
    , jit16_() {
  }

  JITExecutable<char>::Executable Compile8(const Code* code);

  JITExecutable<char16_t>::Executable Compile16(const Code* code);

  int Execute(VM* vm, Code* code, const core::string_view& subject,
              int* captures, std::size_t current_position);

  int Execute(VM* vm, Code* code, const core::u16string_view& subject,
              int* captures, std::size_t current_position);

 private:
  std::unique_ptr<JIT<char>> jit8_;
  std::unique_ptr<JIT<char16_t>> jit16_;
};

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_CODE_H_
