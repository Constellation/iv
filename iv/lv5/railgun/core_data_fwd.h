#ifndef IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#define IV_LV5_RAILGUN_CORE_DATA_FWD_H_
#include <algorithm>
#include <iv/detail/unique_ptr.h>
#include <iv/sorted_vector.h>
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
  typedef std::vector<BytecodeOffsetAndLine> Lines;

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

  struct Comparator {
    bool operator()(const BytecodeOffsetAndLine& line,
                    std::size_t bytecode_offset) const {
      return line.first < bytecode_offset;
    }

    bool operator()(std::size_t bytecode_offset,
                    const BytecodeOffsetAndLine& line) const {
      return bytecode_offset < line.first;
    }
  };

  void AttachLine(std::size_t bytecode_offset, std::size_t line_number) {
    if (lines_.empty()) {
      lines_.push_back(std::make_pair(bytecode_offset, line_number));
      return;
    }

    BytecodeOffsetAndLine& line = lines_.back();
    if (line.first == bytecode_offset) {
      line.second = line_number;
      return;
    }

    if (line.second != line_number) {
      lines_.push_back(std::make_pair(bytecode_offset, line_number));
    }
  }

  std::size_t LookupLineNumber(std::size_t bytecode_offset) const {
    const Lines::const_iterator it =
        std::upper_bound(lines_.begin(),
                         lines_.end(),
                         bytecode_offset, Comparator());
    if (it != lines_.begin()) {
      return (it - 1)->second;
    }
    return 1;
  }

  const breaker::Assembler* assembler() const { return asm_.get(); }

 private:
  explicit CoreData()
    : data_(),
      lines_(),
      compiled_(false),
      asm_() {
    lines_.reserve(1024);
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
