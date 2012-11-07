#ifndef IV_LV5_BREAKER_NATIVE_CODE_H_
#define IV_LV5_BREAKER_NATIVE_CODE_H_
#include <iv/functor.h>
#include <iv/lv5/breaker/assembler.h>

namespace iv {
namespace lv5 {
namespace breaker {

class NativeCode {
 public:
  typedef std::pair<std::size_t, std::size_t> PCOffsetAndBytecodeOffset;
  typedef core::SortedVector<PCOffsetAndBytecodeOffset> BytecodeOffsets;
  typedef std::vector<IC*> ICVector;
  typedef ExecutablePages<> Pages;

  NativeCode(Assembler* as)
    : bytecode_offsets_(),
      ics_(),
      pages_(),
      asm_() {
    bytecode_offsets_.reserve(1024);
    asm_.reset(as);
  }

  ~NativeCode() {
    std::for_each(ics_.begin(), ics_.end(), core::Deleter<IC>());
  }

  std::size_t PCToBytecodeOffset(void* pc) const {
    const BytecodeOffsets::const_iterator it =
        std::upper_bound(
            bytecode_offsets_.begin(),
            bytecode_offsets_.end(),
            core::BitCast<uint64_t>(pc) - core::BitCast<uint64_t>(asm_->getCode()),
            Comparator());
    if (it != bytecode_offsets_.begin()) {
      return (it - 1)->second;
    }
    return 0;
  }

  struct Comparator {
    bool operator()(const PCOffsetAndBytecodeOffset& offset,
                    std::size_t pc_offset) const {
      return offset.first < pc_offset;
    }

    bool operator()(std::size_t pc_offset,
                    const PCOffsetAndBytecodeOffset& offset) const {
      return pc_offset < offset.first;
    }
  };

  void AttachBytecodeOffset(std::size_t pc_offset,
                            std::size_t bytecode_offset) {
    if (bytecode_offsets_.empty()) {
      bytecode_offsets_.push_back(std::make_pair(pc_offset, bytecode_offset));
      return;
    }

    PCOffsetAndBytecodeOffset& offset = bytecode_offsets_.back();
    if (offset.first == pc_offset) {
      offset.second = bytecode_offset;
      return;
    }

    if (offset.second != bytecode_offset) {
      bytecode_offsets_.push_back(std::make_pair(pc_offset, bytecode_offset));
    }
  }

  void BindIC(IC* ic) {
    ics_.push_back(ic);
  }

  GC_ms_entry* MarkChildren(GC_word* top,
                            GC_ms_entry* entry,
                            GC_ms_entry* mark_sp_limit,
                            GC_word env) {
    std::size_t index = 0;
    for (ICVector::const_iterator it = ics_.begin(),
         last = ics_.end(); it != last; ++it, ++index) {
      IC* ic = *it;
      entry = ic->MarkChildren(top, entry, mark_sp_limit, env);
    }
    return entry;
  }

  void MarkChildren(radio::Core* core) {
    for (ICVector::const_iterator it = ics_.begin(),
         last = ics_.end(); it != last; ++it) {
      (*it)->MarkChildren(core);
    }
  }

  Pages* pages() { return &pages_; }
  const Pages* pages() const { return &pages_; }
  Assembler* assembler() const { return asm_.get(); }

 private:
  BytecodeOffsets bytecode_offsets_;
  ICVector ics_;
  Pages pages_;
  core::unique_ptr<Assembler> asm_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_NATIVE_CODE_H_
