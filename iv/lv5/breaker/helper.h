#ifndef IV_BREAKER_HELPER_H_
#define IV_BREAKER_HELPER_H_
#include <iv/detail/cstdint.h>
#include <iv/detail/cinttypes.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {
namespace helper {

// helper functions for code generation
inline std::size_t Generate64Mov(Xbyak::CodeGenerator* as,
                                 const Xbyak::Reg64& reg = rax) {
  const uint64_t dummy64 = UINT64_C(0x0FFF000000000000);
  const std::size_t k64MovImmOffset = 2;
  const std::size_t result = as->getSize() + k64MovImmOffset;
  as->mov(reg, dummy64);
  return result;
}

inline bool MovConstant(
    Xbyak::CodeGenerator* as,
    const Xbyak::Operand& dst, uintptr_t value, const Xbyak::Reg64& tmp) {
  // in uint32_t
  if (!Xbyak::inner::IsInInt32(value)) {
    as->mov(tmp, value);
    as->mov(dst, tmp);
    return false;
  }
  as->mov(dst, value);
  return true;
}

inline bool CmpConstant(
    Xbyak::CodeGenerator* as,
    const Xbyak::Operand& dst, uintptr_t value, const Xbyak::Reg64& tmp) {
  // in uint32_t
  if (!Xbyak::inner::IsInInt32(value)) {
    as->mov(tmp, value);
    as->cmp(dst, tmp);
    return false;
  }
  as->cmp(dst, value);
  return true;
}

inline bool TestConstant(
    Xbyak::CodeGenerator* as,
    const Xbyak::Operand& dst, uintptr_t value, const Xbyak::Reg64& tmp) {
  // in uint32_t
  if (!Xbyak::inner::IsInInt32(value)) {
    as->mov(tmp, value);
    as->test(dst, tmp);
    return false;
  }
  as->test(dst, value);
  return true;
}

} } } }  // namespace iv::lv5::breaker::helper
#endif  // IV_BREAKER_HELPER_H_
