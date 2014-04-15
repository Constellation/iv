#ifndef IV_AERO_JIT_CODE_H_
#define IV_AERO_JIT_CODE_H_
#include <iv/platform.h>
#include <iv/aero/jit_fwd.h>
#include <iv/aero/jit.h>
#if defined(IV_ENABLE_JIT)
namespace iv {
namespace aero {

inline JITExecutable<char>::Executable JITCode::Compile8(const Code* code) {
  if (jit8_) {
    return jit8_->Get();
  }
  jit8_.reset(new JIT<char>(*code));
  jit8_->Compile();
  return jit8_->Get();
}

inline JITExecutable<char16_t>::Executable JITCode::Compile16(const Code* code) {
  if (jit16_) {
    return jit16_->Get();
  }
  jit16_.reset(new JIT<char16_t>(*code));
  jit16_->Compile();
  return jit16_->Get();
}

inline int JITCode::Execute(VM* vm, Code* code, const core::string_view& subject,
                            int* captures, std::size_t current_position) {
  JITExecutable<char>::Executable exec = Compile8(code);
  return exec(vm, subject.data(), subject.size(), captures, current_position);
}

inline int JITCode::Execute(VM* vm, Code* code, const core::u16string_view& subject,
                            int* captures, std::size_t current_position) {
  JITExecutable<char16_t>::Executable exec = Compile16(code);
  return exec(vm, subject.data(), subject.size(), captures, current_position);
}

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_CODE_H_
