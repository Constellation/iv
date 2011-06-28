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
  friend class Compiler;
  typedef GCVector<Symbol>::type Names;
  typedef GCVector<uint8_t>::type Data;
  typedef GCVector<Code*>::type Codes;
  typedef std::tuple<uint8_t, uint16_t, uint16_t, uint16_t, uint16_t>
          ExceptionHandler;
  typedef GCVector<ExceptionHandler>::type ExceptionTable;

  Code(Context* ctx, JSScript* script, const FunctionLiteral& func)
    : strict_(func.strict()),
      has_eval_(false),
      has_arguments_(false),
      has_name_(func.name()),
      arguments_hiding_(false),
      decl_type_(func.type()),
      name_(),
      script_(script),
      start_position_(func.start_position()),
      end_position_(func.end_position()),
      data_(),
      codes_(),
      names_(),
      varnames_(),
      params_(func.params().size()),
      constants_() {
    if (has_name_) {
      name_ = func.name().Address()->symbol();
    }
    const Symbol arguments_symbol = context::arguments_symbol(ctx);
    Names::iterator target = params_.begin();
    for (Identifiers::const_iterator it = func.params().begin(),
         last = func.params().end(); it != last; ++it, ++target) {
      if ((*target = (*it)->symbol()) == arguments_symbol) {
        set_code_hiding_arguments();
      }
    }
  }

  const uint8_t* data() const {
    return data_.data();
  }

  const Codes& codes() const {
    return codes_;
  }

  const Data& Main() const {
    return data_;
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

 private:
  bool strict_;
  bool has_eval_;
  bool has_arguments_;
  bool has_name_;
  bool arguments_hiding_;
  FunctionLiteral::DeclType decl_type_;
  Symbol name_;
  JSScript* script_;
  std::size_t start_position_;
  std::size_t end_position_;
  std::size_t stack_depth_;
  Data data_;
  Codes codes_;
  Names names_;
  Names varnames_;
  Names params_;
  JSVals constants_;
  ExceptionTable exception_table_;
};

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_CODE_H_
