// breaker::Compiler
//
// This compiler parses railgun::opcodes and emit native code.
// Some primitive operation and branch operation is emitted as raw native code,
// and basic complex opcode is emitted as call to stub function, that is,
// Context Threading JIT.
//
// Stub function implementations are in stub.h
#ifndef IV_LV5_BREAKER_COMPILER_H_
#define IV_LV5_BREAKER_COMPILER_H_
#include <iv/debug.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/executable.h>
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/stub.h>
namespace iv {
namespace lv5 {
namespace breaker {

class Compiler {
 public:
  typedef std::unordered_map<railgun::Code*, std::size_t> EntryPointMap;
  typedef std::unordered_map<uint32_t, std::size_t> JumpMap;

  // introducing railgun to this scope
  typedef railgun::Instruction Instruction;

  static const int kJSValSize = sizeof(JSVal);

  Compiler(railgun::Code* top)
    : top_(top),
      code_(NULL),
      asm_(new(PointerFreeGC)Assembler),
      jump_map_(),
      entry_points_(),
      counter_(0) {
    top_->core_data()->set_asm(asm_);
  }

  ~Compiler() {
    // Compilation is finished.
    // Link jumps / calls and set each entry point to Code.
    asm_->ready();
    for (EntryPointMap::const_iterator it = entry_points_.begin(),
         last = entry_points_.end(); it != last; ++it) {
      it->first->set_executable(asm_->GainExecutableByOffset(it->second));
    }
  }

  void Initialize(railgun::Code* code) {
    code_ = code;
    jump_map_.clear();
    entry_points_.insert(std::make_pair(code, asm_->size()));
  }

  void Compile(railgun::Code* code) {
    Initialize(code);
    ScanJumps();
    Main();
  }

  // scan jump target for setting labels
  void ScanJumps() {
    // jump opcodes
    //   IF_FALSE
    //   IF_TRUE
    //   JUMP_SUBROUTINE
    //   JUMP_BY
    //   FORIN_SETUP
    //   FORIN_ENUMERATE
    namespace r = railgun;

    const Instruction* first_instr = code_->begin();
    for (const Instruction* instr = code_->begin(),
         *last = code_->end(); instr != last;) {
      const uint32_t opcode = instr->GetOP();
      const uint32_t length = r::kOPLength[opcode];
      const int32_t index = instr - first_instr;
      switch (opcode) {
        case r::OP::IF_FALSE:
        case r::OP::IF_TRUE:
        case r::OP::JUMP_SUBROUTINE:
        case r::OP::JUMP_BY:
        case r::OP::FORIN_SETUP:
        case r::OP::FORIN_ENUMERATE: {
          const int32_t jump = instr[1].jump.to;
          const uint32_t to = index + jump;
          jump_map_.insert(std::make_pair(to, counter_++));
          break;
        }
      }
      std::advance(instr, length);
    }

    // and handlers table
    const r::ExceptionTable& table = code_->exception_table();
    for (r::ExceptionTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it) {
      const r::Handler& handler = *it;
      jump_map_.insert(std::make_pair(handler.begin(), counter_++));
      jump_map_.insert(std::make_pair(handler.end(), counter_++));
    }
  }

