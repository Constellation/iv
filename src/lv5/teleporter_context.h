#ifndef _IV_LV5_TELEPORTER_CONTEXT_H_
#define _IV_LV5_TELEPORTER_CONTEXT_H_
#include "lv5/context.h"
#include "lv5/teleporter_fwd.h"
#include "lv5/teleporter_interpreter.h"
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
      interp_(),
      binding_(global_data()->global_obj()),
      mode_(NORMAL),
      ret_(),
      target_(NULL),
      generate_script_counter_(0),
      current_script_(NULL) {
    interp_.set_context(this);
    Initialize();
  }

  class ScriptScope : private core::Noncopyable<ScriptScope>::type {
   public:
    ScriptScope(Context* ctx, teleporter::JSScript* script)
      : ctx_(ctx),
        prev_(ctx->current_script()) {
      ctx_->set_current_script(script);
    }
    ~ScriptScope() {
      ctx_->set_current_script(prev_);
    }
   private:
    Context* ctx_;
    teleporter::JSScript* prev_;
  };

  bool Run(teleporter::JSScript* script);

  JSVal this_binding() const {
    return binding_;
  }

  void set_this_binding(const JSVal& binding) {
    binding_ = binding;
  }

  teleporter::Interpreter* interp() {
    return &interp_;
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

  JSVal ErrorVal();

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

  teleporter::JSScript* current_script() const {
    return current_script_;
  }

  void set_current_script(teleporter::JSScript* script) {
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

 private:
  teleporter::Interpreter interp_;
  JSVal binding_;
  Mode mode_;
  JSVal ret_;
  const BreakableStatement* target_;
  Error error_;
  std::size_t generate_script_counter_;
  teleporter::JSScript* current_script_;
};

} } }  // namespace iv::lv5::teleporter
#endif  // _IV_LV5_TELEPORTER_CONTEXT_H_
