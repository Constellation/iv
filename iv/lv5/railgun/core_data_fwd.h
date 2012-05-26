#ifndef IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#define IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#include <iv/detail/unique_ptr.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/gc_kind.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/instruction_fwd.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class CoreData : public GCKind<CoreData> {
 public:
  friend class breaker::Compiler;

  typedef std::vector<Instruction> Data;

  // Vector of pairs of bytecode offset and line number.
  // This offset is counted from Total Bytecode (that is, CoreData unit),
  // not Code unit.
  typedef std::pair<std::size_t, std::size_t> BytecodeOffsetAndLine;
  typedef core::SortedVector<BytecodeOffsetAndLine> Lines;

  static CoreData* New() {
    return new CoreData();
  }

  GC_ms_entry* MarkChildren(GC_word* top,
                            GC_ms_entry* entry,
                            GC_ms_entry* mark_sp_limit,
                            GC_word env);

  void MarkChildren(radio::Core* core);

  Data* data() {
    return &data_;
  }

  const Data* data() const {
    return &data_;
  }

  void SetCompiled() {
    compiled_ = true;
  }

  void AttachLine(std::size_t bytecode_offset, std::size_t line_number) {
  }

 private:
  explicit CoreData()
    : data_(),
      lines_(),
      compiled_(false),
      asm_(NULL) {
  }

  void set_asm(breaker::Assembler* assembler) {
    asm_.reset(assembler);
  }

  Data data_;
  Lines lines_;
  bool compiled_;
  core::unique_ptr<breaker::Assembler> asm_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CORE_DATA_FWD_H_
