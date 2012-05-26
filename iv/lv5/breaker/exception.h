#ifndef IV_LV5_BREAKER_EXCEPTION_H_
#define IV_LV5_BREAKER_EXCEPTION_H_
#include <iv/debug.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/templates.h>
#include <iv/lv5/breaker/assembler.h>
namespace iv {
namespace lv5 {
namespace breaker {

// Search exception handler from pc, and unwind frames.
// If handler is found, return to handler pc.
// copied from railgun::VM exception handler phase
inline void* SearchExceptionHandler(void* pc, void** rsp,
                                    Frame* stack, railgun::Frame* frame) {
  Context* ctx = stack->ctx;
  Error* e = stack->error;
  assert(*e);
  const std::size_t bytecode_offset =
      frame->code()->core_data()->assembler()->PCToBytecodeOffset(pc);
  frame->MaterializeErrorStack(
      ctx, e, frame->code()->core_data()->data()->data() + bytecode_offset);
  while (true) {
    bool in_range = false;
    const railgun::ExceptionTable& table = frame->code()->exception_table();
    for (railgun::ExceptionTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it) {
      const railgun::Handler& handler = *it;
      if (in_range && handler.program_counter_begin() >= pc) {
        break;  // handler not found
      }

      // Because return address points next instruction,
      // handler range is begin < pc <= end
      if (handler.program_counter_begin() < pc &&
          pc <= handler.program_counter_end()) {
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
            const JSVal error = e->Detail(ctx);
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
      // Because frame is code frame, first lexical_env is variable_env.
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

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_EXCEPTION_H_
