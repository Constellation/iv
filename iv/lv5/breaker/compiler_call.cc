#include <iv/platform.h>
#if !defined(IV_ENABLE_JIT)
#include <iv/dummy_cc.h>
IV_DUMMY_CC()
#else

#include <iv/debug.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/railgun/instruction.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/helper.h>
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/type.h>
#include <iv/lv5/breaker/stub.h>
#include <iv/lv5/breaker/compiler.h>
#include <iv/lv5/breaker/runtime.h>
#include <iv/lv5/breaker/compiler_comparison.h>
#include <iv/lv5/breaker/entry_point.h>

namespace iv {
namespace lv5 {
namespace breaker {

// opcode | (callee | offset | argc_with_this)
void Compiler::EmitCALL(const Instruction* instr) {
  const register_t callee = Reg(instr[1].ssw.i16[0]);
  const register_t offset = Reg(instr[1].ssw.i16[1]);
  const uint32_t argc_with_this = instr[1].ssw.u32;
  {
    const Assembler::LocalLabelScope scope(asm_);
    asm_->mov(rdi, r14);
    LoadVR(rsi, callee);
    assert(!IsConstantID(offset));
    asm_->lea(rdx, ptr[r13 + offset * kJSValSize]);
    asm_->mov(ecx, argc_with_this);
    asm_->mov(r8, core::BitCast<uint64_t>(instr));
    asm_->Call(&stub::CALL);
    asm_->test(rdx, rdx);
    asm_->jz(".CALL_EXIT");

    // move to new Frame
    asm_->mov(r13, rdx);

    // call function
    asm_->call(rax);

    // unwind Frame
    asm_->mov(r13, ptr[r13 + offsetof(railgun::Frame, prev_)]);  // current frame
    const register_t frame_end_offset = Reg(code_->FrameSize());
    assert(!IsConstantID(frame_end_offset));
    asm_->lea(rcx, ptr[r13 + frame_end_offset * kJSValSize]);

    // rcx is new stack pointer
    asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::StackPointerOffset())], rcx);
    asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::CurrentFrameOffset())], r13);

    asm_->L(".CALL_EXIT");
  }
}

// opcode | (callee | offset | argc_with_this)
void Compiler::EmitCONSTRUCT(const Instruction* instr) {
  const register_t callee = Reg(instr[1].ssw.i16[0]);
  const register_t offset = Reg(instr[1].ssw.i16[1]);
  const uint32_t argc_with_this = instr[1].ssw.u32;
  {
    const Assembler::LocalLabelScope scope(asm_);
    asm_->mov(rdi, r14);
    LoadVR(rsi, callee);
    assert(!IsConstantID(offset));
    asm_->lea(rdx, ptr[r13 + offset * kJSValSize]);
    asm_->mov(ecx, argc_with_this);
    asm_->mov(r8, core::BitCast<uint64_t>(instr));
    asm_->Call(&stub::CONSTRUCT);
    asm_->test(rdx, rdx);
    asm_->jz(".CONSTRUCT_EXIT");

    // move to new Frame
    asm_->mov(r13, rdx);

    // call function
    asm_->call(rax);

    // unwind Frame
    asm_->mov(rdx, ptr[r13 + kJSValSize * Reg(railgun::FrameConstant<>::kThisOffset)]);  // NOLINT
    asm_->mov(r13, ptr[r13 + offsetof(railgun::Frame, prev_)]);  // current frame
    const register_t frame_end_offset = Reg(code_->FrameSize());
    asm_->lea(rcx, ptr[r13 + frame_end_offset * kJSValSize]);

    // rcx is new stack pointer
    asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::StackPointerOffset())], rcx);
    asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::CurrentFrameOffset())], r13);

    // after call of JS Function
    // rax is result value
    asm_->mov(rsi, detail::jsval64::kValueMask);
    asm_->test(rsi, rax);
    asm_->jnz(".RESULT_IS_NOT_OBJECT");

    // currently, rax target is guaranteed as cell
    CompareCellTag(rax, radio::OBJECT);
    asm_->je(".CONSTRUCT_EXIT");

    // constructor call and return value is not object
    asm_->L(".RESULT_IS_NOT_OBJECT");
    asm_->mov(rax, rdx);

    asm_->L(".CONSTRUCT_EXIT");
  }
}

// opcode | (callee | offset | argc_with_this)
void Compiler::EmitEVAL(const Instruction* instr) {
  const register_t callee = Reg(instr[1].ssw.i16[0]);
  const register_t offset = Reg(instr[1].ssw.i16[1]);
  const uint32_t argc_with_this = instr[1].ssw.u32;
  {
    const Assembler::LocalLabelScope scope(asm_);
    asm_->mov(rdi, r14);
    LoadVR(rsi, callee);
    assert(!IsConstantID(offset));
    asm_->lea(rdx, ptr[r13 + offset * kJSValSize]);
    asm_->mov(ecx, argc_with_this);
    asm_->mov(r8, core::BitCast<uint64_t>(instr));
    asm_->mov(r9, r13);
    asm_->Call(&stub::EVAL);
    asm_->test(rdx, rdx);
    asm_->jz(".CALL_EXIT");

    // move to new Frame
    asm_->mov(r13, rdx);

    // call function
    asm_->call(rax);

    // unwind Frame
    asm_->mov(r13, ptr[r13 + offsetof(railgun::Frame, prev_)]);  // current frame
    const register_t frame_end_offset = Reg(code_->FrameSize());
    assert(!IsConstantID(frame_end_offset));
    asm_->lea(rcx, ptr[r13 + frame_end_offset * kJSValSize]);

    // rdx is new stack pointer
    asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::StackPointerOffset())], rcx);
    asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::CurrentFrameOffset())], r13);

    asm_->L(".CALL_EXIT");
  }
}

} } }  // namespace iv::lv5::breaker
#endif
