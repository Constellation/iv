#ifndef IV_LV5_TELEPORTER_CONTEXT_H_
#define IV_LV5_TELEPORTER_CONTEXT_H_
#include <iv/detail/memory.h>
#include <iv/lv5/context.h>
#include <iv/lv5/jserror.h>
#include <iv/lv5/jsfunction.h>
#include <iv/lv5/teleporter/fwd.h>
#include <iv/lv5/teleporter/interpreter_fwd.h>
#include <iv/lv5/teleporter/jsscript.h>
#include <iv/lv5/teleporter/stack_resource.h>
namespace iv {
namespace lv5 {
namespace teleporter {

class Context : public iv::lv5::Context {
 public:
  enum Mode {
    NORMAL,
    BREAK,
    CONTINUE,
    RETURN,
    THROW
  };

  Context()
    : iv::lv5::Context(),
      lexical_env_(global_env()),
      variable_env_(global_env()),
      interp_(new Interpreter(this)),
      binding_(global_data()->global_obj()),
      mode_(NORMAL),
      ret_(),
      target_(NULL),
      generate_script_counter_(0),
      current_script_(NULL),
      strict_(false),
      stack_resource_(global_data()->global_obj()) {
    RegisterStack(stack_resource_.stack());
    Initialize<&FunctionConstructor, &GlobalEval>();
  }

  class ScriptScope : private core::Noncopyable<> {
   public:
    ScriptScope(Context* ctx, JSScript* script)
      : ctx_(ctx),
        prev_(ctx->current_script()) {
      ctx_->set_current_script(script);
    }
    ~ScriptScope() {
      ctx_->set_current_script(prev_);
    }
   private:
    Context* ctx_;
    JSScript* prev_;
  };

  bool Run(teleporter::JSScript* script) {
    const ScriptScope scope(this, script);
    interp_->Run(script->function(),
                 script->type() == teleporter::JSScript::kEval);
    assert(!ret_.IsEmpty() || error_);
    return error_;
  }

  JSEnv* lexical_env() const {
    return lexical_env_;
  }

  void set_lexical_env(JSEnv* env) {
    lexical_env_ = env;
  }

  JSEnv* variable_env() const {
    return variable_env_;
  }

  void set_variable_env(JSEnv* env) {
    variable_env_ = env;
  }

  JSVal this_binding() const {
    return binding_;
  }

  void set_this_binding(const JSVal& binding) {
    binding_ = binding;
  }

  std::shared_ptr<Interpreter> interp() {
    return interp_;
  }

  Mode mode() const {
    return mode_;
  }

  template<Mode m>
  bool IsMode() const {
    return mode_ == m;
  }

  void set_mode(Mode mode) {
    mode_ = mode;
  }

  JSVal ErrorVal() {
    return JSError::Detail(this, &error_);
  }


  const Error& error() const {
    return error_;
  }

  Error* error() {
    return &error_;
  }

  void set_error(const Error& error) {
    error_ = error;
  }

  const JSVal& ret() const {
    return ret_;
  }

  JSVal& ret() {
    return ret_;
  }

  void set_ret(const JSVal& ret) {
    ret_ = ret;
  }

  void Return(JSVal val) {
    ret_ = val;
  }

  void SetStatement(Mode mode, const JSVal& val,
                    const BreakableStatement* target) {
    mode_ = mode;
    ret_ = val;
    target_ = target;
  }

  bool IsError() const {
    return error_;
  }

  const BreakableStatement* target() const {
    return target_;
  }

  JSScript* current_script() const {
    return current_script_;
  }

  void set_current_script(JSScript* script) {
    current_script_ = script;
  }

  bool IsShouldGC() {
    ++generate_script_counter_;
    if (generate_script_counter_ > 30) {
      generate_script_counter_ = 0;
      return true;
    } else {
      return false;
    }
  }

  bool InCurrentLabelSet(const AnonymousBreakableStatement* stmt) const {
    // AnonymousBreakableStatement has empty label at first
    return !target_ || stmt == target_;
  }

  bool InCurrentLabelSet(const NamedOnlyBreakableStatement* stmt) const {
    return stmt == target_;
  }

  void set_strict(bool strict) {
    strict_ = strict;
  }

  bool IsStrict() const {
    return strict_;
  }

 private:
  JSEnv* lexical_env_;
  JSEnv* variable_env_;
  std::shared_ptr<Interpreter> interp_;
  JSVal binding_;
  Mode mode_;
  JSVal ret_;
  const BreakableStatement* target_;
  Error error_;
  std::size_t generate_script_counter_;
  JSScript* current_script_;
  bool strict_;
  StackResource stack_resource_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // IV_LV5_TELEPORTER_CONTEXT_H_