  void Main() {
    namespace r = railgun;

    // generate local label scope
    const Assembler::LocalLabelScope local_label_scope(asm_);

    // emit prologue

    // general storage space
    // We can access this space by qword[rsp + k64Size * 0]
    asm_->push(asm_->r12);

    const Instruction* first_instr = code_->begin();
    for (const Instruction* instr = code_->begin(),
         *last = code_->end(); instr != last;) {
      const uint32_t opcode = instr->GetOP();
      const uint32_t length = r::kOPLength[opcode];
      const int32_t index = instr - first_instr;

      const JumpMap::iterator it = jump_map_.find(index);
      if (it != jump_map_.end()) {
        asm_->L(MakeLabel(it->second).c_str());
      }

      switch (opcode) {
        case r::OP::NOP:
          EmitNOP(instr);
          break;
        case r::OP::MV:
          EmitMV(instr);
          break;
        case r::OP::UNARY_POSITIVE:
          EmitUNARY_POSITIVE(instr);
          break;
        case r::OP::UNARY_NEGATIVE:
          EmitUNARY_NEGATIVE(instr);
          break;
        case r::OP::UNARY_NOT:
          EmitUNARY_NOT(instr);
          break;
        case r::OP::UNARY_BIT_NOT:
          EmitUNARY_BIT_NOT(instr);
          break;
        case r::OP::BINARY_ADD:
          EmitBINARY_ADD(instr);
          break;
        case r::OP::BINARY_SUBTRACT:
          EmitBINARY_SUBTRACT(instr);
          break;
        case r::OP::BINARY_MULTIPLY:
          EmitBINARY_MULTIPLY(instr);
          break;
        case r::OP::BINARY_DIVIDE:
          EmitBINARY_DIVIDE(instr);
          break;
        case r::OP::BINARY_MODULO:
          EmitBINARY_MODULO(instr);
          break;
        case r::OP::BINARY_LSHIFT:
          EmitBINARY_LSHIFT(instr);
          break;
        case r::OP::BINARY_RSHIFT:
          EmitBINARY_RSHIFT(instr);
          break;
        case r::OP::BINARY_RSHIFT_LOGICAL:
          EmitBINARY_RSHIFT_LOGICAL(instr);
          break;
        case r::OP::BINARY_LT:
          EmitBINARY_LT(instr);
          break;
        case r::OP::BINARY_LTE:
          EmitBINARY_LTE(instr);
          break;
        case r::OP::BINARY_GT:
          EmitBINARY_GT(instr);
          break;
        case r::OP::BINARY_GTE:
          EmitBINARY_GTE(instr);
          break;
        case r::OP::BINARY_INSTANCEOF:
          EmitBINARY_INSTANCEOF(instr);
          break;
        case r::OP::LOAD_UNDEFINED:
          EmitLOAD_UNDEFINED(instr);
          break;
        case r::OP::DEBUGGER:
          EmitDEBUGGER(instr);
          break;
        case r::OP::CALL:
          EmitCALL(instr);
          break;
        case r::OP::RETURN:
          EmitRETURN(instr);
          break;
        case r::OP::RESULT:
          EmitRESULT(instr);
          break;
        case r::OP::LOAD_CONST:
          EmitLOAD_CONST(instr);
          break;
        case r::OP::LOAD_GLOBAL:
          EmitLOAD_GLOBAL(instr);
          break;
        case r::OP::STORE_GLOBAL:
          EmitSTORE_GLOBAL(instr);
          break;
        case r::OP::LOAD_FUNCTION:
          EmitLOAD_FUNCTION(instr);
          break;
        case r::OP::TO_PRIMITIVE_AND_TO_STRING:
          EmitTO_PRIMITIVE_AND_TO_STRING(instr);
          break;
        case r::OP::CONCAT:
          EmitCONCAT(instr);
          break;
        case r::OP::INSTANTIATE_DECLARATION_BINDING:
          EmitINSTANTIATE_DECLARATION_BINDING(instr);
          break;
        case r::OP::INSTANTIATE_VARIABLE_BINDING:
          EmitINSTANTIATE_VARIABLE_BINDING(instr);
          break;
        case r::OP::INCREMENT:
          EmitINCREMENT(instr);
          break;
        case r::OP::DECREMENT:
          EmitDECREMENT(instr);
          break;
        case r::OP::POSTFIX_INCREMENT:
          EmitPOSTFIX_INCREMENT(instr);
          break;
        case r::OP::POSTFIX_DECREMENT:
          EmitPOSTFIX_DECREMENT(instr);
          break;
        case r::OP::IF_TRUE:
          EmitIF_TRUE(instr);
          break;
        case r::OP::IF_FALSE:
          EmitIF_FALSE(instr);
          break;
        case r::OP::JUMP_BY:
          EmitJUMP_BY(instr);
          break;
        case r::OP::THROW:
          EmitTHROW(instr);
          break;
      }
      std::advance(instr, length);
    }
  }

  // Emitters
  //
  // JIT Convension is
  //  caller-save
  //   rax : tmp
  //   rcx : tmp
  //   rdx : tmp
  //   r10 : tmp
  //   r11 : tmp
  //
  //  callee-save
  //   r12 : context
  //   r13 : frame

  static int16_t Reg(int16_t reg) {
    return railgun::FrameConstant<>::kFrameSize + reg;
  }

  // opcode
  void EmitNOP(const Instruction* instr) {
  }

  // opcode | (dst | src)
  void EmitMV(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    asm_->mov(asm_->r10, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->r10);
  }

  // opcode | (size | mutable_start)
  void EmitBUILD_ENV(const Instruction* instr) {
    const uint32_t size = instr[1].u32[0];
    const uint32_t mutable_start = instr[1].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->r13);
    asm_->mov(asm_->rdx, size);
    asm_->mov(asm_->rcx, mutable_start);
    asm_->Call(&stub::BUILD_ENV);
  }

