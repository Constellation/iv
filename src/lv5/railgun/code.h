#ifndef _IV_LV5_RAILGUN_CODE_H_
#define _IV_LV5_RAILGUN_CODE_H_
#include <algorithm>
#include "detail/tuple.h"
#include "lv5/jsval.h"
#include "lv5/jsobject.h"
#include "lv5/gc_template.h"
#include "lv5/specialized_ast.h"
#include "lv5/heap_object.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/op.h"
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

class Code : public HeapObject {
 public:
  enum CodeType {
    FUNCTION,
    GLOBAL,
    EVAL
  };
  enum DeclType {
    PARAM,
    FDECL,
    ARGUMENTS,
    VAR,
    FEXPR
  };
  friend class Compiler;
  friend class FunctionScope;
  typedef GCVector<Symbol>::type Names;
  typedef GCVector<uint8_t>::type Data;
  typedef GCVector<Code*>::type Codes;
  typedef std::tuple<uint8_t, uint16_t, uint16_t, uint16_t, uint16_t>
          ExceptionHandler;
  typedef GCVector<ExceptionHandler>::type ExceptionTable;
  // symbol, decl type, configurable, immutable
  typedef std::tuple<Symbol, DeclType, bool, std::size_t> Decl;
  typedef GCVector<Decl>::type Decls;

  Code(Context* ctx,
       JSScript* script,
       const FunctionLiteral& func,
       Data* data,
       CodeType code_type)
    : code_type_(code_type),
      strict_(func.strict()),
      has_eval_(false),
      has_arguments_(false),
      has_name_(func.name()),
      has_declarative_env_(true),
      arguments_hiding_(false),
      decl_type_(func.type()),
      name_(),
      script_(script),
      start_position_(func.start_position()),
      end_position_(func.end_position()),
      data_(data),
      start_(),
      codes_(),
      names_(),
      varnames_(),
      params_(func.params().size()),
      decls_(),
      constants_() {
    if (has_name_) {
      name_ = func.name().Address()->symbol();
    }
    Names::iterator target = params_.begin();
    for (Identifiers::const_iterator it = func.params().begin(),
         last = func.params().end(); it != last; ++it, ++target) {
      if ((*target = (*it)->symbol()) == symbol::arguments) {
        set_code_hiding_arguments();
      }
    }
  }

  const uint8_t* data() const {
    return data_->data() + start_;
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

  bool HasDeclEnv() const {
    return has_declarative_env_;
  }

  bool ShouldCreateArguments() const {
    return !arguments_hiding_ && (has_arguments_ || has_eval_);
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

  const uint8_t* begin() const {
    return data_->data() + start_;
  }

  const uint8_t* end() const {
    return data_->data() + end_;
  }

  std::size_t locals() const {
    return locals_;
  }

  void set_locals(std::size_t locals) {
    locals_ = locals;
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
  bool has_name_;
  bool has_declarative_env_;
  bool arguments_hiding_;
  FunctionLiteral::DeclType decl_type_;
  Symbol name_;
  JSScript* script_;
  std::size_t start_position_;
  std::size_t end_position_;
  std::size_t stack_depth_;
  std::size_t locals_;
  Data* data_;
  std::size_t start_;
  std::size_t end_;
  Codes codes_;
  Names names_;
  Names varnames_;
  Names params_;
  Decls decls_;
  JSVals constants_;
  ExceptionTable exception_table_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CODE_H_
