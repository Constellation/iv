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
  static const int k64MovImmOffset = 2;
  static const int k32MovImmOffset = 1;
  static const int kJSValSize = sizeof(JSVal);

  MonoIC()
    : IC(IC::MONO),
      map_position_(),
      offset_position_(),
      map_(NULL) {
  }

  void CompileLoad(Assembler* as, JSObject* global, railgun::Code* code, Symbol name) {
    const Assembler::LocalLabelScope scope(as);

    MapCompile(as, as->rdx);

    as->mov(as->rcx, core::BitCast<uint64_t>(global));
    as->mov(as->rax, as->rcx);

    // load map pointer
    as->mov(as->rcx, as->qword[as->rcx + IV_OFFSETOF(JSObject, map_)]);
    as->cmp(as->rdx, as->rcx);
    as->jne(".SLOW");

    OffsetCompile(as, as->edx);

    const std::ptrdiff_t data_offset =
        IV_OFFSETOF(JSObject, slots_) +
        IV_OFFSETOF(JSObject::Slots, data_);
    as->mov(as->rax, as->qword[as->rax + data_offset]);
    as->mov(as->rax, as->qword[as->rax + as->rdx * kJSValSize]);
    as->jmp(".EXIT");

    as->L(".SLOW");
    as->mov(as->rdi, as->r14);
    as->mov(as->rsi, core::BitCast<uint64_t>(name));
    as->mov(as->rdx, core::BitCast<uint64_t>(this));
    as->mov(as->rcx, core::BitCast<uint64_t>(as));
    if (code->strict()) {
      as->Call(&stub::LOAD_GLOBAL<true>);
    } else {
      as->Call(&stub::LOAD_GLOBAL<false>);
    }

    as->L(".EXIT");
  }

  // Now we assume that target source value is stored in rax
  void CompileStore(Assembler* as, JSObject* global, railgun::Code* code, Symbol name) {
    const Assembler::LocalLabelScope scope(as);

    MapCompile(as, as->rdx);

    as->mov(as->rcx, core::BitCast<uint64_t>(global));
    as->mov(as->rdi, as->rcx);

    // load map pointer
    as->mov(as->rcx, as->qword[as->rcx + IV_OFFSETOF(JSObject, map_)]);
    as->cmp(as->rdx, as->rcx);
    as->jne(".SLOW");

    OffsetCompile(as, as->edx);

    const std::ptrdiff_t data_offset =
        IV_OFFSETOF(JSObject, slots_) +
        IV_OFFSETOF(JSObject::Slots, data_);
    as->mov(as->rdi, as->qword[as->rdi + data_offset]);
    as->mov(as->qword[as->rdi + as->rdx * kJSValSize], as->rax);
    as->jmp(".EXIT");

    as->L(".SLOW");
    as->mov(as->rdi, as->r14);
    as->mov(as->rsi, core::BitCast<uint64_t>(name));
    as->mov(as->rdx, core::BitCast<uint64_t>(this));
    as->mov(as->rcx, core::BitCast<uint64_t>(as));
    as->mov(as->r8, as->rax);
    if (code->strict()) {
      as->Call(&stub::STORE_GLOBAL<true>);
    } else {
      as->Call(&stub::STORE_GLOBAL<false>);
    }

    as->L(".EXIT");
  }

  void Repatch(Assembler* as, Map* map, uint32_t offset) {
    // Repatch map pointer in machine code
    as->rewrite(map_position(), core::BitCast<uint64_t>(map), k64Size);
    as->rewrite(offset_position(), offset, k32Size);
    map_ = map;
  }

  std::size_t map_position() const { return map_position_; }

  std::size_t offset_position() const { return offset_position_; }

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
  void MapCompile(Assembler* as, const Xbyak::Reg64& map) {
    const uint64_t dummy64 = UINT64_C(0x0FFF000000000000);
    map_position_ = as->size() + k64MovImmOffset;
    as->mov(map, dummy64);
    as->rewrite(map_position(), 0, k64Size);
  }

  void OffsetCompile(Assembler* as, const Xbyak::Reg32& offset) {
    const uint32_t dummy32 = 0x0FFF00000;
    offset_position_ = as->size() + k32MovImmOffset;
    as->mov(offset, dummy32);
    as->rewrite(offset_position(), 0, k32Size);
  }

  std::size_t map_position_;
  std::size_t offset_position_;
  Map* map_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_MONO_IC_H_