  // opcode | src
  void EmitWITH_SETUP(const Instruction* instr) {
    const int16_t src = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->r13);
    asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->Call(&stub::WITH_SETUP);
  }

  // opcode | (dst | offset)
  void EmitLOAD_CONST(const Instruction* instr) {
    // extract constant bytes
    // append immediate value to machine code
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[1].ssw.u32;
    const uint64_t bytes = Extract(code_->constants()[offset]);
    if (bytes <= UINT32_MAX) {
      // only mov m64, imm32 is allowed
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], bytes);
    } else {
      asm_->mov(asm_->rax, bytes);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_ADD(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_ADD_SLOW_GENERIC");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_ADD_SLOW_GENERIC");
      AddingInt32OverflowGuard(asm_->esi,
                               asm_->edx, asm_->eax, ".BINARY_ADD_SLOW_NUMBER");
      asm_->mov(asm_->esi, asm_->eax);
      asm_->mov(asm_->rdi, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rdi);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_ADD_EXIT");

      // rdi and rsi is always int32 (but overflow)
      // So we just add as int64_t and convert to double,
      // because INT32_MAX + INT32_MAX is in int64_t range, and convert to
      // double makes no error.
      asm_->L(".BINARY_ADD_SLOW_NUMBER");
      asm_->movsxd(asm_->rsi, asm_->esi);
      asm_->movsxd(asm_->rdx, asm_->edx);
      asm_->add(asm_->rsi, asm_->rdx);
      asm_->cvtsi2sd(asm_->xmm0, asm_->rsi);
      asm_->movq(asm_->rsi, asm_->xmm0);
      ConvertNotNaNDoubleToJSVal(asm_->rsi, asm_->rcx);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_ADD_EXIT");

      asm_->L(".BINARY_ADD_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_ADD);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_ADD_EXIT");
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_SUBTRACT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_SUBTRACT_SLOW_GENERIC");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_SUBTRACT_SLOW_GENERIC");
      SubtractingInt32OverflowGuard(asm_->esi,
                                    asm_->edx, asm_->eax, ".BINARY_SUBTRACT_SLOW_NUMBER");
      asm_->mov(asm_->esi, asm_->eax);
      asm_->mov(asm_->rdi, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rdi);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_SUBTRACT_EXIT");

      // rdi and rsi is always int32 (but overflow)
      // So we just sub as int64_t and convert to double,
      // because INT32_MIN - INT32_MIN is in int64_t range, and convert to
      // double makes no error.
      asm_->L(".BINARY_SUBTRACT_SLOW_NUMBER");
      asm_->movsxd(asm_->rsi, asm_->esi);
      asm_->movsxd(asm_->rdx, asm_->edx);
      asm_->sub(asm_->rsi, asm_->rdx);
      asm_->cvtsi2sd(asm_->xmm0, asm_->rsi);
      asm_->movq(asm_->rsi, asm_->xmm0);
      ConvertNotNaNDoubleToJSVal(asm_->rsi, asm_->rcx);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_SUBTRACT_EXIT");

      asm_->L(".BINARY_SUBTRACT_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_SUBTRACT);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_SUBTRACT_EXIT");
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_MULTIPLY(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_MULTIPLY_SLOW_GENERIC");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_MULTIPLY_SLOW_GENERIC");
      MultiplyingInt32OverflowGuard(asm_->esi,
                                    asm_->edx, asm_->eax, ".BINARY_MULTIPLY_SLOW_NUMBER");
      asm_->mov(asm_->esi, asm_->eax);
      asm_->mov(asm_->rdi, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rdi);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_MULTIPLY_EXIT");

      // rdi and rsi is always int32 (but overflow)
      asm_->L(".BINARY_MULTIPLY_SLOW_NUMBER");
      asm_->cvtsi2sd(asm_->xmm0, asm_->esi);
      asm_->cvtsi2sd(asm_->xmm1, asm_->edx);
      asm_->mulsd(asm_->xmm0, asm_->xmm1);
      asm_->movq(asm_->rsi, asm_->xmm0);
      ConvertNotNaNDoubleToJSVal(asm_->rsi, asm_->rcx);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_MULTIPLY_EXIT");

      asm_->L(".BINARY_MULTIPLY_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_MULTIPLY);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_MULTIPLY_EXIT");
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_DIVIDE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      asm_->Call(&stub::BINARY_DIVIDE);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_MODULO(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_MODULO_SLOW_GENERIC");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_MODULO_SLOW_GENERIC");
      // check rhs is more than 0 (n % 0 == NaN)
      // lhs is >= 0 and rhs is > 0 because example like
      //   -1 % -1
      // should return -0.0, so this value is double
      asm_->cmp(asm_->esi, 0);
      asm_->jl(".BINARY_MODULO_SLOW_GENERIC");
      asm_->cmp(asm_->edx, 0);
      asm_->jle(".BINARY_MODULO_SLOW_GENERIC");

