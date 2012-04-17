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

  Compiler()
    : code_(NULL),
      asm_(new(PointerFree)Assembler),
      entry_points_() {
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
          jump_map_.insert(std::make_pair(to, 0));
          break;
        }
      }
      std::advance(it, length);
    }

    // and handlers table
    const ExceptionTable& table = code_->exception_table();
    for (ExceptionTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it) {
      const Handler& handler = *it;
      jump_map_.insert(std::make_pair(handler.begin(), 0));
      jump_map_.insert(std::make_pair(handler.end(), 0));
    }
  }

  void Main() {
    namespace r = railgun;

    // generate local label scope
    const Assembler::LocalLabelScope local_label_scope(asm_);

    // emit prologue

    // push for alignment
    asm_->push(asm_->r12);

    const Instruction* first_instr = code_->begin();
    for (const Instruction* instr = code_->begin(),
         *last = code_->end(); instr != last;) {
      const uint32_t opcode = instr->GetOP();
      const uint32_t length = r::kOPLength[opcode];
      const int32_t index = instr - first_instr;

      const JumpMap::const_iterator it = jump_map_.find(index);
      if (it != jump_map_.end()) {
        it->second = asm_->size();
      }

      switch (opcode) {
      }
      std::advance(it, length);
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
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->r10);
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
    const uint64_t bytes = code_->constants()[offset].Layout().bytes_;
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], bytes);
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_ADD(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      inLocallabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, ".BINARY_ADD_SLOW_GENERIC");
      Int32Guard(asm_->rdx, asm_->rax, ".BINARY_ADD_SLOW_GENERIC");
      AddingInt32OverflowGuard(asm_->esi,
                               asm_->edx, asm_->rax, ".BINARY_ADD_SLOW_NUMBER");
      asm_->mov(asm_->rsi, detail::jsval64::kNumberMask);
      asm_->mov(asm_->esi, asm_->eax);
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rsi);
      jmp(".BINARY_ADD_EXIT");
      L(".BINARY_ADD_SLOW_NUMBER");
      // rdi and rsi is always int32 (but overflow)
      // So we just add as int64_t and convert to double,
      // because INT32_MAX + INT32_MAX is in int64_t range, and convert to
      // double makes no error.
      asm_->movsxd(asm_->rsi, asm_->esi);
      asm_->movsxd(asm_->rdx, asm_->edx);
      asm_->add(asm_->rsi, asm_->rdx);
      asm_->cvtsi2sd(asm_->rsi, asm_->rsi);
      ConvertNotNaNDoubleToJSVal(asm_->rsi);
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rsi);
      jmp(".BINARY_ADD_EXIT");
      L(".BINARY_ADD_SLOW_GENERIC");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_ADD);
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      L(".BINARY_ADD_EXIT");
      outLocalLabel();
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
      inLocalLabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, ".BINARY_LT_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, ".BINARY_LT_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_LT and IF_FALSE
      asm_->setl(asm_->rax);
      ConvertBooleanToJSVal(asm_->rax);
      jmp(".BINARY_LT_EXIT");
      L(".BINARY_LT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_LT);
      L(".BINARY_LT_EXIT");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      outLocalLabel();
    }
  }

  // fusion opcode (IF_FALSE / IF_TRUE)
  // opcode | (dst | lhs | rhs)
  void EmitBINARY_LTE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      inLocalLabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, ".BINARY_LTE_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, ".BINARY_LTE_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_LTE and IF_FALSE
      asm_->setle(asm_->rax);
      ConvertBooleanToJSVal(asm_->rax);
      jmp(".BINARY_LTE_EXIT");
      L(".BINARY_LTE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_LTE);
      L(".BINARY_LTE_EXIT");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      outLocalLabel();
    }
  }

  // fusion opcode (IF_FALSE / IF_TRUE)
  // opcode | (dst | lhs | rhs)
  void EmitBINARY_GT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      inLocalLabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, ".BINARY_GT_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, ".BINARY_GT_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_GT and IF_FALSE
      asm_->setg(asm_->rax);
      ConvertBooleanToJSVal(asm_->rax);
      jmp(".BINARY_GT_EXIT");
      L(".BINARY_GT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_GT);
      L(".BINARY_GT_EXIT");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      outLocalLabel();
    }
  }

  // fusion opcode (IF_FALSE / IF_TRUE)
  // opcode | (dst | lhs | rhs)
  void EmitBINARY_GTE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      inLocalLabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, ".BINARY_GTE_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, ".BINARY_GTE_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_GTE and IF_FALSE
      asm_->setge(asm_->rax);
      ConvertBooleanToJSVal(asm_->rax);
      jmp(".BINARY_GTE_EXIT");
      L(".BINARY_GTE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_GTE);
      L(".BINARY_GTE_EXIT");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      outLocalLabel();
    }
  }

  // opcode | dst
  void EmitLOAD_UNDEFINED(const Instruction* instr) {
    static const uint64_t layout = JSVal(JSUndefined).Layout.bytes_;
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | dst
  void EmitLOAD_EMPTY(const Instruction* instr) {
    static const uint64_t layout = JSVal(JSEmpty).Layout.bytes_;
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | dst
  void EmitLOAD_NULL(const Instruction* instr) {
    static const uint64_t layout = JSVal(JSNull).Layout.bytes_;
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | dst
  void EmitLOAD_TRUE(const Instruction* instr) {
    static const uint64_t layout = JSVal(JSTrue).Layout.bytes_;
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | dst
  void EmitLOAD_FALSE(const Instruction* instr) {
    static const uint64_t layout = JSVal(JSFalse).Layout.bytes_;
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | (dst | src)
  void EmitUNARY_POSITIVE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    {
      inLocalLabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      IsNumber(asm_->rsi, asm_->rax);
      jnz(".UNARY_POSITIVE_FAST");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::TO_NUMBER);
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      jmp(".UNARY_POSITIVE_EXIT");
      L(".UNARY_POSITIVE_FAST");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rsi);
      L(".UNARY_POSITIVE_EXIT");
      outLocalLabel();
    }
  }

  // opcode | (dst | src)
  void EmitUNARY_NEGATIVE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    {
      inLocalLabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, ".UNARY_NEGATIVE_SLOW");
      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->mov(asm_->eax, asm_->esi);
      jmp(".UNARY_NEGATIVE_EXIT");
      L(".UNARY_NEGATIVE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::UNARY_NEGATIVE);
      L(".UNARY_NEGATIVE_EXIT");
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      outLocalLabel();
    }
  }

  // TODO(Constellation) fusion opcode
  // opcode | (dst | src)
  void EmitUNARY_NOT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->Call(&stub::UNARY_NOT);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | src)
  void EmitUNARY_BIT_NOT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    {
      inLocalLabel();
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, ".UNARY_BIT_NOT_SLOW");
      asm_->not(asm_->esi);
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rsi);
      jmp(".UNARY_BIT_NOT_EXIT");
      L(".UNARY_BIT_NOT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::UNARY_BIT_NOT);
      asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
      L(".UNARY_BIT_NOT_EXIT");
      outLocalLabel();
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

  // opcode | (dst | start | count)
  void EmitCONCAT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const int16_t start = Reg(instr[1].ssw.i16[1]);
    const uint32_t count = instr[1].ssw.u32;
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->lea(asm_->rsi, asm_->ptr[asm_->r13 + start * kJSValSize]);
    asm_->mov(asm_->rdx, count);
    asm_->Call(
        static_cast<JSString*(*)(Context*, JSVal*, uint32_t)>(&JSString::New));
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
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
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | code)
  void EmitLOAD_FUNCTION(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t code = instr[1].ssw.u32;
    Code* target = code_->codes()[code];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, target);
    asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + offsetof(Frame, lexical_env_)]);
    asm_->Call(&JSVMFunction::New);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | offset)
  void EmitLOAD_REGEXP(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[1].ssw.u32;
    JSRegExp* regexp = static_cast<JSRegExp*>(frame->GetConstant(offset).object());
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, regexp);
    asm_->Call(&JSRegExp::New);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | dst
  void EmitRESULT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | src
  void EmitRETURN(const Instruction* instr) {
    const int16_t src = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->mov(asm_->rsi, asm_->r13);
    asm_->Call(&stub::RETURN);
    // restore previous frame
    asm_->mov(asm_->r13, ptr[asm_->r13 + offsetof(railgun::Frame, prev_)]);
    asm_->pop(asm_->rcx);  // discard alignment element
    asm_->ret();
  }

  // leave flags
  void IsNumber(const Reg64& reg, Reg64& tmp) {
    asm_->mov(tmp, reg);
    asm_->and(tmp, detail::jsval64::kNumberMask);
  }

  // NaN is not handled
  void ConvertNotNaNDoubleToJSVal(Reg64& target) {  // NOLINT
    asm_->add(target, detail::jsval64::kDoubleOffset);
  }

  void ConvertBooleanToJSVal(Reg64& target) {  // NOLINT
    asm_->or(target, detail::jsval64::kBooleanRepresentation);
  }

  void Int32Guard(const Reg64& target, Reg64& tmp, const char* label) {  // NOLINT
    asm_->mov(tmp, target);
    asm_->and(tmp, detail::jsval64::kNumberMask);
    asm_->cmp(tmp, detail::jsval64::kNumberMask);
    asm_->je(label);
  }

  void AddingInt32OverflowGuard(const Reg32& lhs, const Reg32& rhs,
                                Reg32& out, const char* label) {  // NOLINT
    asm_->mov(out, lhs);
    asm_->add(out, rhs);
    asm_->jo(label);
  }

  static std::string MakeLabel(std::size_t num) {
    std::string str("IV_LV5_BREAKER_JT_");
    str.reserve(str.size() + 10);
    core::detail::UIntToStringWithRadix<uint64_t>(num, std::back_inserter(str), 32);
    return str;
  }

  railgun::Code* code_;
  Assembler* asm_;
  JumpMap jump_map_;
  EntryPointMap entry_points_;
};

inline void CompileInternal(Compiler* compiler, railgun::Code* code) {
  compiler->Compile(code);
  for (Code::Codes::const_iterator it = code->codes().begin(),
       last = code->codes().end(); it != last; ++it) {
    CompileInternal(compiler, *it);
  }
}

inline void Compile(railgun::Code* code) {
  Compiler compiler;
  CompileInternal(compiler, code);
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_H_
