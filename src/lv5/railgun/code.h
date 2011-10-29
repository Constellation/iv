#ifndef IV_LV5_RAILGUN_CODE_H_
#define IV_LV5_RAILGUN_CODE_H_
#include <algorithm>
#include "detail/tuple.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/gc_template.h"
#include "lv5/specialized_ast.h"
#include "lv5/map.h"
#include "lv5/radio/cell.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/core_data_fwd.h"
#include "lv5/railgun/jsscript.h"
#include "lv5/railgun/direct_threading.h"
namespace iv {
namespace lv5 {
namespace railgun {

struct Handler {
  enum Type {
    CATCH,
    FINALLY,
    ITER
  };
};

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
  friend class FunctionScope;
  typedef GCVector<Symbol>::type Names;
  typedef GCVector<Instruction>::type Data;
  typedef GCVector<Code*>::type Codes;
  typedef std::tuple<uint8_t, uint16_t, uint16_t, uint16_t, uint16_t>
          ExceptionHandler;
  typedef GCVector<ExceptionHandler>::type ExceptionTable;
  // symbol, decl type, configurable, param point
  typedef std::tuple<Symbol, DeclType, std::size_t, uint32_t> Decl;
  typedef GCVector<Decl>::type Decls;

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
      has_declarative_env_(true),
      arguments_hiding_(false),
      scope_nest_count_(0),
      reserved_record_size_(0),
      decl_type_(func.type()),
      name_(),
      script_(script),
      start_position_(func.start_position()),
      end_position_(func.end_position()),
      stack_depth_(0),
      core_(core),
      start_(),
      end_(),
      codes_(),
      names_(),
      varnames_(),
      params_(func.params().size()),
      locals_(),
      decls_(),
      constants_(),
      exception_table_(),
      construct_map_(NULL) {
    if (has_name_) {
      name_ = func.name().Address()->symbol();
    }
    Names::iterator target = params_.begin();
    for (Identifiers::const_iterator it = func.params().begin(),
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

  const Names& varnames() const {
    return varnames_;
  }

  Names& varnames() {
    return varnames_;
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

  bool strict() const {
    return strict_;
  }

  bool HasEval() const {
    return has_eval_;
  }

  JSScript* script() const {
    return script_;
  }

  bool HasArguments() const {
    return has_arguments_;
  }

  bool HasArgumentsAssign() const {
    return has_arguments_assign_;
  }

  bool HasDeclEnv() const {
    return has_declarative_env_;
  }

  bool IsShouldCreateArguments() const {
    return !arguments_hiding_ && (has_arguments_ || has_eval_);
  }

  bool IsShouldCreateHeapArguments() const {
     return IsShouldCreateArguments() && !strict();
  }

  bool IsFunctionDeclaration() const {
    return decl_type_ == FunctionLiteral::DECLARATION;
  }

  FunctionLiteral::DeclType decl_type() const {
    return decl_type_;
  }

  void set_code_has_eval() {
    has_eval_ = true;
  }

  void set_code_has_arguments() {
    has_arguments_ = true;
  }

  void set_code_has_arguments_assign() {
    has_arguments_assign_ = true;
  }

  void set_code_hiding_arguments() {
    arguments_hiding_ = true;
  }

  bool HasName() const {
    return has_name_;
  }

  const Symbol& name() const {
    return name_;
  }

  std::size_t start_position() const {
    return start_position_;
  }

  std::size_t end_position() const {
    return end_position_;
  }

  std::size_t stack_depth() const {
    return stack_depth_;
  }

  void set_stack_depth(std::size_t depth) {
    stack_depth_ = depth;
  }

  std::size_t start() const {
    return start_;
  }

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

  const Names& locals() const {
    return locals_;
  }

  CodeType code_type() const {
    return code_type_;
  }

  const Decls& decls() const {
    return decls_;
  }

  void set_has_declarative_env(bool val) {
    has_declarative_env_ = val;
  }

  void set_reserved_record_size(uint32_t size) {
    reserved_record_size_ = size;
  }

  uint32_t reserved_record_size() const {
    return reserved_record_size_;
  }

  void set_scope_nest_count(uint32_t count) {
    scope_nest_count_ = count;
  }

  uint32_t scope_nest_count() const {
    return scope_nest_count_;
  }

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
    // core->MarkCell(core_);
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
  bool has_eval_;
  bool has_arguments_;
  bool has_arguments_assign_;
  bool has_name_;
  bool has_declarative_env_;
  bool arguments_hiding_;
  uint32_t scope_nest_count_;
  uint32_t reserved_record_size_;
  FunctionLiteral::DeclType decl_type_;
  Symbol name_;
  JSScript* script_;
  std::size_t start_position_;
  std::size_t end_position_;
  std::size_t stack_depth_;
  CoreData* core_;
  std::size_t start_;
  std::size_t end_;
  Codes codes_;
  Names names_;
  Names varnames_;
  Names params_;
  Names locals_;
  Decls decls_;
  JSVals constants_;
  ExceptionTable exception_table_;
  Map* construct_map_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_CODE_H_