      asm_->mov(asm_->eax, asm_->esi);
      asm_->mov(asm_->ecx, asm_->edx);
      asm_->mov(asm_->edx, 0);
      asm_->idiv(asm_->ecx);

      asm_->mov(asm_->rdi, detail::jsval64::kNumberMask);
      asm_->add(asm_->rdx, asm_->rdi);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rdx);
      asm_->jmp(".BINARY_MODULO_EXIT");

      asm_->L(".BINARY_MODULO_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_MODULO);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_MODULO_EXIT");
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_LSHIFT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rcx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rdx, ".BINARY_LSHIFT_SLOW_GENERIC");
      Int32Guard(asm_->rcx, asm_->rax, asm_->rdx, ".BINARY_LSHIFT_SLOW_GENERIC");
      asm_->sal(asm_->esi, asm_->cl);
      asm_->mov(asm_->rdx, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rdx);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_LSHIFT_EXIT");

      asm_->L(".BINARY_LSHIFT_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rdx, asm_->rcx);
      asm_->Call(&stub::BINARY_LSHIFT);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_LSHIFT_EXIT");
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_RSHIFT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rcx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rdx, ".BINARY_RSHIFT_SLOW_GENERIC");
      Int32Guard(asm_->rcx, asm_->rax, asm_->rdx, ".BINARY_RSHIFT_SLOW_GENERIC");
      asm_->sar(asm_->esi, asm_->cl);
      asm_->mov(asm_->rdx, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rdx);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_RSHIFT_EXIT");

      asm_->L(".BINARY_RSHIFT_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rdx, asm_->rcx);
      asm_->Call(&stub::BINARY_RSHIFT);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_RSHIFT_EXIT");
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_RSHIFT_LOGICAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rcx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rdx, ".BINARY_RSHIFT_LOGICAL_SLOW_GENERIC");
      Int32Guard(asm_->rcx, asm_->rax, asm_->rdx, ".BINARY_RSHIFT_LOGICAL_SLOW_GENERIC");
      asm_->shr(asm_->esi, asm_->cl);
      asm_->cmp(asm_->esi, 0);
      asm_->jl(".BINARY_RSHIFT_LOGICAL_DOUBLE");  // uint32_t
      asm_->mov(asm_->rdx, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rdx);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_RSHIFT_LOGICAL_EXIT");

      asm_->L(".BINARY_RSHIFT_LOGICAL_DOUBLE");
      asm_->cvtsi2sd(asm_->xmm0, asm_->rsi);
      asm_->movq(asm_->rsi, asm_->xmm0);
      ConvertNotNaNDoubleToJSVal(asm_->rsi, asm_->rcx);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".BINARY_RSHIFT_LOGICAL_EXIT");

      asm_->L(".BINARY_RSHIFT_LOGICAL_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rdx, asm_->rcx);
      asm_->Call(&stub::BINARY_RSHIFT_LOGICAL);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_RSHIFT_LOGICAL_EXIT");
    }
  }

  // TODO(Constellation) refactoring emitter for binary lt / lte / gt / gte
  // fusion opcode (IF_FALSE / IF_TRUE)
  // opcode | (dst | lhs | rhs)
  void EmitBINARY_LT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_LT_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_LT_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_LT and IF_FALSE
      asm_->setl(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_LT_EXIT");
      asm_->L(".BINARY_LT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_LT);
      asm_->L(".BINARY_LT_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // fusion opcode (IF_FALSE / IF_TRUE)
  // opcode | (dst | lhs | rhs)
  void EmitBINARY_LTE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_LTE_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_LTE_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_LTE and IF_FALSE
      asm_->setle(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_LTE_EXIT");
      asm_->L(".BINARY_LTE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_LTE);
      asm_->L(".BINARY_LTE_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // fusion opcode (IF_FALSE / IF_TRUE)
  // opcode | (dst | lhs | rhs)
  void EmitBINARY_GT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_GT_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_GT_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_GT and IF_FALSE
      asm_->setg(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_GT_EXIT");
      asm_->L(".BINARY_GT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_GT);
      asm_->L(".BINARY_GT_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // fusion opcode (IF_FALSE / IF_TRUE)
  // opcode | (dst | lhs | rhs)
  void EmitBINARY_GTE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_GTE_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_GTE_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_GTE and IF_FALSE
      asm_->setge(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_GTE_EXIT");
      asm_->L(".BINARY_GTE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_GTE);
      asm_->L(".BINARY_GTE_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_INSTANCEOF(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      asm_->Call(&stub::BINARY_INSTANCEOF);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".BINARY_LSHIFT_EXIT");
    }
  }

  // opcode | dst
  void EmitLOAD_UNDEFINED(const Instruction* instr) {
    static const uint64_t layout = Extract(JSUndefined);
    const int16_t dst = Reg(instr[1].i32[0]);
    assert(layout <= UINT32_MAX);
    asm_->mov(asm_->qword[asm_->r13 + (dst * kJSValSize)], layout);
  }

  // opcode | dst
  void EmitLOAD_EMPTY(const Instruction* instr) {
    static const uint64_t layout = Extract(JSEmpty);
    const int16_t dst = Reg(instr[1].i32[0]);
    assert(layout <= UINT32_MAX);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | dst
  void EmitLOAD_NULL(const Instruction* instr) {
    static const uint64_t layout = Extract(JSNull);
    const int16_t dst = Reg(instr[1].i32[0]);
    assert(layout <= UINT32_MAX);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | dst
  void EmitLOAD_TRUE(const Instruction* instr) {
    static const uint64_t layout = Extract(JSTrue);
    const int16_t dst = Reg(instr[1].i32[0]);
    assert(layout <= UINT32_MAX);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | dst
  void EmitLOAD_FALSE(const Instruction* instr) {
    static const uint64_t layout = Extract(JSFalse);
    const int16_t dst = Reg(instr[1].i32[0]);
    assert(layout <= UINT32_MAX);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | (dst | src)
  void EmitUNARY_POSITIVE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      IsNumber(asm_->rsi, asm_->rax);
      asm_->jnz(".UNARY_POSITIVE_FAST");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::TO_NUMBER);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->jmp(".UNARY_POSITIVE_EXIT");
      asm_->L(".UNARY_POSITIVE_FAST");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->L(".UNARY_POSITIVE_EXIT");
    }
  }

  // opcode | (dst | src)
  void EmitUNARY_NEGATIVE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".UNARY_NEGATIVE_SLOW");
      asm_->mov(asm_->eax, asm_->esi);
      asm_->mov(asm_->rdi, detail::jsval64::kNumberMask);
      asm_->add(asm_->rax, asm_->rdi);
      asm_->jmp(".UNARY_NEGATIVE_EXIT");
      asm_->L(".UNARY_NEGATIVE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::UNARY_NEGATIVE);
      asm_->L(".UNARY_NEGATIVE_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // TODO(Constellation) fusion opcode
  // opcode | (dst | src)
  void EmitUNARY_NOT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->Call(&stub::UNARY_NOT);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | src)
  void EmitUNARY_BIT_NOT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".UNARY_BIT_NOT_SLOW");
      asm_->not(asm_->esi);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rsi);
      asm_->jmp(".UNARY_BIT_NOT_EXIT");
      asm_->L(".UNARY_BIT_NOT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::UNARY_BIT_NOT);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
      asm_->L(".UNARY_BIT_NOT_EXIT");
    }
  }

  // opcode | src
  void EmitTHROW(const Instruction* instr) {
    const int16_t src = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->Call(&stub::THROW);
  }

  // opcode
  void EmitDEBUGGER(const Instruction* instr) {
    // debug mode only
#if defined(DEBUG)
    asm_->Breakpoint();
#endif
  }

  // opcode | src
  void EmitTO_PRIMITIVE_AND_TO_STRING(const Instruction* instr) {
    const int16_t src = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->Call(&stub::TO_PRIMITIVE_AND_TO_STRING);
    asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], asm_->rax);
  }

  // opcode | (dst | start | count)
  void EmitCONCAT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const int16_t start = Reg(instr[1].ssw.i16[1]);
    const uint32_t count = instr[1].ssw.u32;
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->lea(asm_->rsi, asm_->ptr[asm_->r13 + start * kJSValSize]);
    asm_->mov(asm_->edx, count);
    asm_->Call(&stub::CONCAT);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode
  void EmitPOP_ENV(const Instruction* instr) {
    // TODO(Constellation) inline this function
    asm_->mov(asm_->rdi, asm_->r13);
    asm_->Call(&stub::POP_ENV);
  }

  // opcode | (dst | size)
  void EmitLOAD_ARRAY(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t size = instr[1].ssw.u32;
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, size);
    asm_->Call(&JSArray::ReservedNew);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | code)
  void EmitLOAD_FUNCTION(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t code = instr[1].ssw.u32;
    railgun::Code* target = code_->codes()[code];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(target));
    asm_->mov(asm_->rdx,
              asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->Call(&railgun::JSVMFunction::New);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | offset)
  void EmitLOAD_REGEXP(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[1].ssw.u32;
    JSRegExp* regexp =
        static_cast<JSRegExp*>(code_->constants()[offset].object());
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(regexp));
    asm_->Call(static_cast<JSRegExp*(*)(Context*, JSRegExp*)>(&JSRegExp::New));
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | dst
  void EmitRESULT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | src
  void EmitRETURN(const Instruction* instr) {
    // Calling convension is different from railgun::VM.
    // Constructor checks value is object in call site, not callee site.
    // So r13 is still callee Frame.
    const int16_t src = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rax, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->add(asm_->rsp, k64Size);
    asm_->ret();
  }

  // opcode | (callee | offset | argc_with_this)
  void EmitCONSTRUCT(const Instruction* instr) {
    const Assembler::LocalLabelScope scope(asm_);

    // after call of JS Function
    // rax is result value
    asm_->mov(asm_->rsi, detail::jsval64::kValueMask);
    asm_->and(asm_->rsi, asm_->rax);
    asm_->jnz(".RESULT_IS_OK");  // not cell
    asm_->test(asm_->rax, asm_->rax);
    asm_->jz(".RESULT_IS_OK");  // empty
    // currently, rax target is garanteed as object
    asm_->mov(asm_->rcx, asm_->ptr[asm_->rax + IV_OFFSETOF(radio::Cell, next_address_of_freelist_or_storage_)]);  // NOLINT
    asm_->shr(asm_->rcx, radio::Color::kOffset);
    asm_->cmp(asm_->rcx, radio::OBJECT);
    asm_->je(".RESULT_IS_OK");
    // constructor call and return value is not object
    asm_->mov(asm_->rax, asm_->ptr[asm_->r13 - kJSValSize * railgun::FrameConstant<>::kThisOffset]);  // NOLINT
    asm_->L(".RESULT_IS_OK");
  }

  // opcode | (dst | index) | nop | nop
  void EmitLOAD_GLOBAL(const Instruction* instr) {
    const uint32_t index = instr[1].ssw.u32;
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[index];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(name));
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(instr));
    if (code_->strict()) {
      asm_->Call(&stub::LOAD_GLOBAL<true>);
    } else {
      asm_->Call(&stub::LOAD_GLOBAL<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (src | name) | nop | nop
  void EmitSTORE_GLOBAL(const Instruction* instr) {
    const uint32_t index = instr[1].ssw.u32;
    const int16_t src = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[index];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    asm_->mov(asm_->rcx, core::BitCast<uint64_t>(instr));
    if (code_->strict()) {
      asm_->Call(&stub::STORE_GLOBAL<true>);
    } else {
      asm_->Call(&stub::STORE_GLOBAL<false>);
    }
  }

  // opcode | (callee | offset | argc_with_this)
  void EmitCALL(const Instruction* instr) {
    const int16_t callee = Reg(instr[1].ssw.i16[0]);
    const int16_t offset = Reg(instr[1].ssw.i16[1]);
    const uint32_t argc_with_this = instr[1].ssw.u32;
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + callee * kJSValSize]);
      asm_->lea(asm_->rdx, asm_->ptr[asm_->r13 + offset * kJSValSize]);
      asm_->mov(asm_->rcx, argc_with_this);
      asm_->mov(asm_->r8, asm_->rsp);
      asm_->mov(asm_->qword[asm_->rsp], asm_->r13);
      asm_->Call(&stub::CALL);
      asm_->mov(asm_->rcx, asm_->qword[asm_->rsp]);
      asm_->cmp(asm_->rcx, asm_->r13);
      asm_->je(".CALL_EXIT");
      // move to new Frame
      asm_->mov(asm_->r13, asm_->rcx);
      asm_->call(asm_->rax);
      // unwind Frame
      asm_->mov(asm_->rcx, asm_->r13);  // old frame
      asm_->mov(asm_->r13, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, prev_)]); // current frame
      const int16_t frame_end_offset = Reg(code_->registers());
      asm_->lea(asm_->rbx, asm_->ptr[asm_->r13 + frame_end_offset * kJSValSize]);
      asm_->cmp(asm_->rcx, asm_->rbx);
      asm_->jge(".CALL_UNWIND_OLD");
      asm_->mov(asm_->rcx, asm_->rbx);
      asm_->L(".CALL_UNWIND_OLD");
      // rcx is new stack pointer
      asm_->mov(asm_->rbx, asm_->ptr[asm_->r12 + IV_OFFSETOF(railgun::Context, vm_)]);
      asm_->mov(asm_->ptr[asm_->rbx + (IV_OFFSETOF(railgun::VM, stack_) + IV_OFFSETOF(railgun::Stack, stack_pointer_))], asm_->rcx);
      asm_->mov(asm_->ptr[asm_->rbx + (IV_OFFSETOF(railgun::VM, stack_) + IV_OFFSETOF(railgun::Stack, current_))], asm_->r13);
      asm_->L(".CALL_EXIT");
    }
  }

  // opcode | (name | configurable)
  void EmitINSTANTIATE_DECLARATION_BINDING(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].u32[0]];
    const bool configurable = instr[1].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, variable_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (configurable) {
      asm_->Call(&stub::INSTANTIATE_DECLARATION_BINDING<true>);
    } else {
      asm_->Call(&stub::INSTANTIATE_DECLARATION_BINDING<false>);
    }
  }

  // opcode | (name | configurable)
  void EmitINSTANTIATE_VARIABLE_BINDING(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].u32[0]];
    const bool configurable = instr[1].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, variable_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (configurable) {
      if (code_->strict()) {
        asm_->Call(&stub::INSTANTIATE_VARIABLE_BINDING<true, true>);
      } else {
        asm_->Call(&stub::INSTANTIATE_VARIABLE_BINDING<true, false>);
      }
    } else {
      if (code_->strict()) {
        asm_->Call(&stub::INSTANTIATE_VARIABLE_BINDING<false, true>);
      } else {
        asm_->Call(&stub::INSTANTIATE_VARIABLE_BINDING<false, false>);
      }
    }
  }

  // opcode | src
  void EmitINCREMENT(const Instruction* instr) {
    const int16_t src = Reg(instr[1].i32[0]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MAX) + 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".INCREMENT_SLOW");
      asm_->inc(asm_->esi);
      asm_->jo(".INCREMENT_OVERFLOW");

      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rax);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rsi);
      asm_->jmp(".INCREMENT_EXIT");

      asm_->L(".INCREMENT_OVERFLOW");
      // overflow ==> INT32_MAX + 1
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], overflow);
      asm_->jmp(".INCREMENT_EXIT");

      asm_->L(".INCREMENT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::INCREMENT);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rax);

      asm_->L(".INCREMENT_EXIT");
    }
  }

  // opcode | src
  void EmitDECREMENT(const Instruction* instr) {
    const int16_t src = Reg(instr[1].i32[0]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MIN) - 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".DECREMENT_SLOW");
      asm_->sub(asm_->esi, 1);
      asm_->jo(".DECREMENT_OVERFLOW");

      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rax);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rsi);
      asm_->jmp(".DECREMENT_EXIT");

      // overflow ==> INT32_MIN - 1
      asm_->L(".DECREMENT_OVERFLOW");
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], overflow);
      asm_->jmp(".DECREMENT_EXIT");

      asm_->L(".DECREMENT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::DECREMENT);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rax);

      asm_->L(".DECREMENT_EXIT");
    }
  }

  // opcode | (dst | src)
  void EmitPOSTFIX_INCREMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MAX) + 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->rsi);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".INCREMENT_SLOW");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rdx);
      asm_->inc(asm_->esi);
      asm_->jo(".INCREMENT_OVERFLOW");

      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rax);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rsi);
      asm_->jmp(".INCREMENT_EXIT");

      // overflow ==> INT32_MAX + 1
      asm_->L(".INCREMENT_OVERFLOW");
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], overflow);
      asm_->jmp(".INCREMENT_EXIT");

      asm_->L(".INCREMENT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->lea(asm_->rdx, asm_->qword[asm_->r13 + dst * kJSValSize]);
      asm_->Call(&stub::POSTFIX_INCREMENT);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rax);

      asm_->L(".INCREMENT_EXIT");
    }
  }

  // opcode | (dst | src)
  void EmitPOSTFIX_DECREMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MIN) - 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->rsi);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".DECREMENT_SLOW");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rdx);
      asm_->sub(asm_->esi, 1);
      asm_->jo(".DECREMENT_OVERFLOW");

      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->add(asm_->rsi, asm_->rax);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rsi);
      asm_->jmp(".DECREMENT_EXIT");

      // overflow ==> INT32_MIN - 1
      asm_->L(".DECREMENT_OVERFLOW");
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], overflow);
      asm_->jmp(".DECREMENT_EXIT");

      asm_->L(".DECREMENT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->lea(asm_->rdx, asm_->ptr[asm_->r13 + dst * kJSValSize]);
      asm_->Call(&stub::POSTFIX_DECREMENT);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rax);

      asm_->L(".DECREMENT_EXIT");
    }
  }

  // opcode | (jmp | cond)
  void EmitIF_TRUE(const Instruction* instr) {
    // TODO(Constelation) inlining this
    const int16_t cond = Reg(instr[1].jump.i16[0]);
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second;
    const std::string label = MakeLabel(num);
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + cond * kJSValSize]);
    asm_->Call(&stub::TO_BOOLEAN);
    asm_->test(asm_->eax, asm_->eax);
    asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
  }

  // opcode | (jmp | cond)
  void EmitIF_FALSE(const Instruction* instr) {
    // TODO(Constelation) inlining this
    const int16_t cond = Reg(instr[1].jump.i16[0]);
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second;
    const std::string label = MakeLabel(num);
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + cond * kJSValSize]);
    asm_->Call(&stub::TO_BOOLEAN);
    asm_->test(asm_->eax, asm_->eax);
    asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
  }

  // opcode | jmp
  void EmitJUMP_BY(const Instruction* instr) {
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second;
    const std::string label = MakeLabel(num);
    asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
  }

  // leave flags
  void IsNumber(const Xbyak::Reg64& reg, const Xbyak::Reg64& tmp) {
    asm_->mov(tmp, detail::jsval64::kNumberMask);
    asm_->and(tmp, reg);
  }

  // NaN is not handled
  void ConvertNotNaNDoubleToJSVal(const Xbyak::Reg64& target,
                                  const Xbyak::Reg64& tmp) {
    asm_->mov(tmp, detail::jsval64::kDoubleOffset);
    asm_->add(target, tmp);
  }

  void ConvertBooleanToJSVal(const Xbyak::Reg64& target) {
    asm_->or(Xbyak::Reg32(target.getIdx()), detail::jsval64::kBooleanRepresentation);
  }

  void Int32Guard(const Xbyak::Reg64& target,
                  const Xbyak::Reg64& tmp1,
                  const Xbyak::Reg64& tmp2, const char* label) {
    asm_->mov(tmp1, detail::jsval64::kNumberMask);
    asm_->and(tmp1, target);
    asm_->mov(tmp2, detail::jsval64::kNumberMask);
    asm_->cmp(tmp1, tmp2);
    asm_->jne(label);
  }

  void AddingInt32OverflowGuard(const Xbyak::Reg32& lhs,
                                const Xbyak::Reg32& rhs,
                                const Xbyak::Reg32& out, const char* label) {
    asm_->mov(out, lhs);
    asm_->add(out, rhs);
    asm_->jo(label);
  }

  void SubtractingInt32OverflowGuard(const Xbyak::Reg32& lhs,
                                     const Xbyak::Reg32& rhs,
                                     const Xbyak::Reg32& out, const char* label) {
    asm_->mov(out, lhs);
    asm_->sub(out, rhs);
    asm_->jo(label);
  }

  void MultiplyingInt32OverflowGuard(const Xbyak::Reg32& lhs,
                                     const Xbyak::Reg32& rhs,
                                     const Xbyak::Reg32& out, const char* label) {
    asm_->mov(out, lhs);
    asm_->or(out, rhs);
    asm_->shr(out, 15);
    asm_->jnz(label);
    asm_->mov(out, lhs);
    asm_->imul(out, rhs);
  }

  static std::string MakeLabel(std::size_t num) {
    std::string str("IV_LV5_BREAKER_JT_");
    str.reserve(str.size() + 10);
    core::detail::UIntToStringWithRadix<uint64_t>(num,
                                                  std::back_inserter(str), 32);
    return str;
  }

  railgun::Code* top_;
  railgun::Code* code_;
  Assembler* asm_;
  JumpMap jump_map_;
  EntryPointMap entry_points_;
  std::size_t counter_;
};

inline void CompileInternal(Compiler* compiler, railgun::Code* code) {
  compiler->Compile(code);
  for (railgun::Code::Codes::const_iterator it = code->codes().begin(),
       last = code->codes().end(); it != last; ++it) {
    CompileInternal(compiler, *it);
  }
}

inline void Compile(railgun::Code* code) {
  Compiler compiler(code);
  CompileInternal(&compiler, code);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_H_
