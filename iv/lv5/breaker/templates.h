// Templates create static functions which is used in breaker.
// But we want codes which we can control machine code completely,
// so use Xbyak and generate templates code at runtime.
#ifndef IV_LV5_BREAKER_TEMPLATES_H_
#define IV_LV5_BREAKER_TEMPLATES_H_
#include <iv/singleton.h>
#include <iv/bit_cast.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Templates
  : private core::Singleton<Templates>,
    private Xbyak::CodeGenerator {
 public:
  friend class core::Singleton<Templates>;

  void* dispatch_exception_handler() const {
    return dispatch_exception_handler_;
  }

  void* exception_handler_is_not_found() const {
    return exception_handler_is_not_found_;
  }

 private:
  Templates()
    : core::Singleton<Templates>(),
      Xbyak::CodeGenerator(),
      dispatch_exception_handler_(NULL),
      exception_handler_is_not_found_(NULL) {
    dispatch_exception_handler_ = CompileDispatchExceptionHandler();
    exception_handler_is_not_found_ = CompileExceptionHandlerIsNotFound();
  }

  ~Templates() { }

  void* CompileDispatchExceptionHandler() {
    // In this procedure, callee-save registers are the same to main code.
    // So use r12(context) and r13(frame) directly.
    void* ptr = core::BitCast<void*>(getCode());

    // passing
    //   pc to 1st argument
    //   Frame to 2nd argument
    mov(rdi, rax);
    mov(rsi, r12);
    mov(rax, rsp);
    push(r13);
    push(rax);
    mov(rax, &iv_lv5_breaker_search_exception_handler);
    call(rax);
    pop(r13);  // unwinded frame
    pop(rsp);  // calculated rsp
    jmp(rax);  // jump to exception handler
    return ptr;
  }

  void* CompileExceptionHandlerIsNotFound() {
    void* ptr = core::BitCast<void*>(getCode());
    ret();
    return ptr;
  }

  void* dispatch_exception_handler_;
  void* exception_handler_is_not_found_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TEMPLATES_H_
