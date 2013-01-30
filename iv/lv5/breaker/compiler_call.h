#ifndef IV_LV5_BREAKER_COMPILER_CALL_H_
#define IV_LV5_BREAKER_COMPILER_CALL_H_
namespace iv {
namespace lv5 {
namespace breaker {

// opcode | (callee | offset | argc_with_this)
inline void Compiler::EmitCALL(const Instruction* instr) {
  const register_t callee = Reg(instr[1].ssw.i16[0]);
  const register_t offset = Reg(instr[1].ssw.i16[1]);
  const uint32_t argc_with_this = instr[1].ssw.u32;

  const Assembler::LocalLabelScope scope(asm_);
  LoadVR(rsi, callee);

  asm_->lea(rdx, ptr[r13 + sizeof(JSVal) * argc_with_this]);

  CallIC* ic(new CallIC());
  native_code()->BindIC(ic);

  // IC JSFunction pointer
  ic->set_function_offset(helper::Generate64Mov(asm_, rax));
  asm_->cmp(rsi, rax);
  asm_->jne(".slow_call", Xbyak::CodeGenerator::T_NEAR);

  // IC real code pointer
  ic->set_executable_offset(helper::Generate64Mov(asm_, rax));
  asm_->call(rax);

  asm_->L(".unwind");
  asm_->mov(r13, ptr[r13 + offsetof(railgun::Frame, prev_)]);  // current frame
  const register_t frame_end_offset = Reg(code_->FrameSize());
  assert(!IsConstantID(frame_end_offset));
  asm_->lea(rcx, ptr[r13 + frame_end_offset * kJSValSize]);
  // rcx is new stack pointer
  asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::StackPointerOffset())], rcx);
  asm_->mov(ptr[r12 + (railgun::Context::VMOffset() + railgun::VM::StackOffset() + railgun::Stack::CurrentFrameOffset())], r13);
  asm_->jmp(".exit", Xbyak::CodeGenerator::T_NEAR);

  asm_->L(".slow_call");

  asm_->jmp(".unwind");

  asm_->L(".exit");



  asm_->mov(rdi, r14);
  assert(!IsConstantID(offset));
  asm_->lea(rdx, ptr[r13 + offset * kJSValSize]);
  asm_->mov(ecx, argc_with_this);
  asm_->mov(r8, core::BitCast<uint64_t>(instr));
  asm_->Call(&stub::CALL);
  asm_->test(rdx, rdx);
  asm_->jz(".exit");

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

  asm_->L(".exit");
}

// opcode | (callee | offset | argc_with_this)
inline void Compiler::EmitCONSTRUCT(const Instruction* instr) {
  const register_t callee = Reg(instr[1].ssw.i16[0]);
  const register_t offset = Reg(instr[1].ssw.i16[1]);
  const uint32_t argc_with_this = instr[1].ssw.u32;

  const Assembler::LocalLabelScope scope(asm_);
  asm_->mov(rdi, r14);
  LoadVR(rsi, callee);
  assert(!IsConstantID(offset));
  asm_->lea(rdx, ptr[r13 + offset * kJSValSize]);
  asm_->mov(ecx, argc_with_this);
  asm_->mov(r8, core::BitCast<uint64_t>(instr));
  asm_->Call(&stub::CONSTRUCT);
  asm_->test(rdx, rdx);
  asm_->jz(".exit");

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
  asm_->jnz(".not_object");

  // currently, rax target is guaranteed as cell
  CompareCellTag(rax, radio::OBJECT);
  asm_->je(".exit");

  // constructor call and return value is not object
  asm_->L(".not_object");
  asm_->mov(rax, rdx);

  asm_->L(".exit");
}

// opcode | (callee | offset | argc_with_this)
inline void Compiler::EmitEVAL(const Instruction* instr) {
  const register_t callee = Reg(instr[1].ssw.i16[0]);
  const register_t offset = Reg(instr[1].ssw.i16[1]);
  const uint32_t argc_with_this = instr[1].ssw.u32;

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
  asm_->jz(".exit");

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

  asm_->L(".exit");
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_CALL_H_
