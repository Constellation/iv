#ifndef IV_AERO_JIT_CODE_H_
#define IV_AERO_JIT_CODE_H_
#include <iv/no_operator_names_guard.h>
#include <iv/aero/jit_fwd.h>
#include <iv/aero/jit.h>
#if defined(IV_ENABLE_JIT)
namespace iv {
namespace aero {

inline JITExecutable<char>::Executable JITCode::Compile8(const Code* code) {
  if (jit8_) {
    return exec8_;
  }
  JIT<char>* jit = new JIT<char>(*code);
  jit->Compile();
  jit8_ = jit;
  exec8_ = jit->Get();
  return exec8_;
}

inline JITExecutable<uint16_t>::Executable JITCode::Compile16(const Code* code) {
  if (jit16_) {
    return exec16_;
  }
  JIT<uint16_t>* jit = new JIT<uint16_t>(*code);
  jit->Compile();
  jit16_ = jit;
  exec16_ = jit->Get();
  return exec16_;
}

inline int JITCode::Execute(VM* vm, Code* code, const core::StringPiece& subject,
                            int* captures, std::size_t current_position) {
  JITExecutable<char>::Executable exec = Compile8(code);
  return exec(vm, subject.data(), subject.size(), captures, current_position);
}

inline int JITCode::Execute(VM* vm, Code* code, const core::UStringPiece& subject,
                            int* captures, std::size_t current_position) {
  JITExecutable<uint16_t>::Executable exec = Compile16(code);
  return exec(vm, subject.data(), subject.size(), captures, current_position);
}

} }  // namespace iv::aero
#endif
#endif  // IV_AERO_JIT_CODE_H_
