#ifndef IV_LV5_RAILGUN_VM_FWD_H_
#define IV_LV5_RAILGUN_VM_FWD_H_
#include <iv/arith.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/operation.h>
#include <iv/lv5/railgun/stack.h>
#include <iv/lv5/railgun/direct_threading.h>
#include <iv/lv5/railgun/statistics.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Context;

class VM : private Operation {
 public:
  friend class breaker::Compiler;
  static const int32_t kJumpFromSubroutine = 0;
  static const int32_t kJumpFromReturn = 1;
  static const int32_t kJumpFromFinally = 2;

  inline JSVal Run(Code* code, Error* e);

  inline JSVal RunEval(Code* code,
                       JSEnv* variable_env, JSEnv* lexical_env,
                       JSVal this_binding, Error* e);

  inline JSVal Execute(Arguments* args, JSVMFunction* func, Error* e);

  // normal pass
  explicit VM(lv5::Context* ctx)
    : Operation(ctx),
      stack_(),
      direct_threading_dispatch_table_(NULL),
      statistics_() {
  }

  template<OP::Type op>
  static bool IsOP(const Instruction& inst) {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
    return GetLabel<op>() == inst.label;
#else
    return op == inst.u32[0];
#endif
  }

  Stack* stack() {
    return &stack_;
  }

  const Stack* stack() const {
    return &stack_;
  }

  void DumpStatistics() const {
    statistics_.Dump();
  }

  static std::size_t StackOffset() {
    return IV_OFFSETOF(VM, stack_);
  }

#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
 private:
  // dispatch table get pass
  explicit VM(DispatchTableTag tag)
    : Operation(NULL),
      stack_(tag),
      direct_threading_dispatch_table_(NULL) {
    Execute(NULL, NULL);
  }

  static LabelOPTable CreateLabelTable() {
    LabelOPTable result;
    const DirectThreadingDispatchTable& table = VM::DispatchTable();
    std::size_t index = 0;
    for (DirectThreadingDispatchTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it, ++index) {
      result.insert(std::make_pair(*it, static_cast<OP::Type>(index)));
    }
    return result;
  }

 public:
  // opcode to label table
  static const DirectThreadingDispatchTable& DispatchTable() {
    static const VM vm(DIRECT_THREADED_DISPATCH_TABLE);
    return *vm.direct_threading_dispatch_table_;
  }

  // label to opcode table
  static const LabelOPTable& LabelTable() {
    static const LabelOPTable table(CreateLabelTable());
    return table;
  }

  template<OP::Type op>
  static const void* GetLabel() {
    return DispatchTable()[op];
  }
#endif

  Context* ctx() const;

 private:
  // internal Execute
  // VM main routine
  inline JSVal Execute(Frame* frame, Error* e);

  static void VerifyDynamicEnvironment(Frame* frame) {
    JSEnv* env = frame->lexical_env()->outer();
    for (; env; env = env->outer()) {
      if (frame->variable_env() == env) {
        break;
      }
    }
    assert(env == frame->variable_env());
  }

  Stack stack_;
  const DirectThreadingDispatchTable* direct_threading_dispatch_table_;
  Statistics statistics_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_VM_FWD_H_
