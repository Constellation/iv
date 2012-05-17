// Templates create static functions which is used in breaker.
// But we want codes which we can control machine code completely,
// so use Xbyak and generate templates code at runtime.
#ifndef IV_LV5_BREAKER_TEMPLATES_H_
#define IV_LV5_BREAKER_TEMPLATES_H_
#include <iv/singleton.h>
#include <iv/bit_cast.h>
#include <iv/debug.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
namespace iv {
namespace lv5 {
namespace breaker {

static const std::size_t kDispatchExceptionHandler = 0;
static const std::size_t kExceptionHandlerIsNotFound = kDispatchExceptionHandler + 96;
static const std::size_t kBreakerPrologue = kExceptionHandlerIsNotFound + 96;

class TemplatesGenerator : public Xbyak::CodeGenerator {
 public:
  TemplatesGenerator(char* ptr, std::size_t size)
    : Xbyak::CodeGenerator(size, ptr) {
    Xbyak::CodeArray::protect(ptr, size, true);
    CompileDispatchExceptionHandler(kExceptionHandlerIsNotFound);
    CompileExceptionHandlerIsNotFound(kBreakerPrologue);
    CompileBreakerPrologue(0);

    prologue =
        core::BitCast<TemplatesGenerator::PrologueType>(ptr + kBreakerPrologue);
  }

  void CompileDispatchExceptionHandler(std::size_t size) {
    // In this procedure, callee-save registers are the same to main code.
    // So use r12(context) and r13(frame) directly.
    // passing
    //   pc to 1st argument
    //   Frame to 2nd argument
    mov(rdi, rax);
    mov(rsi, rsp);
    mov(rdx, r14);
    mov(rcx, r13);
    mov(rax, core::BitCast<uint64_t>(&search_exception_handler));
    call(rax);
    mov(r13, ptr[r14 + offsetof(Frame, frame)]);
    mov(rsp, ptr[r14 + offsetof(Frame, rsp)]);
    jmp(rax);  // jump to exception handler
    Padding(size);
  }

  void CompileExceptionHandlerIsNotFound(std::size_t size) {
    // restore callee-save registers
    mov(r14, ptr[rsp + offsetof(Frame, r14)]);
    mov(r13, ptr[rsp + offsetof(Frame, r13)]);
    mov(r12, ptr[rsp + offsetof(Frame, r12)]);
    add(rsp, sizeof(Frame));
    ret();
    Padding(size);
  }

  typedef JSVal(*PrologueType)(railgun::Context* ctx,
                               railgun::Frame* frame, void* code, Error* e);
  // rdi : context
  // rsi : frame
  // rdx : code ptr
  // rcx : error ptr
  void CompileBreakerPrologue(std::size_t size) {
    sub(rsp, sizeof(Frame));

    // construct breaker::Frame
    mov(ptr[rsp + offsetof(Frame, r14)], r14);
    mov(ptr[rsp + offsetof(Frame, r13)], r13);
    mov(ptr[rsp + offsetof(Frame, r12)], r12);
    mov(ptr[rsp + offsetof(Frame, ctx)], rdi);
    mov(ptr[rsp + offsetof(Frame, frame)], rsi);
    mov(ptr[rsp + offsetof(Frame, error)], rcx);
    mov(r12, rdi);
    mov(r13, rsi);
    mov(r14, rsp);
    lea(rcx, ptr[rsp - k64Size * 2]);
    mov(ptr[rsp + offsetof(Frame, rsp)], rcx);
    lea(rcx, ptr[rsp - k64Size]);
    mov(ptr[rsp + offsetof(Frame, ret)], rcx);

    // initialize return address of frame
    call(rdx);

    mov(r14, ptr[rsp + offsetof(Frame, r14)]);
    mov(r13, ptr[rsp + offsetof(Frame, r13)]);
    mov(r12, ptr[rsp + offsetof(Frame, r12)]);
    add(rsp, sizeof(Frame));

    ret();
    Padding(size);
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

 public:  // opened
  PrologueType prologue;
};

template<typename D = void>
struct Templates {
  static void* dispatch_exception_handler() {
    return reinterpret_cast<void*>(code + kDispatchExceptionHandler);
  }

  static void* exception_handler_is_not_found() {
    return reinterpret_cast<void*>(code + kExceptionHandlerIsNotFound);
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

static const uint64_t kStackPayload = 2;  // NOLINT

// Search exception handler from pc, and unwind frames.
// If handler is found, return to handler pc.
// copied from railgun::VM exception handler phase
inline void* search_exception_handler(void* pc, void** rsp,
                                      Frame* stack, railgun::Frame* frame) {
  using namespace iv::lv5;  // NOLINT
  railgun::Context* ctx = stack->ctx;
  Error* e = stack->error;
  assert(*e);
  while (true) {
    bool in_range = false;
    const railgun::ExceptionTable& table = frame->code()->exception_table();
    for (railgun::ExceptionTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it) {
      const railgun::Handler& handler = *it;
      if (in_range && handler.program_counter_begin() > pc) {
        break;  // handler not found
      }
      if (handler.program_counter_begin() <= pc &&
          pc < handler.program_counter_end()) {
        in_range = true;
        switch (handler.type()) {
          case railgun::Handler::ITERATOR: {
            // control iterator lifetime
            railgun::NativeIterator* it =
                static_cast<railgun::NativeIterator*>(
                    frame->RegisterFile()[handler.ret()].cell());
            ctx->ReleaseNativeIterator(it);
            continue;
          }

          case railgun::Handler::ENV: {
            // roll up environment
            frame->set_lexical_env(frame->lexical_env()->outer());
            continue;
          }

          default: {
            // catch or finally
            const JSVal error = JSError::Detail(ctx, e);
            e->Clear();
            JSVal* reg = frame->RegisterFile();
            if (handler.type() == railgun::Handler::FINALLY) {
              reg[handler.flag()] = railgun::VM::kJumpFromFinally;
              reg[handler.jmp()] = error;
            } else {
              reg[handler.ret()] = error;
            }
            stack->rsp = rsp;
            stack->frame = frame;
            stack->ret = rsp - 1;
            return handler.program_counter_end();
          }
        }
      }
    }
    // handler not in this frame
    // so, unwind frame and search again
    if (frame->prev_pc_ == NULL) {
      // this code is invoked by native function
      // so, not unwind and return (continues to after for main loop)
      break;
    } else {
      // unwind frame
      rsp = rsp + kStackPayload;
      pc = *core::BitCast<void**>(rsp - 1);
      // because of Frame is code frame,
      // first lexical_env is variable_env.
      // (if Eval / Global, this is not valid)
      assert(frame->lexical_env() == frame->variable_env());
      frame = ctx->vm()->stack()->Unwind(frame);
    }
  }
  rsp = rsp + kStackPayload;
  stack->rsp = rsp;
  stack->frame = frame;
  stack->ret = rsp - 1;
  return Templates<>::exception_handler_is_not_found();
}

inline JSVal breaker_prologue(railgun::Context* ctx,
                              railgun::Frame* frame, void* ptr, Error* e) {
  return Templates<>::generator.prologue(ctx, frame, ptr, e);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TEMPLATES_H_
