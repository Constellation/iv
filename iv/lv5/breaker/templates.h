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
static const std::size_t kBreakerPrologue = 96;

class TemplatesGenerator : private Xbyak::CodeGenerator {
 public:
  TemplatesGenerator(char* ptr, std::size_t size)
    : Xbyak::CodeGenerator(size, ptr) {
    Xbyak::CodeArray::protect(ptr, size, true);
    CompileDispatchExceptionHandler(kExceptionHandlerIsNotFound);
    CompileExceptionHandlerIsNotFound(kBreakerPrologue);
    CompileBreakerPrologue(0);
  }

  void CompileDispatchExceptionHandler(std::size_t size) {
    // In this procedure, callee-save registers are the same to main code.
    // So use r12(context) and r13(frame) directly.
    // passing
    //   pc to 1st argument
    //   Frame to 2nd argument
    mov(rdi, rax);
    mov(rsi, r12);
    mov(rax, rsp);
    push(r13);
    push(rax);
    mov(rax, core::BitCast<uint64_t>(&search_exception_handler));
    call(rax);
    pop(r13);  // unwinded frame
    pop(rsp);  // calculated rsp
    jmp(rax);  // jump to exception handler
    Padding(size);
  }

  void CompileExceptionHandlerIsNotFound(std::size_t size) {
    // restore callee-save registers
    pop(r13);
    pop(r12);
    pop(rax);  // alignment element
    ret();
    Padding(size);
  }

  void CompileBreakerPrologue(std::size_t size) {
    sub(rsp, k64Size * 3);
    mov(ptr[rsp + k64Size * 1], r13);
    mov(ptr[rsp + k64Size * 2], r12);
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
};

template<typename D = void>
struct Templates {
  static void* dispatch_exception_handler() {
    return reinterpret_cast<void*>(code + kDispatchExceptionHandler);
  }

  static void* exception_handler_is_not_found() {
    return reinterpret_cast<void*>(code + kExceptionHandlerIsNotFound);
  }

  static void* breaker_prologue() {
    return reinterpret_cast<void*>(code + kBreakerPrologue);
  }

  static MIE_ALIGN(4096) char code[4096];
  static TemplatesGenerator generator;
};

template<typename D>
TemplatesGenerator Templates<D>::generator(code, 4096);

template<typename D>
char Templates<D>::code[4096];

static const uint64_t kStackPayload = 2;  // NOLINT

// Search exception handler from pc, and unwind frames.
// If handler is found, return to handler pc.
// copied from railgun::VM exception handler phase
inline void* search_exception_handler(void* pc,
                                      iv::lv5::railgun::Context* ctx,
                                      void** target) {
  using namespace iv::lv5;
  railgun::Frame** frame_out = reinterpret_cast<railgun::Frame**>(target);
  uint64_t** offset_out = (reinterpret_cast<uint64_t**>(target) + 1);
  railgun::Frame* frame = *frame_out;
  JSVal* reg = frame->RegisterFile();
  Error* e = ctx->PendingError();
  uint64_t offset = 0;
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
                static_cast<railgun::NativeIterator*>(reg[handler.ret()].cell());
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
            if (handler.type() == railgun::Handler::FINALLY) {
              reg[handler.flag()] = railgun::VM::kJumpFromFinally;
              reg[handler.jmp()] = error;
            } else {
              reg[handler.ret()] = error;
            }
            *frame_out = frame;
            *offset_out += offset * kStackPayload;
            return handler.program_counter_end();
          }
        }
      }
    }
    // handler not in this frame
    // so, unwind frame and search again
    if (frame->return_address_ == NULL) {
      // this code is invoked by native function
      // so, not unwind and return (continues to after for main loop)
      break;
    } else {
      // unwind frame
      pc = frame->return_address_;
      // because of Frame is code frame,
      // first lexical_env is variable_env.
      // (if Eval / Global, this is not valid)
      assert(frame->lexical_env() == frame->variable_env());
      frame = ctx->vm()->stack()->Unwind(frame);
      offset += 1;
      reg = frame->RegisterFile();
    }
  }
  *frame_out = frame;
  *offset_out += offset * kStackPayload;
  return Templates<>::exception_handler_is_not_found();
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_TEMPLATES_H_
