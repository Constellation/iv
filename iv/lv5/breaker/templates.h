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

static const std::size_t kDispatchExceptionHandler = 0;
static const std::size_t kExceptionHandlerIsNotFound = 48;

class TemplatesGenerator : private Xbyak::CodeGenerator {
 public:
  friend class core::Singleton<Templates>;

  Templates(char* ptr, std::size_t size)
    : Xbyak::CodeGenerator(size, ptr) {
    Xbyak::CodeArray::protect(ptr, size, true);
    dispatch_exception_handler_ = CompileDispatchExceptionHandler(kExceptionHandlerIsNotFound);
    exception_handler_is_not_found_ = CompileExceptionHandlerIsNotFound(0);
  }

  void* CompileDispatchExceptionHandler(std::size_t size) {
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
    Padding(size);
    return ptr;
  }

  void* CompileExceptionHandlerIsNotFound(std::size_t size) {
    void* ptr = core::BitCast<void*>(getCode());
    ret();
    Padding(size);
    return ptr;
  }

  void Padding(std::size_t size) {
    if (size) {
      std::size_t current = getSize();
      while (current != size) {
        nop();
        ++current;
      }
    }
  }

  void* dispatch_exception_handler_;
  void* exception_handler_is_not_found_;
};

template<typename D = void>
struct Templates {
  static void* dispatch_exception_handler() {
    return reinterpret_cast<void*>(code_ + kDispatchExceptionHandler);
  }

  static void* exception_handler_is_not_found() {
    return reinterpret_cast<void*>(code_ + kExceptionHandlerIsNotFound);
  }

  MIE_ALIGN(4096) char code_[4096];
  static TemplatesGenerator generator;
};

template<typename D>
TemplatesGenerator Templates<D>::generator(code_, 4096);

template<typename D>
char Templates<D>::code_[4096];

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TEMPLATES_H_
