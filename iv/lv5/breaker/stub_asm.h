#ifndef IV_LV5_BREAKER_STUB_ASM_H_
#define IV_LV5_BREAKER_STUB_ASM_H_
#include <iv/lv5/breaker/fwd.h>

// Search exception handler from pc, and unwind frames.
// If handler is found, return to handler pc.
// copied from railgun::VM exception handler phase
extern "C" inline void* iv_lv5_breaker_search_exception_handler(
    void* pc, railgun::Frame* frame, railgun::Context* ctx) {
  Code* code = frame->code();
  JSVal* reg = frame->RegisterFile();
  Error* e = ctx->PendingError();
  while (true) {
    bool in_range = false;
    const ExceptionTable& table = frame->code()->exception_table();
    for (ExceptionTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it) {
      const Handler& handler = *it;
      if (in_range && handler.program_counter_begin() > pc) {
        break;  // handler not found
      }
      if (handler.program_counter_begin() <= pc &&
          pc < handler.program_counter_end()) {
        in_range = true;
        switch (handler.type()) {
          case Handler::ITERATOR: {
            // control iterator lifetime
            NativeIterator* it =
                static_cast<NativeIterator*>(reg[handler.ret()].cell());
            ctx->ReleaseNativeIterator(it);
            continue;
          }

          case Handler::ENV: {
            // roll up environment
            frame->set_lexical_env(frame->lexical_env()->outer());
            continue;
          }

          default: {
            // catch or finally
            const JSVal error = JSError::Detail(ctx, e);
            e->Clear();
            if (handler.type() == Handler::FINALLY) {
              reg[handler.flag()] = kJumpFromFinally;
              reg[handler.jmp()] = error;
            } else {
              reg[handler.ret()] = error;
            }
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
      reg = frame->RegisterFile();
    }
  }
  return reinterpret_cast<void*>(&iv_lv5_breaker_exception_handler_is_not_found);
}

// In this procedure, callee-save registers are the same to main code.
// So use r12(frame) and r13(context) directly.
__asm__(
  IV_LV5_BREAKER_SYMBOL(iv_lv5_breaker_dispatch_exception_handler) ":" "\n"
  // passing
  //   pc to 1st argument
  //   Frame to 2nd argument
  "mov %rax, %rdi" "\n"
  "mov %r12, %rsi" "\n"
  "mov %r13, %rdx" "\n"
  "call " IV_LV5_BREAKER_SYMBOL(iv_lv5_breaker_search_exception_handler) "\n"
  // jump to exception handler
  "jmpq *%rax" "\n"
);

__asm__(
  IV_LV5_BREAKER_SYMBOL(iv_lv5_breaker_exception_handler_is_not_found) ":" "\n"
  // cleanup main code stack
  "ret" "\n"
);

#endif  // IV_LV5_BREAKER_STUB_ASM_H_
