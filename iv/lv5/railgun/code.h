#ifndef IV_LV5_RAILGUN_CODE_H_
#define IV_LV5_RAILGUN_CODE_H_
#include <algorithm>
#include <iv/detail/tuple.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
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
  enum DeclType {
    PARAM,
    PARAM_LOCAL,
    FDECL,
    ARGUMENTS,
    ARGUMENTS_LOCAL,
    VAR,
    FEXPR,
    FEXPR_LOCAL
  };
  friend class Compiler;
  typedef GCVector<Symbol>::type Names;
  typedef GCVector<Instruction>::type Data;
  typedef GCVector<Code*>::type Codes;
  // symbol, decl type, configurable, param point
  typedef std::tuple<Symbol, DeclType, std::size_t, uint32_t> Decl;

  Code(Context* ctx,
       JSScript* script,
       const FunctionLiteral& func,
       CoreData* core,
       CodeType code_type)
    : code_type_(code_type),
      strict_(func.strict()),
      has_eval_(false),
      has_arguments_(false),
      has_arguments_assign_(false),
      has_name_(func.name()),
      scope_nest_count_(0),
      heap_size_(0),
      name_(),
      script_(script),
      block_begin_position_(func.block_begin_position()),
      block_end_position_(func.block_end_position()),
      stack_depth_(0),
      core_(core),
      start_(),
      end_(),
      codes_(),
      names_(),
      params_(func.params().size()),
      constants_(),
      exception_table_(),
      construct_map_(NULL) {
    if (has_name_) {
      name_ = func.name().Address()->symbol();
    }
    Names::iterator target = params_.begin();
    for (Assigneds::const_iterator it = func.params().begin(),
         last = func.params().end(); it != last; ++it, ++target) {
      if ((*target = (*it)->symbol()) == symbol::arguments()) {
        set_code_hiding_arguments();
      }
    }
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

  template<Handler::Type type>
  void RegisterHandler(uint16_t begin, uint16_t end,
                       uint16_t stack_base_level,
                       uint16_t dynamic_env_level) {
    exception_table_.push_back(
        std::make_tuple(type, begin, end, stack_base_level, dynamic_env_level));
  }

  bool strict() const { return strict_; }

  JSScript* script() const { return script_; }

  bool HasName() const { return has_name_; }

  const Symbol& name() const { return name_; }

  std::size_t block_begin_position() const { return block_begin_position_; }

  std::size_t block_end_position() const { return block_end_position_; }

  std::size_t stack_depth() const { return stack_depth_; }

  void set_stack_depth(std::size_t depth) {
    stack_depth_ = depth + stack_size_;
  }

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

  void set_heap_size(uint32_t size) { heap_size_ = size; }

  uint32_t heap_size() const { return heap_size_; }

  void set_stack_size(uint32_t size) { stack_size_ = size; }

  uint32_t stack_size() const { return stack_size_; }

  void set_scope_nest_count(uint32_t count) { scope_nest_count_ = count; }

  uint32_t scope_nest_count() const { return scope_nest_count_; }

  Data* GetData() {
    return core_->data();
  }

  Map* ConstructMap(Context* ctx) {
    if (!construct_map_) {
      construct_map_ = Map::New(ctx);
    }
    return construct_map_;
  }

  void MarkChildren(radio::Core* core) {
    core->MarkCell(script_);
    core_->MarkChildren(core);
    std::for_each(codes_.begin(), codes_.end(), radio::Core::Marker(core));
    std::for_each(constants_.begin(),
                  constants_.end(), radio::Core::Marker(core));
    core->MarkCell(construct_map_);
  }

 private:

  void set_start(std::size_t start) {
    start_ = start;
  }

  void set_end(std::size_t end) {
    end_ = end;
  }

  CodeType code_type_;
  bool strict_;
  bool has_name_;
  uint32_t scope_nest_count_;
  uint32_t heap_size_;
  uint32_t stack_size_;
  Symbol name_;
  JSScript* script_;
  std::size_t block_begin_position_;
  std::size_t block_end_position_;
  std::size_t stack_depth_;
  CoreData* core_;
  std::size_t start_;
  std::size_t end_;
  Codes codes_;
  Names names_;
  Names params_;
  JSVals constants_;
  ExceptionTable exception_table_;
  Map* construct_map_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CODE_H_
