#ifndef IV_BREAKER_IC_H_
#define IV_BREAKER_IC_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

class IC {
 public:
  static const std::size_t k64MovImmOffset = 2;

  enum Type {
    MONO,
    POLY
  };

  explicit IC(Type type)
    : type_(type) {
  }

  virtual ~IC() { }

  virtual void MarkChildren(radio::Core* core) { }
  virtual GC_ms_entry* MarkChildren(GC_word* top,
                                    GC_ms_entry* entry,
                                    GC_ms_entry* mark_sp_limit,
                                    GC_word env) {
    return entry;
  }

  static std::size_t Generate64Mov(Xbyak::CodeGenerator* as, const Xbyak::Reg64& reg) {
    const uint64_t dummy64 = UINT64_C(0x0FFF000000000000);
    const std::size_t result = as->getSize() + k64MovImmOffset;
    as->mov(reg, dummy64);
    return result;
  }

  static std::size_t Generate64Mov(Xbyak::CodeGenerator* as) {
    return Generate64Mov(as, rax);
  }

  // Generate Tail position
  static std::size_t GenerateTail(Xbyak::CodeGenerator* as) {
    const std::size_t result = Generate64Mov(as);
    as->jmp(rax);
    return result;
  }

  static void Rewrite(uint8_t* data, uint64_t disp, std::size_t size) {
    for (size_t i = 0; i < size; i++) {
      data[i] = static_cast<uint8_t>(disp >> (i * 8));
    }
  }

  static std::size_t TestMap(
      Xbyak::CodeGenerator* as, Map* map,
      const Xbyak::Reg64& obj,
      const Xbyak::Reg64& tmp,
      const char* fail,
      Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    const std::size_t offset = Generate64Mov(as, tmp);
    as->rewrite(offset, core::BitCast<uintptr_t>(map), k64Size);
    as->cmp(tmp, qword[obj + JSObject::MapOffset()]);
    as->jne(fail, type);
    return offset;
  }

  static void TestMapConstant(
      Xbyak::CodeGenerator* as, Map* map,
      const Xbyak::Reg64& obj,
      const Xbyak::Reg64& tmp,
      const char* fail,
      Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    // in uint32_t
    const uintptr_t ptr = core::BitCast<uintptr_t>(map);
    if (!Xbyak::inner::IsInInt32(ptr)) {
      TestMap(as, map, obj, tmp, fail, type);
      return;
    }
    as->cmp(qword[obj + JSObject::MapOffset()], static_cast<uint32_t>(ptr));
    as->jne(fail, type);
  }

 private:
  Type type_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_MONO_IC_H_
