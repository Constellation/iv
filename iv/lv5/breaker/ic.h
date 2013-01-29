#ifndef IV_BREAKER_IC_H_
#define IV_BREAKER_IC_H_
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/helper.h>
#include <iv/lv5/radio/core_fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

class IC {
 public:

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

  // Generate Tail position
  static std::size_t GenerateTail(Xbyak::CodeGenerator* as) {
    const std::size_t result = helper::Generate64Mov(as);
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
    const std::size_t offset = helper::Generate64Mov(as, tmp);
    const std::size_t map_offset =
        IV_CAST_OFFSET(radio::Cell*, JSCell*) + JSCell::MapOffset();
    as->rewrite(offset, core::BitCast<uintptr_t>(map), k64Size);
    as->cmp(tmp, qword[obj + map_offset]);
    as->jne(fail, type);
    return offset;
  }

  static bool TestMapConstant(
      Xbyak::CodeGenerator* as, Map* map,
      const Xbyak::Reg64& obj,
      const Xbyak::Reg64& tmp,
      const char* fail,
      Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    // in uint32_t
    const uintptr_t ptr = core::BitCast<uintptr_t>(map);
    const std::size_t map_offset =
        IV_CAST_OFFSET(radio::Cell*, JSCell*) + JSCell::MapOffset();
    const bool packed = helper::CmpConstant(as, qword[obj + map_offset], ptr, tmp);
    as->jne(fail, type);
    return packed;
  }

  Type type() const { return type_; }

 private:
  Type type_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_MONO_IC_H_
