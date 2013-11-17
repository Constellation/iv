// MonoIC compiler
#ifndef IV_BREAKER_MONO_IC_H_
#define IV_BREAKER_MONO_IC_H_
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/ic.h>
namespace iv {
namespace lv5 {
namespace breaker {

class MonoIC : public IC {
 public:
  MonoIC() : IC(IC::MONO) { }
};

class GlobalIC : public MonoIC {
 public:
  static const int kMovAddressImmOffset = 3;
  static const std::size_t kMaxOffset = 0x7FFFFFFF;

  GlobalIC(bool strict)
    : MonoIC(),
      map_position_(),
      offset_position_(),
      map_(nullptr),
      strict_(strict) {
  }

  void CompileLoad(Assembler* as, JSObject* global, railgun::Code* code, Symbol name) {
    const Assembler::LocalLabelScope scope(as);

    as->mov(rax, core::BitCast<uint64_t>(global));
    map_position_ = IC::TestMap(as, nullptr, rax, rdx, ".SLOW");

    const uint32_t dummy32 = 0x7FFF0000;
    const std::ptrdiff_t data_offset =
        JSObject::SlotsOffset() +
        JSObject::Slots::DataOffset();
    as->mov(rax, qword[rax + data_offset]);
    offset_position_ = as->size() + kMovAddressImmOffset;
    as->mov(rax, qword[rax + dummy32]);
    as->jmp(".EXIT");

    as->L(".SLOW");
    as->mov(rdi, r14);
    as->mov(rsi, core::BitCast<uint64_t>(name));
    as->mov(rdx, core::BitCast<uint64_t>(this));
    as->mov(rcx, core::BitCast<uint64_t>(as));
    as->Call(&stub::LOAD_GLOBAL);

    as->L(".EXIT");
  }

  // Now we assume that target source value is stored in rax
  void CompileStore(Assembler* as, JSObject* global, railgun::Code* code, Symbol name) {
    const Assembler::LocalLabelScope scope(as);

    as->mov(rcx, core::BitCast<uint64_t>(global));
    map_position_ = IC::TestMap(as, nullptr, rcx, rdx, ".SLOW");

    const uint32_t dummy32 = 0x7FFF0000;
    const std::ptrdiff_t data_offset =
        JSObject::SlotsOffset() +
        JSObject::Slots::DataOffset();
    as->mov(rcx, qword[rcx + data_offset]);
    offset_position_ = as->size() + kMovAddressImmOffset;
    as->mov(qword[rcx + dummy32], rax);
    as->jmp(".EXIT");

    as->L(".SLOW");
    as->mov(rdi, r14);
    as->mov(rsi, core::BitCast<uint64_t>(name));
    as->mov(rdx, core::BitCast<uint64_t>(this));
    as->mov(rcx, core::BitCast<uint64_t>(as));
    as->mov(r8, rax);
    as->Call(&stub::STORE_GLOBAL);

    as->L(".EXIT");
  }

  void Repatch(Assembler* as, Map* map, uint32_t offset) {
    if (offset <= GlobalIC::kMaxOffset) {
      as->rewrite(map_position(), core::BitCast<uint64_t>(map), k64Size);
      as->rewrite(offset_position(), offset, k32Size);
      map_ = map;
    }
  }

  std::size_t map_position() const { return map_position_; }
  std::size_t offset_position() const { return offset_position_; }
  bool strict() const { return strict_; }

  virtual void MarkChildren(radio::Core* core) {
    core->MarkCell(map_);
  }

  virtual GC_ms_entry* MarkChildren(GC_word* top,
                                    GC_ms_entry* entry,
                                    GC_ms_entry* mark_sp_limit,
                                    GC_word env) {
    return GC_MARK_AND_PUSH(
        map_, entry, mark_sp_limit, reinterpret_cast<void**>(this));
  }

 private:
  std::size_t map_position_;
  std::size_t offset_position_;
  Map* map_;
  bool strict_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_MONO_IC_H_
