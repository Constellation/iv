#ifndef _IV_LV5_TELEPORTER_CONTEXT_H_
#define _IV_LV5_TELEPORTER_CONTEXT_H_
#include <tr1/memory>
#include "lv5/context.h"
#include "lv5/jserror.h"
#include "lv5/jsfunction.h"
#include "lv5/teleporter/fwd.h"
#include "lv5/teleporter/interpreter.h"
#include "lv5/teleporter/jsscript.h"
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
      interp_(new Interpreter(this)),
      binding_(global_data()->global_obj()),
      mode_(NORMAL),
      ret_(),
      target_(NULL),
      generate_script_counter_(0),
      current_script_(NULL),
      strict_(false) {
    Initialize(
        JSInlinedFunction<&FunctionConstructor, 1>::NewPlain(this));
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

  JSVal this_binding() const {
    return binding_;
  }

  void set_this_binding(const JSVal& binding) {
    binding_ = binding;
  }

  std::tr1::shared_ptr<Interpreter> interp() {
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
  std::tr1::shared_ptr<Interpreter> interp_;
  JSVal binding_;
  Mode mode_;
  JSVal ret_;
  const BreakableStatement* target_;
  Error error_;
  std::size_t generate_script_counter_;
  JSScript* current_script_;
  bool strict_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // _IV_LV5_TELEPORTER_CONTEXT_H_
