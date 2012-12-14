// Templates create static functions which is used in breaker.
// But we want codes which we can control machine code completely,
// so use Xbyak and generate templates code at runtime.
#ifndef IV_LV5_BREAKER_TEMPLATES_H_
#define IV_LV5_BREAKER_TEMPLATES_H_
#include <iv/bit_cast.h>
#include <iv/debug.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
namespace iv {
namespace lv5 {
namespace breaker {

static const uint64_t kStackPayload = 2;  // NOLINT

class TemplatesGenerator : public Xbyak::CodeGenerator {
 public:
  TemplatesGenerator(char* ptr, std::size_t size)
    : Xbyak::CodeGenerator(size, ptr) {
    Xbyak::CodeArray::protect(ptr, size, true);
    dispatch_exception_handler = CompileDispatchExceptionHandler();
    CompileBreakerPrologue();
  }

  void* CompileDispatchExceptionHandler() {
    // In this procedure, callee-save registers are the same to main code.
    // So use r12(context) and r13(frame) directly.
    // passing
    //   pc to 1st argument
    //   Frame to 2nd argument
    const std::size_t size = getSize();
    mov(rdi, rax);
    mov(rsi, rsp);
    mov(rdx, r14);
    mov(rcx, r13);
    mov(rax, core::BitCast<uint64_t>(&SearchExceptionHandler));
    call(rax);
    mov(r13, ptr[r14 + offsetof(Frame, frame)]);
    mov(rsp, ptr[r14 + offsetof(Frame, rsp)]);
    jmp(rax);  // jump to exception handler
    return core::BitCast<void*>(getCode() + size);
  }

  typedef JSVal(*PrologueType)(Context* ctx,
                               railgun::Frame* frame, void* code, Error* e);
  // rdi : context
  // rsi : frame
  // rdx : code ptr
  // rcx : error ptr
  void CompileBreakerPrologue() {
    const std::size_t size = getSize();
    sub(rsp, sizeof(Frame));

    // construct breaker::Frame
    mov(ptr[rsp + offsetof(Frame, r15)], r15);
    mov(ptr[rsp + offsetof(Frame, r14)], r14);
    mov(ptr[rsp + offsetof(Frame, r13)], r13);
    mov(ptr[rsp + offsetof(Frame, r12)], r12);
    mov(ptr[rsp + offsetof(Frame, ctx)], rdi);
    mov(ptr[rsp + offsetof(Frame, frame)], rsi);
    mov(ptr[rsp + offsetof(Frame, error)], rcx);
    mov(r12, rdi);
    mov(r13, rsi);
    mov(r14, rsp);
    mov(r15, detail::jsval64::kNumberMask);
    lea(rcx, ptr[rsp - k64Size * 2]);
    mov(ptr[rsp + offsetof(Frame, rsp)], rcx);
    lea(rcx, ptr[rsp - k64Size]);
    mov(ptr[rsp + offsetof(Frame, ret)], rcx);

    // initialize return address of frame
    call(rdx);

    // exception or normal return
    const std::size_t not_found = getSize();

    mov(r15, ptr[rsp + offsetof(Frame, r15)]);
    mov(r14, ptr[rsp + offsetof(Frame, r14)]);
    mov(r13, ptr[rsp + offsetof(Frame, r13)]);
    mov(r12, ptr[rsp + offsetof(Frame, r12)]);
    add(rsp, sizeof(Frame));

    ret();

    exception_handler_is_not_found = core::BitCast<void*>(getCode() + not_found);
    prologue = core::BitCast<PrologueType>(getCode() + size);
  }

 public:  // opened
  PrologueType prologue;
  void* dispatch_exception_handler;
  void* exception_handler_is_not_found;
};

template<typename D = void>
struct Templates {
  static void* dispatch_exception_handler() {
    return Templates<>::generator.dispatch_exception_handler;
  }

  static void* exception_handler_is_not_found() {
    return Templates<>::generator.exception_handler_is_not_found;
  }

  static MIE_ALIGN(4096) char code[4096];
  static TemplatesGenerator generator;
};

template<typename D>
TemplatesGenerator Templates<D>::generator(code, 4096);

template<typename D>
char Templates<D>::code[4096];

struct ForceInstantiate {
  ForceInstantiate() { Templates<>::generator.getCode(); }
};

inline JSVal breaker_prologue(Context* ctx,
                              railgun::Frame* frame, void* ptr, Error* e) {
  return Templates<>::generator.prologue(ctx, frame, ptr, e);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TEMPLATES_H_
