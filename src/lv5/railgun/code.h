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

  explicit Code(JSScript* script, const FunctionLiteral& func)
    : strict_(func.strict()),
      has_eval_(false),
      has_arguments_(false),
      script_(script),
      start_position_(func.start_position()),
      end_position_(func.end_position()),
      data_(),
      codes_(),
      names_(),
      varnames_(),
      params_(func.params().size()),
      constants_() {
    std::transform(func.params().begin(),
                   func.params().end(),
                   params_.begin(),
                   std::mem_fun(&Identifier::symbol));
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

  void set_code_has_eval() {
    has_eval_ = true;
  }

  void set_code_has_arguments() {
    has_arguments_ = true;
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
