#ifndef IV_LV5_RAILGUN_VM_FWD_H_
#define IV_LV5_RAILGUN_VM_FWD_H_
#include <iv/arith.h>
#include <iv/lv5/arguments.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/operation.h>
#include <iv/lv5/railgun/stack.h>
#include <iv/lv5/railgun/direct_threading.h>
namespace iv {
namespace lv5 {
namespace railgun {
class VM {
 public:
  static const int32_t kJumpFromSubroutine = 0;
  static const int32_t kJumpFromReturn = 1;
  static const int32_t kJumpFromFinally = 2;

  inline JSVal Run(Code* code, Error* e);

  inline JSVal RunEval(Code* code,
                       JSEnv* variable_env, JSEnv* lexical_env,
                       JSVal this_binding, Error* e);

  inline JSVal Execute(Arguments* args, JSVMFunction* func, Error* e);

  // normal pass
  explicit VM(Context* ctx)
    : ctx_(ctx),
      operation_(ctx),
      stack_(),
      direct_threading_dispatch_table_(NULL) {
  }

  template<OP::Type op>
  static bool IsOP(const Instruction& inst) {
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
    return GetLabel<op>() == inst.label;
#else
    return op == inst.u32;
#endif
  }

  Stack* stack() {
    return &stack_;
  }

  const Stack* stack() const {
    return &stack_;
  }

#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
 private:
  // dispatch table get pass
  explicit VM(DispatchTableTag tag)
    : ctx_(NULL),
      operation_(NULL),
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

 private:
  // internal Execute
  // VM main routine
  inline JSVal Execute(Frame* frame, Error* e);

  Context* ctx_;
  Operation operation_;
  Stack stack_;
  const DirectThreadingDispatchTable* direct_threading_dispatch_table_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_VM_FWD_H_
