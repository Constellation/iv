#ifndef IV_LV5_RAILGUN_CODE_H_
#define IV_LV5_RAILGUN_CODE_H_
#include <algorithm>
#include <iv/detail/tuple.h>
#include <iv/ustring.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/map.h>
#include <iv/lv5/radio/cell.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/core_data_fwd.h>
#include <iv/lv5/railgun/jsscript.h>
#include <iv/lv5/railgun/exception.h>
#include <iv/lv5/railgun/direct_threading.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Code : public radio::HeapObject<radio::POINTER> {
 public:
  enum CodeType {
    FUNCTION,
    GLOBAL,
    EVAL
  };
  friend class Compiler;
  friend class ConstantPool;
  friend class breaker::Compiler;
  typedef GCVector<Symbol>::type Names;
  typedef CoreData::Data Data;
  typedef GCVector<Code*>::type Codes;
  typedef GCVector<Map*>::type Maps;

  Code(Context* ctx,
       JSScript* script,
       const FunctionLiteral& func,
       CoreData* core,
       CodeType code_type)
    : code_type_(code_type),
      strict_(func.strict()),
      empty_(false),
      has_name_(func.name()),
      needs_declarative_environment_(false),
      this_materialized_(func.scope().direct_call_to_eval()),
      heap_size_(0),
      stack_size_(0),
      temporary_registers_(0),
      hot_code_counter_(0),
      name_(),
      script_(script),
      block_begin_position_(func.block_begin_position()),
      block_end_position_(func.block_end_position()),
      core_(core),
      start_(),
      end_(),
      codes_(),
      names_(),
      params_(func.params().size()),
      constants_(),
      maps_(),
      exception_table_(),
      construct_map_(nullptr),
      executable_(nullptr) {
    if (has_name_) {
      name_ = func.name().Address()->symbol();
    }
    Names::iterator target = params_.begin();
    for (Assigneds::const_iterator it = func.params().begin(),
         last = func.params().end(); it != last; ++it, ++target) {
      *target = (*it)->symbol();
    }
  }

  std::u16string GenerateErrorLine(const Instruction* instr) const {
    const std::size_t line_number =
        core_->LookupLineNumber(instr - core_->data()->data());
    std::u16string result;
    if (has_name_) {
      result.append(symbol::GetSymbolString(name()));
    } else {
      static const char* anonymous = "(anonymous)";
      static const char* eval = "(eval)";
      static const char* global = "(global)";
      if (code_type() == EVAL) {
        result.append(eval, eval + std::strlen(eval));
      } else if (code_type() == GLOBAL) {
        result.append(global, global + std::strlen(global));
      } else {
        result.append(anonymous, anonymous + std::strlen(anonymous));
      }
    }
    result.push_back('@');
    result.append(script_->filename());
    result.push_back(':');
    core::UInt64ToString(line_number, std::back_inserter(result));
    return result;
  }

  const Instruction* data() const {
    return core_->data()->data() + start_;
  }

  Instruction* data() {
    return core_->data()->data() + start_;
  }

  const Codes& codes() const {
    return codes_;
  }

  const JSVals& constants() const {
    return constants_;
  }

  const Names& names() const {
    return names_;
  }

  const Names& params() const {
    return params_;
  }

  const ExceptionTable& exception_table() const {
    return exception_table_;
  }

  ExceptionTable& exception_table() {
    return exception_table_;
  }

  void RegisterHandler(const Handler& handler) {
    exception_table_.push_back(handler);
  }

  bool strict() const { return strict_; }

  JSScript* script() const { return script_; }

  bool HasName() const { return has_name_; }

  const Symbol& name() const { return name_; }

  std::size_t block_begin_position() const { return block_begin_position_; }

  std::size_t block_end_position() const { return block_end_position_; }

  std::size_t start() const { return start_; }

  const Instruction* begin() const {
    return core_->data()->data() + start_;
  }

  Instruction* begin() {
    return core_->data()->data() + start_;
  }

  const Instruction* end() const {
    return core_->data()->data() + end_;
  }

  Instruction* end() {
    return core_->data()->data() + end_;
  }

  CodeType code_type() const { return code_type_; }

  uint32_t heap_size() const { return heap_size_; }

  uint32_t stack_size() const { return stack_size_; }

  uint32_t temporary_registers() const { return temporary_registers_; }

  bool empty() const { return empty_; }

  const Data* GetData() const { return core_->data(); }

  Data* GetData() { return core_->data(); }

  uint32_t registers() const {
    return stack_size() + temporary_registers();
    // return stack_size() + heap_size() + temporary_registers();
  }

  uint32_t FrameSize() const { return frame_size_; }

  bool needs_declarative_environment() const {
    return needs_declarative_environment_;
  }

  bool IsThisMaterialized() const { return this_materialized_; }

  void set_executable(void* executable) {
    executable_ = executable;
  }

  CoreData* core_data() { return core_; }

  void* executable() const { return executable_; }

  void RegisterMap(Map* map) { maps_.push_back(map); }

  void MarkChildren(radio::Core* core) {
    core->MarkCell(script_);
    core_->MarkChildren(core);
    std::for_each(codes_.begin(), codes_.end(), radio::Core::Marker(core));
    std::for_each(constants_.begin(),
                  constants_.end(), radio::Core::Marker(core));
    std::for_each(maps_.begin(), maps_.end(), radio::Core::Marker(core));
    core->MarkCell(construct_map_);
  }

  inline uint32_t IncrementHotCodeCounter() {
    return ++hot_code_counter_;
  }

 private:
  void set_start(std::size_t start) { start_ = start; }

  void set_end(std::size_t end) { end_ = end; }

  void set_heap_size(uint32_t size) { heap_size_ = size; }

  void set_stack_size(uint32_t size) { stack_size_ = size; }

  void set_temporary_registers(uint32_t size) { temporary_registers_ = size; }

  void set_empty(bool val) { empty_ = val; }

  void set_this_materialized(bool val) { this_materialized_ = val; }

  void set_needs_declarative_environment(bool val) {
    needs_declarative_environment_ = val;
  }

  void CalculateFrameSize(uint32_t frame_size) {
    frame_size_ = std::max<uint32_t>(frame_size, registers());
  }

  CodeType code_type_;
  bool strict_;
  bool empty_;
  bool has_name_;
  bool needs_declarative_environment_;
  bool this_materialized_;
  uint32_t heap_size_;   // heaps
  uint32_t stack_size_;  // locals
  uint32_t temporary_registers_;  // number of temporary registers
  uint32_t frame_size_;  // frame total size includes next frame header
  uint32_t hot_code_counter_;  // counter for invocation and backward jumps
  Symbol name_;
  JSScript* script_;
  std::size_t block_begin_position_;
  std::size_t block_end_position_;
  CoreData* core_;
  std::size_t start_;
  std::size_t end_;
  Codes codes_;
  Names names_;
  Names params_;
  JSVals constants_;
  Maps maps_;
  ExceptionTable exception_table_;
  Map* construct_map_;
  void* executable_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CODE_H_
