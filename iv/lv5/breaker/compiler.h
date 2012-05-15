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

static const std::size_t kEncodeRotateN = core::math::CTZ64(lv5::detail::jsval64::kDoubleOffset);

class Compiler {
 public:
  // introducing railgun to this scope
  typedef railgun::Instruction Instruction;

  class JumpInfo {
   public:
    JumpInfo(std::size_t c, bool handled = false)
      : counter(c),
        offset(0),
        exception_handled(handled) {
    }
    JumpInfo()
      : counter(0),
        offset(0),
        exception_handled(false) {
    }
    std::size_t counter;
    std::size_t offset;
    bool exception_handled;
  };

  typedef std::unordered_map<railgun::Code*, std::size_t> EntryPointMap;
  typedef std::unordered_map<uint32_t, JumpInfo> JumpMap;
  typedef std::unordered_map<std::size_t, Assembler::RepatchSite> UnresolvedAddressMap;
  typedef std::vector<Assembler::RepatchSite> RepatchSites;
  typedef std::vector<railgun::Code*> Codes;
  typedef std::unordered_map<const Instruction*, std::size_t> HandlerLinks;

  static const int kJSValSize = sizeof(JSVal);

  explicit Compiler(railgun::Code* top)
    : top_(top),
      code_(NULL),
      asm_(new(PointerFreeGC)Assembler),
      jump_map_(),
      entry_points_(),
      unresolved_address_map_(),
      handler_links_(),
      codes_(),
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

    // Repatch phase
    // link jump subroutine
    {
      for (UnresolvedAddressMap::const_iterator it = unresolved_address_map_.begin(),
           last = unresolved_address_map_.end(); it != last; ++it) {
        uint64_t ptr = core::BitCast<uint64_t>(asm_->GainExecutableByOffset(it->first));
        assert(ptr % 2 == 0);
        // encode ptr value to JSVal invalid number
        // double offset value is 1000000000000000000000000000000000000000000000000
        ptr += 0x1;
        const uint64_t result = (ptr << kEncodeRotateN) | (ptr >> (64 - kEncodeRotateN));
        it->second.Repatch(asm_, result);
      }
    }
    // link exception handlers
    {
      for (Codes::const_iterator it = codes_.begin(),
           last = codes_.end(); it != last; ++it) {
        railgun::Code* code = *it;
        const Instruction* first_instr = code->begin();
        railgun::ExceptionTable& table = code->exception_table();
        for (railgun::ExceptionTable::iterator it = table.begin(),
             last = table.end(); it != last; ++it) {
          railgun::Handler& handler = *it;
          const Instruction* begin = first_instr + handler.begin();
          const Instruction* end = first_instr + handler.end();
          assert(handler_links_.find(begin) != handler_links_.end());
          assert(handler_links_.find(end) != handler_links_.end());
          handler.set_program_counter_begin(
              asm_->GainExecutableByOffset(handler_links_.find(begin)->second));
          handler.set_program_counter_end(
              asm_->GainExecutableByOffset(handler_links_.find(end)->second));
        }
      }
    }
  }

  void Initialize(railgun::Code* code) {
    code_ = code;
    codes_.push_back(code);
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
          jump_map_.insert(std::make_pair(to, JumpInfo(counter_++)));
          break;
        }
      }
      std::advance(instr, length);
    }

    // and handlers table
    const r::ExceptionTable& table = code_->exception_table();
    for (r::ExceptionTable::const_iterator it = table.begin(),
         last = table.end(); it != last; ++it) {
      // should override (because handled option is true)
      const r::Handler& handler = *it;
      jump_map_[handler.begin()] = JumpInfo(counter_++, true);
      jump_map_[handler.end()] = JumpInfo(counter_++, true);
    }
  }

  void CheckBlock(const Instruction* instr) {
    const int32_t index = instr - code_->begin();
    const JumpMap::iterator it = jump_map_.find(index);
    if (it != jump_map_.end()) {
      asm_->L(MakeLabel(it->second.counter).c_str());
      if (it->second.exception_handled) {
        // store handler range
        handler_links_.insert(std::make_pair(instr, asm_->size()));
      }
    }
  }

  void Main() {
    namespace r = railgun;

    // generate local label scope
    const Assembler::LocalLabelScope local_label_scope(asm_);

    // emit prologue

    // general storage space
    // We can access this space by qword[rsp + k64Size * 0] a.k.a. qword[rsp]
    asm_->push(asm_->r12);

    for (const Instruction* instr = code_->begin(),
         *last = code_->end(); instr != last;) {
      const uint32_t opcode = instr->GetOP();
      const uint32_t length = r::kOPLength[opcode];

      CheckBlock(instr);

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
        case r::OP::BINARY_IN:
          EmitBINARY_IN(instr);
          break;
        case r::OP::BINARY_EQ:
          EmitBINARY_EQ(instr);
          break;
        case r::OP::BINARY_STRICT_EQ:
          EmitBINARY_STRICT_EQ(instr);
          break;
        case r::OP::BINARY_NE:
          EmitBINARY_NE(instr);
          break;
        case r::OP::BINARY_STRICT_NE:
          EmitBINARY_STRICT_NE(instr);
          break;
        case r::OP::BINARY_BIT_AND:
          EmitBINARY_BIT_AND(instr);
          break;
        case r::OP::BINARY_BIT_XOR:
          EmitBINARY_BIT_XOR(instr);
          break;
        case r::OP::BINARY_BIT_OR:
          EmitBINARY_BIT_OR(instr);
          break;
        case r::OP::RETURN:
          EmitRETURN(instr);
          break;
        case r::OP::THROW:
          EmitTHROW(instr);
          break;
        case r::OP::POP_ENV:
          EmitPOP_ENV(instr);
          break;
        case r::OP::WITH_SETUP:
          EmitWITH_SETUP(instr);
          break;
        case r::OP::RETURN_SUBROUTINE:
          EmitRETURN_SUBROUTINE(instr);
          break;
        case r::OP::DEBUGGER:
          EmitDEBUGGER(instr);
          break;
        case r::OP::LOAD_UNDEFINED:
          EmitLOAD_UNDEFINED(instr);
          break;
        case r::OP::LOAD_TRUE:
          EmitLOAD_TRUE(instr);
          break;
        case r::OP::LOAD_FALSE:
          EmitLOAD_FALSE(instr);
          break;
        case r::OP::LOAD_NULL:
          EmitLOAD_NULL(instr);
          break;
        case r::OP::LOAD_EMPTY:
          EmitLOAD_EMPTY(instr);
          break;
        case r::OP::LOAD_REGEXP:
          EmitLOAD_REGEXP(instr);
          break;
        case r::OP::LOAD_OBJECT:
          EmitLOAD_OBJECT(instr);
          break;
        case r::OP::LOAD_ELEMENT:
          EmitLOAD_ELEMENT(instr);
          break;
        case r::OP::STORE_ELEMENT:
          EmitSTORE_ELEMENT(instr);
          break;
        case r::OP::DELETE_ELEMENT:
          EmitDELETE_ELEMENT(instr);
          break;
        case r::OP::INCREMENT_ELEMENT:
          EmitINCREMENT_ELEMENT(instr);
          break;
        case r::OP::DECREMENT_ELEMENT:
          EmitDECREMENT_ELEMENT(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_ELEMENT:
          EmitPOSTFIX_INCREMENT_ELEMENT(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_ELEMENT:
          EmitPOSTFIX_DECREMENT_ELEMENT(instr);
          break;
        case r::OP::TO_NUMBER:
          EmitTO_NUMBER(instr);
          break;
        case r::OP::TO_PRIMITIVE_AND_TO_STRING:
          EmitTO_PRIMITIVE_AND_TO_STRING(instr);
          break;
        case r::OP::CONCAT:
          EmitCONCAT(instr);
          break;
        case r::OP::RAISE_REFERENCE:
          EmitRAISE_REFERENCE(instr);
          break;
        case r::OP::RAISE_IMMUTABLE:
          EmitRAISE_IMMUTABLE(instr);
          break;
        case r::OP::TYPEOF:
          EmitTYPEOF(instr);
          break;
        case r::OP::STORE_OBJECT_DATA:
          EmitSTORE_OBJECT_DATA(instr);
          break;
        case r::OP::STORE_OBJECT_GET:
          EmitSTORE_OBJECT_GET(instr);
          break;
        case r::OP::STORE_OBJECT_SET:
          EmitSTORE_OBJECT_SET(instr);
          break;
        // case r::OP::LOAD_PROP:
        // case r::OP::LOAD_PROP_GENERIC:
        // case r::OP::LOAD_PROP_OWN:
        // case r::OP::LOAD_PROP_OWN_MEGAMORPHIC:
        // case r::OP::LOAD_PROP_PROTO:
        // case r::OP::LOAD_PROP_CHAIN:
        // case r::OP::STORE_PROP:
        // case r::OP::STORE_PROP_GENERIC:
        // case r::OP::DELETE_PROP:
        // case r::OP::INCREMENT_PROP:
        // case r::OP::DECREMENT_PROP:
        // case r::OP::POSTFIX_INCREMENT_PROP:
        // case r::OP::POSTFIX_DECREMENT_PROP:
        case r::OP::LOAD_CONST:
          EmitLOAD_CONST(instr);
          break;
        case r::OP::JUMP_BY:
          EmitJUMP_BY(instr);
          break;
        case r::OP::JUMP_SUBROUTINE:
          EmitJUMP_SUBROUTINE(instr);
          break;
        case r::OP::IF_FALSE:
          EmitIF_FALSE(instr);
          break;
        case r::OP::IF_TRUE:
          EmitIF_TRUE(instr);
          break;
        case r::OP::FORIN_SETUP:
          EmitFORIN_SETUP(instr);
          break;
        case r::OP::FORIN_ENUMERATE:
          EmitFORIN_ENUMERATE(instr);
          break;
        case r::OP::FORIN_LEAVE:
          EmitFORIN_LEAVE(instr);
          break;
        case r::OP::TRY_CATCH_SETUP:
          EmitTRY_CATCH_SETUP(instr);
          break;
        case r::OP::LOAD_NAME:
          EmitLOAD_NAME(instr);
          break;
        case r::OP::STORE_NAME:
          EmitSTORE_NAME(instr);
          break;
        case r::OP::DELETE_NAME:
          EmitDELETE_NAME(instr);
          break;
        case r::OP::INCREMENT_NAME:
          EmitINCREMENT_NAME(instr);
          break;
        case r::OP::DECREMENT_NAME:
          EmitDECREMENT_NAME(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_NAME:
          EmitPOSTFIX_INCREMENT_NAME(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_NAME:
          EmitPOSTFIX_DECREMENT_NAME(instr);
          break;
        case r::OP::TYPEOF_NAME:
          EmitTYPEOF_NAME(instr);
          break;
        case r::OP::LOAD_GLOBAL:
          EmitLOAD_GLOBAL(instr);
          break;
        case r::OP::STORE_GLOBAL:
          EmitSTORE_GLOBAL(instr);
          break;
        case r::OP::DELETE_GLOBAL:
          EmitDELETE_GLOBAL(instr);
          break;
        case r::OP::INCREMENT_GLOBAL:
          EmitINCREMENT_GLOBAL(instr);
          break;
        case r::OP::DECREMENT_GLOBAL:
          EmitDECREMENT_GLOBAL(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_GLOBAL:
          EmitPOSTFIX_INCREMENT_GLOBAL(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_GLOBAL:
          EmitPOSTFIX_DECREMENT_GLOBAL(instr);
          break;
        case r::OP::TYPEOF_GLOBAL:
          EmitTYPEOF_GLOBAL(instr);
          break;
        case r::OP::LOAD_HEAP:
          EmitLOAD_HEAP(instr);
          break;
        case r::OP::STORE_HEAP:
          EmitSTORE_HEAP(instr);
          break;
        case r::OP::DELETE_HEAP:
          EmitDELETE_HEAP(instr);
          break;
        case r::OP::INCREMENT_HEAP:
          EmitINCREMENT_HEAP(instr);
          break;
        case r::OP::DECREMENT_HEAP:
          EmitDECREMENT_HEAP(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_HEAP:
          EmitPOSTFIX_INCREMENT_HEAP(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_HEAP:
          EmitPOSTFIX_DECREMENT_HEAP(instr);
          break;
        case r::OP::TYPEOF_HEAP:
          EmitTYPEOF_HEAP(instr);
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
        case r::OP::PREPARE_DYNAMIC_CALL:
          EmitPREPARE_DYNAMIC_CALL(instr);
          break;
        case r::OP::CALL:
          EmitCALL(instr);
          break;
        case r::OP::CONSTRUCT:
          EmitCONSTRUCT(instr);
          break;
        case r::OP::EVAL:
          EmitEVAL(instr);
          break;
        case r::OP::RESULT:
          EmitRESULT(instr);
          break;
        case r::OP::LOAD_FUNCTION:
          EmitLOAD_FUNCTION(instr);
          break;
        case r::OP::INIT_VECTOR_ARRAY_ELEMENT:
          EmitINIT_VECTOR_ARRAY_ELEMENT(instr);
          break;
        case r::OP::INIT_SPARSE_ARRAY_ELEMENT:
          EmitINIT_SPARSE_ARRAY_ELEMENT(instr);
          break;
        case r::OP::LOAD_ARRAY:
          EmitLOAD_ARRAY(instr);
          break;
        case r::OP::BUILD_ENV:
          EmitBUILD_ENV(instr);
          break;
        case r::OP::INSTANTIATE_DECLARATION_BINDING:
          EmitINSTANTIATE_DECLARATION_BINDING(instr);
          break;
        case r::OP::INSTANTIATE_VARIABLE_BINDING:
          EmitINSTANTIATE_VARIABLE_BINDING(instr);
          break;
        case r::OP::INITIALIZE_HEAP_IMMUTABLE:
          EmitINITIALIZE_HEAP_IMMUTABLE(instr);
          break;
        case r::OP::LOAD_ARGUMENTS:
          EmitLOAD_ARGUMENTS(instr);
          break;
      }
      std::advance(instr, length);
    }
    // because handler makes range label to end.
    CheckBlock(code_->end());
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
    asm_->mov(asm_->edx, size);
    asm_->mov(asm_->ecx, mutable_start);
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

  // opcode | (jmp | flag)
  void EmitRETURN_SUBROUTINE(const Instruction* instr) {
    const int16_t jump = Reg(instr[1].i16[0]);
    const int16_t flag = Reg(instr[1].i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rax, asm_->ptr[asm_->r13 + flag * kJSValSize]);
      asm_->cmp(asm_->eax, railgun::VM::kJumpFromSubroutine);
      asm_->jne(".RETURN_TO_HANDLING");

      // encoded address value
      asm_->mov(asm_->rax, asm_->qword[asm_->r13 + jump * kJSValSize]);
      asm_->ror(asm_->rax, kEncodeRotateN);
      asm_->sub(asm_->rax, 1);
      asm_->jmp(asm_->rax);

      asm_->L(".RETURN_TO_HANDLING");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + jump * kJSValSize]);
      asm_->Call(&stub::THROW);
    }
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
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_IN(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      asm_->Call(&stub::BINARY_IN);
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_EQ(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_EQ_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_EQ_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_EQ and IF_FALSE
      asm_->sete(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_EQ_EXIT");
      asm_->L(".BINARY_EQ_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_EQ);
      asm_->L(".BINARY_EQ_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_STRICT_EQ(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_STRICT_EQ_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_STRICT_EQ_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_STRICT_EQ and IF_FALSE
      asm_->sete(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_STRICT_EQ_EXIT");
      asm_->L(".BINARY_STRICT_EQ_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_STRICT_EQ);
      asm_->L(".BINARY_STRICT_EQ_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_NE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_NE_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_NE_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_NE and IF_FALSE
      asm_->setne(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_NE_EXIT");
      asm_->L(".BINARY_NE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_NE);
      asm_->L(".BINARY_NE_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_STRICT_NE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_STRICT_NE_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_STRICT_NE_SLOW");
      asm_->cmp(asm_->esi, asm_->edx);
      // TODO(Constellation)
      // we should introduce fusion opcode, like BINARY_STRICT_NE and IF_FALSE
      asm_->setne(asm_->al);
      ConvertBooleanToJSVal(asm_->rax);
      asm_->jmp(".BINARY_STRICT_NE_EXIT");
      asm_->L(".BINARY_STRICT_NE_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_STRICT_NE);
      asm_->L(".BINARY_STRICT_NE_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_AND(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_BIT_AND_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_BIT_AND_SLOW");
      asm_->and(asm_->esi, asm_->edx);
      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->add(asm_->rax, asm_->rsi);
      asm_->jmp(".BINARY_BIT_AND_EXIT");

      asm_->L(".BINARY_BIT_AND_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_BIT_AND);

      asm_->L(".BINARY_BIT_AND_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_XOR(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_BIT_XOR_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_BIT_XOR_SLOW");
      asm_->xor(asm_->esi, asm_->edx);
      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->add(asm_->rax, asm_->rsi);
      asm_->jmp(".BINARY_BIT_XOR_EXIT");

      asm_->L(".BINARY_BIT_XOR_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_BIT_XOR);

      asm_->L(".BINARY_BIT_XOR_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_OR(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t lhs = Reg(instr[1].i16[1]);
    const int16_t rhs = Reg(instr[1].i16[2]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + lhs * kJSValSize]);
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + rhs * kJSValSize]);
      Int32Guard(asm_->rsi, asm_->rax, asm_->rcx, ".BINARY_BIT_OR_SLOW");
      Int32Guard(asm_->rdx, asm_->rax, asm_->rcx, ".BINARY_BIT_OR_SLOW");
      asm_->or(asm_->esi, asm_->edx);
      asm_->mov(asm_->rax, detail::jsval64::kNumberMask);
      asm_->add(asm_->rax, asm_->rsi);
      asm_->jmp(".BINARY_BIT_OR_EXIT");

      asm_->L(".BINARY_BIT_OR_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->Call(&stub::BINARY_BIT_OR);

      asm_->L(".BINARY_BIT_OR_EXIT");
      asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
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
  void EmitTO_NUMBER(const Instruction* instr) {
    const int16_t src = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->Call(&stub::TO_NUMBER);
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
  void EmitRAISE_REFERENCE(const Instruction* instr) {
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->Call(&stub::RAISE_REFERENCE);
  }

  // opcode name
  void EmitRAISE_IMMUTABLE(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].u32[0]];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(name));
    asm_->Call(&stub::RAISE_IMMUTABLE);
  }

  // opcode | (dst | src)
  void EmitTYPEOF(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t src = Reg(instr[1].i16[1]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->Call(&stub::TYPEOF);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_DATA(const Instruction* instr) {
    const int16_t obj = Reg(instr[1].i16[0]);
    const int16_t item = Reg(instr[1].i16[1]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t merged = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + obj * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + item * kJSValSize]);
    asm_->mov(asm_->ecx, offset);
    if (merged) {
      asm_->Call(&stub::STORE_OBJECT_DATA<true>);
    } else {
      asm_->Call(&stub::STORE_OBJECT_DATA<false>);
    }
  }

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_GET(const Instruction* instr) {
    const int16_t obj = Reg(instr[1].i16[0]);
    const int16_t item = Reg(instr[1].i16[1]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t merged = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + obj * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + item * kJSValSize]);
    asm_->mov(asm_->ecx, offset);
    if (merged) {
      asm_->Call(&stub::STORE_OBJECT_GET<true>);
    } else {
      asm_->Call(&stub::STORE_OBJECT_GET<false>);
    }
  }

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_SET(const Instruction* instr) {
    const int16_t obj = Reg(instr[1].i16[0]);
    const int16_t item = Reg(instr[1].i16[1]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t merged = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + obj * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + item * kJSValSize]);
    asm_->mov(asm_->ecx, offset);
    if (merged) {
      asm_->Call(&stub::STORE_OBJECT_SET<true>);
    } else {
      asm_->Call(&stub::STORE_OBJECT_SET<false>);
    }
  }

  // opcode
  void EmitPOP_ENV(const Instruction* instr) {
    // TODO(Constellation) inline this function
    asm_->mov(asm_->rdi, asm_->r13);
    asm_->Call(&stub::POP_ENV);
  }

  // opcode | (ary | reg) | (index | size)
  void EmitINIT_VECTOR_ARRAY_ELEMENT(const Instruction* instr) {
    const int16_t ary = Reg(instr[1].i16[0]);
    const int16_t reg = Reg(instr[1].i16[1]);
    const uint32_t index = instr[2].u32[0];
    const uint32_t size = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->qword[asm_->r13 + ary * kJSValSize]);
    asm_->lea(asm_->rsi, asm_->qword[asm_->r13 + reg * kJSValSize]);
    asm_->mov(asm_->edx, index);
    asm_->mov(asm_->ecx, size);
    asm_->Call(&stub::INIT_VECTOR_ARRAY_ELEMENT);
  }

  // opcode | (ary | reg) | (index | size)
  void EmitINIT_SPARSE_ARRAY_ELEMENT(const Instruction* instr) {
    const int16_t ary = Reg(instr[1].i16[0]);
    const int16_t reg = Reg(instr[1].i16[1]);
    const uint32_t index = instr[2].u32[0];
    const uint32_t size = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->qword[asm_->r13 + ary * kJSValSize]);
    asm_->lea(asm_->rsi, asm_->qword[asm_->r13 + reg * kJSValSize]);
    asm_->mov(asm_->edx, index);
    asm_->mov(asm_->ecx, size);
    asm_->Call(&stub::INIT_SPARSE_ARRAY_ELEMENT);
  }

  // opcode | (dst | size)
  void EmitLOAD_ARRAY(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t size = instr[1].ssw.u32;
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->esi, size);
    asm_->Call(&stub::LOAD_ARRAY);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | code)
  void EmitLOAD_FUNCTION(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    railgun::Code* target = code_->codes()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(target));
    asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->Call(&stub::LOAD_FUNCTION);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | offset)
  void EmitLOAD_REGEXP(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    JSRegExp* regexp =
        static_cast<JSRegExp*>(code_->constants()[instr[1].ssw.u32].object());
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(regexp));
    asm_->Call(&stub::LOAD_REGEXP);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | dst | map
  void EmitLOAD_OBJECT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(instr[2].map));
    asm_->Call(&stub::LOAD_OBJECT);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | base | element)
  void EmitLOAD_ELEMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t base = Reg(instr[1].i16[1]);
    const int16_t element = Reg(instr[1].i16[2]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->qword[asm_->r13 + element * kJSValSize]);
    asm_->Call(&stub::LOAD_ELEMENT);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (base | element | src)
  void EmitSTORE_ELEMENT(const Instruction* instr) {
    const int16_t base = Reg(instr[1].i16[0]);
    const int16_t element = Reg(instr[1].i16[1]);
    const int16_t src = Reg(instr[1].i16[2]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->qword[asm_->r13 + element * kJSValSize]);
    asm_->mov(asm_->rcx, asm_->qword[asm_->r13 + src * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::STORE_ELEMENT<true>);
    } else {
      asm_->Call(&stub::STORE_ELEMENT<false>);
    }
  }

  // opcode | (dst | base | element)
  void EmitDELETE_ELEMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t base = Reg(instr[1].i16[1]);
    const int16_t element = Reg(instr[1].i16[2]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->qword[asm_->r13 + element * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::DELETE_ELEMENT<true>);
    } else {
      asm_->Call(&stub::DELETE_ELEMENT<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | base | element)
  void EmitINCREMENT_ELEMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t base = Reg(instr[1].i16[1]);
    const int16_t element = Reg(instr[1].i16[2]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->qword[asm_->r13 + element * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementElement<1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementElement<1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | base | element)
  void EmitDECREMENT_ELEMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t base = Reg(instr[1].i16[1]);
    const int16_t element = Reg(instr[1].i16[2]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->qword[asm_->r13 + element * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementElement<-1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementElement<-1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | base | element)
  void EmitPOSTFIX_INCREMENT_ELEMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t base = Reg(instr[1].i16[1]);
    const int16_t element = Reg(instr[1].i16[2]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->qword[asm_->r13 + element * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementElement<1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementElement<1, 0, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | base | element)
  void EmitPOSTFIX_DECREMENT_ELEMENT(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].i16[0]);
    const int16_t base = Reg(instr[1].i16[1]);
    const int16_t element = Reg(instr[1].i16[2]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->mov(asm_->rdx, asm_->qword[asm_->r13 + element * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementElement<-1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementElement<-1, 0, false>);
    }
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

  // opcode | (dst | index) | nop | nop
  void EmitLOAD_GLOBAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
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
    const int16_t src = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
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

  // opcode | (dst | name) | nop | nop
  void EmitDELETE_GLOBAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(name));
    asm_->Call(&stub::DELETE_GLOBAL);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | nop | nop
  void EmitINCREMENT_GLOBAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(instr));
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementGlobal<1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementGlobal<1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | nop | nop
  void EmitDECREMENT_GLOBAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(instr));
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementGlobal<-1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementGlobal<-1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | nop | nop
  void EmitPOSTFIX_INCREMENT_GLOBAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(instr));
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementGlobal<1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementGlobal<1, 0, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | nop | nop
  void EmitPOSTFIX_DECREMENT_GLOBAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(instr));
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementGlobal<-1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementGlobal<-1, 0, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | nop | nop
  void EmitTYPEOF_GLOBAL(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::TYPEOF_GLOBAL<true>);
    } else {
      asm_->Call(&stub::TYPEOF_GLOBAL<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | index) | (offset | nest)
  void EmitLOAD_HEAP(const Instruction* instr) {
    // TODO(Constellation) inline this method
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->edx, offset);
    asm_->mov(asm_->ecx, nest);
    if (code_->strict()) {
      asm_->Call(&stub::LOAD_HEAP<true>);
    } else {
      asm_->Call(&stub::LOAD_HEAP<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (src | name) | (offset | nest)
  void EmitSTORE_HEAP(const Instruction* instr) {
    // TODO(Constellation) inline this method
    const int16_t src = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->edx, offset);
    asm_->mov(asm_->ecx, nest);
    asm_->mov(asm_->r8, asm_->qword[asm_->r13 + src * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::STORE_HEAP<true>);
    } else {
      asm_->Call(&stub::STORE_HEAP<false>);
    }
  }

  // opcode | (dst | name) | (offset | nest)
  void EmitDELETE_HEAP(const Instruction* instr) {
    static const uint64_t layout = Extract(JSFalse);
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], layout);
  }

  // opcode | (dst | name) | (offset | nest)
  void EmitINCREMENT_HEAP(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->edx, offset);
    asm_->mov(asm_->ecx, nest);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementHeap<1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementHeap<1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | (offset | nest)
  void EmitDECREMENT_HEAP(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->edx, offset);
    asm_->mov(asm_->ecx, nest);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementHeap<-1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementHeap<-1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | (offset | nest)
  void EmitPOSTFIX_INCREMENT_HEAP(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->edx, offset);
    asm_->mov(asm_->ecx, nest);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementHeap<1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementHeap<1, 0, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | (offset | nest)
  void EmitPOSTFIX_DECREMENT_HEAP(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->edx, offset);
    asm_->mov(asm_->ecx, nest);
    if (code_->strict()) {
      asm_->Call(&stub::IncrementHeap<-1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementHeap<-1, 0, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name) | (offset | nest)
  void EmitTYPEOF_HEAP(const Instruction* instr) {
    // TODO(Constellation) inline this method
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->edx, offset);
    asm_->mov(asm_->ecx, nest);
    if (code_->strict()) {
      asm_->Call(&stub::TYPEOF_HEAP<true>);
    } else {
      asm_->Call(&stub::TYPEOF_HEAP<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
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
      asm_->mov(asm_->ecx, argc_with_this);
      asm_->mov(asm_->r8, asm_->rsp);
      asm_->mov(asm_->qword[asm_->rsp], asm_->r13);
      asm_->Call(&stub::CALL);
      asm_->mov(asm_->rcx, asm_->qword[asm_->rsp]);
      asm_->cmp(asm_->rcx, asm_->r13);
      asm_->je(".CALL_EXIT");

      // move to new Frame
      asm_->mov(asm_->r13, asm_->rcx);
      // initialize return address of frame
      asm_->lea(asm_->rcx, asm_->ptr[asm_->rsp - k64Size * 3]);
      asm_->mov(asm_->qword[asm_->r13 + IV_OFFSETOF(railgun::Frame, return_address_position_)], asm_->rcx);
      asm_->call(asm_->rax);

      // unwind Frame
      asm_->mov(asm_->rcx, asm_->r13);  // old frame
      asm_->mov(asm_->r13, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, prev_)]);  // current frame
      const int16_t frame_end_offset = Reg(code_->registers());
      asm_->lea(asm_->rbx, asm_->ptr[asm_->r13 + frame_end_offset * kJSValSize]);
      asm_->cmp(asm_->rcx, asm_->rbx);
      asm_->jge(".CALL_UNWIND_OLD");
      asm_->mov(asm_->rcx, asm_->rbx);

      // rcx is new stack pointer
      asm_->L(".CALL_UNWIND_OLD");
      asm_->mov(asm_->rbx, asm_->ptr[asm_->r12 + IV_OFFSETOF(railgun::Context, vm_)]);
      asm_->mov(asm_->ptr[asm_->rbx + (IV_OFFSETOF(railgun::VM, stack_) + IV_OFFSETOF(railgun::Stack, stack_pointer_))], asm_->rcx);
      asm_->mov(asm_->ptr[asm_->rbx + (IV_OFFSETOF(railgun::VM, stack_) + IV_OFFSETOF(railgun::Stack, current_))], asm_->r13);

      asm_->L(".CALL_EXIT");
    }
  }

  // opcode | (callee | offset | argc_with_this)
  void EmitCONSTRUCT(const Instruction* instr) {
    const int16_t callee = Reg(instr[1].ssw.i16[0]);
    const int16_t offset = Reg(instr[1].ssw.i16[1]);
    const uint32_t argc_with_this = instr[1].ssw.u32;
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + callee * kJSValSize]);
      asm_->lea(asm_->rdx, asm_->ptr[asm_->r13 + offset * kJSValSize]);
      asm_->mov(asm_->ecx, argc_with_this);
      asm_->mov(asm_->r8, asm_->rsp);
      asm_->mov(asm_->qword[asm_->rsp], asm_->r13);
      asm_->Call(&stub::CONSTRUCT);
      asm_->mov(asm_->rcx, asm_->qword[asm_->rsp]);
      asm_->cmp(asm_->rcx, asm_->r13);
      asm_->je(".CONSTRUCT_EXIT");

      // move to new Frame
      asm_->mov(asm_->r13, asm_->rcx);
      // initialize return address of frame
      asm_->lea(asm_->rcx, asm_->ptr[asm_->rsp - k64Size * 3]);
      asm_->mov(asm_->qword[asm_->r13 + IV_OFFSETOF(railgun::Frame, return_address_position_)], asm_->rcx);
      asm_->call(asm_->rax);

      // unwind Frame
      asm_->mov(asm_->rcx, asm_->r13);  // old frame
      asm_->mov(asm_->rdx, asm_->ptr[asm_->r13 + kJSValSize * Reg(railgun::FrameConstant<>::kThisOffset)]);  // NOLINT
      asm_->mov(asm_->r13, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, prev_)]);  // current frame
      const int16_t frame_end_offset = Reg(code_->registers());
      asm_->lea(asm_->rbx, asm_->ptr[asm_->r13 + frame_end_offset * kJSValSize]);
      asm_->cmp(asm_->rcx, asm_->rbx);
      asm_->jge(".CALL_UNWIND_OLD");
      asm_->mov(asm_->rcx, asm_->rbx);

      // rcx is new stack pointer
      asm_->L(".CALL_UNWIND_OLD");
      asm_->mov(asm_->rbx, asm_->ptr[asm_->r12 + IV_OFFSETOF(railgun::Context, vm_)]);
      asm_->mov(asm_->ptr[asm_->rbx + (IV_OFFSETOF(railgun::VM, stack_) + IV_OFFSETOF(railgun::Stack, stack_pointer_))], asm_->rcx);
      asm_->mov(asm_->ptr[asm_->rbx + (IV_OFFSETOF(railgun::VM, stack_) + IV_OFFSETOF(railgun::Stack, current_))], asm_->r13);

      // after call of JS Function
      // rax is result value
      asm_->mov(asm_->rsi, detail::jsval64::kValueMask);
      asm_->and(asm_->rsi, asm_->rax);
      asm_->jnz(".RESULT_IS_NOT_OBJECT");

      // currently, rax target is garanteed as cell
      asm_->mov(asm_->rcx, asm_->ptr[asm_->rax + IV_OFFSETOF(radio::Cell, next_address_of_freelist_or_storage_)]);  // NOLINT
      asm_->shr(asm_->rcx, radio::Color::kOffset);
      asm_->cmp(asm_->rcx, radio::OBJECT);
      asm_->je(".CONSTRUCT_EXIT");

      // constructor call and return value is not object
      asm_->L(".RESULT_IS_NOT_OBJECT");
      asm_->mov(asm_->rax, asm_->rdx);

      asm_->L(".CONSTRUCT_EXIT");
    }
  }

  // opcode | (callee | offset | argc_with_this)
  void EmitEVAL(const Instruction* instr) {
    const int16_t callee = Reg(instr[1].ssw.i16[0]);
    const int16_t offset = Reg(instr[1].ssw.i16[1]);
    const uint32_t argc_with_this = instr[1].ssw.u32;
    {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + callee * kJSValSize]);
      asm_->lea(asm_->rdx, asm_->ptr[asm_->r13 + offset * kJSValSize]);
      asm_->mov(asm_->ecx, argc_with_this);
      asm_->mov(asm_->r8, asm_->rsp);
      asm_->mov(asm_->qword[asm_->rsp], asm_->r13);
      asm_->Call(&stub::EVAL);
      asm_->mov(asm_->rcx, asm_->qword[asm_->rsp]);
      asm_->cmp(asm_->rcx, asm_->r13);
      asm_->je(".CALL_EXIT");

      // move to new Frame
      asm_->mov(asm_->r13, asm_->rcx);
      // initialize return address of frame
      asm_->lea(asm_->rcx, asm_->ptr[asm_->rsp - k64Size * 3]);
      asm_->mov(asm_->qword[asm_->r13 + IV_OFFSETOF(railgun::Frame, return_address_position_)], asm_->rcx);
      asm_->call(asm_->rax);

      // unwind Frame
      asm_->mov(asm_->rcx, asm_->r13);  // old frame
      asm_->mov(asm_->r13, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, prev_)]);  // current frame
      const int16_t frame_end_offset = Reg(code_->registers());
      asm_->lea(asm_->rbx, asm_->ptr[asm_->r13 + frame_end_offset * kJSValSize]);
      asm_->cmp(asm_->rcx, asm_->rbx);
      asm_->jge(".CALL_UNWIND_OLD");
      asm_->mov(asm_->rcx, asm_->rbx);

      // rcx is new stack pointer
      asm_->L(".CALL_UNWIND_OLD");
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

  // opcode | (src | offset)
  void EmitINITIALIZE_HEAP_IMMUTABLE(const Instruction* instr) {
    const int16_t src = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[1].ssw.u32;
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, variable_env_)]);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + src * kJSValSize]);
    asm_->mov(asm_->edx, offset);
    asm_->Call(&stub::INITIALIZE_HEAP_IMMUTABLE);
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
      asm_->mov(asm_->rax, overflow);
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], asm_->rax);
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
      asm_->mov(asm_->rax, overflow);
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], asm_->rax);
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
      asm_->mov(asm_->rax, overflow);
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], asm_->rax);
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
      asm_->mov(asm_->rax, overflow);
      asm_->mov(asm_->qword[asm_->r13 + src * kJSValSize], asm_->rax);
      asm_->jmp(".DECREMENT_EXIT");

      asm_->L(".DECREMENT_SLOW");
      asm_->mov(asm_->rdi, asm_->r12);
      asm_->lea(asm_->rdx, asm_->ptr[asm_->r13 + dst * kJSValSize]);
      asm_->Call(&stub::POSTFIX_DECREMENT);
      asm_->mov(asm_->ptr[asm_->r13 + src * kJSValSize], asm_->rax);

      asm_->L(".DECREMENT_EXIT");
    }
  }

  // opcode | (dst | basedst | name)
  void EmitPREPARE_DYNAMIC_CALL(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    const int16_t base = Reg(instr[1].ssw.i16[1]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    asm_->lea(asm_->rcx, asm_->qword[asm_->r13 + base * kJSValSize]);
    asm_->Call(&stub::PREPARE_DYNAMIC_CALL);
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (jmp | cond)
  void EmitIF_TRUE(const Instruction* instr) {
    // TODO(Constelation) inlining this
    const int16_t cond = Reg(instr[1].jump.i16[0]);
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second.counter;
    const std::string label = MakeLabel(num);
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + cond * kJSValSize]);
    asm_->Call(&stub::TO_BOOLEAN);
    asm_->test(asm_->eax, asm_->eax);
    asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
  }

  // opcode | (jmp : addr | flag)
  void EmitJUMP_SUBROUTINE(const Instruction* instr) {
    static const uint64_t layout = Extract(JSVal::Int32(railgun::VM::kJumpFromSubroutine));
    const int16_t addr = Reg(instr[1].jump.i16[0]);
    const int16_t flag = Reg(instr[1].jump.i16[1]);
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second.counter;
    const std::string label = MakeLabel(num);

    // register position and repatch afterward
    Assembler::RepatchSite site;
    site.Mov(asm_, asm_->rax);
    // Value is JSVal, but, this indicates pointer to address
    asm_->mov(asm_->qword[asm_->r13 + addr * kJSValSize], asm_->rax);
    asm_->mov(asm_->rax, layout);
    asm_->mov(asm_->qword[asm_->r13 + flag * kJSValSize], asm_->rax);
    asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);

    asm_->align(2);
    // Now, LSB of ptr to this place is 0
    // So we rotate this bit, make dummy double value
    // and store to virtual register
    unresolved_address_map_.insert(std::make_pair(asm_->size(), site));
  }

  // opcode | (jmp | cond)
  void EmitIF_FALSE(const Instruction* instr) {
    // TODO(Constelation) inlining this
    const int16_t cond = Reg(instr[1].jump.i16[0]);
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second.counter;
    const std::string label = MakeLabel(num);
    asm_->mov(asm_->rdi, asm_->ptr[asm_->r13 + cond * kJSValSize]);
    asm_->Call(&stub::TO_BOOLEAN);
    asm_->test(asm_->eax, asm_->eax);
    asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
  }

  // opcode | (jmp : iterator | enumerable)
  void EmitFORIN_SETUP(const Instruction* instr) {
    const int16_t iterator = Reg(instr[1].jump.i16[0]);
    const int16_t enumerable = Reg(instr[1].jump.i16[1]);
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second.counter;
    const std::string label = MakeLabel(num);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + enumerable * kJSValSize]);
    NullOrUndefinedGuard(asm_->rsi, asm_->rdi, label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->Call(&stub::FORIN_SETUP);
    asm_->mov(asm_->ptr[asm_->r13 + iterator * kJSValSize], asm_->rax);
  }

  // opcode | (jmp : dst | iterator)
  void EmitFORIN_ENUMERATE(const Instruction* instr) {
    const int16_t dst = Reg(instr[1].jump.i16[0]);
    const int16_t iterator = Reg(instr[1].jump.i16[1]);
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second.counter;
    const std::string label = MakeLabel(num);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + iterator * kJSValSize]);
    asm_->Call(&stub::FORIN_ENUMERATE);
    asm_->cmp(asm_->rax, 0);
    asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | iterator
  void EmitFORIN_LEAVE(const Instruction* instr) {
    const int16_t iterator = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + iterator * kJSValSize]);
    asm_->Call(&stub::FORIN_LEAVE);
  }

  // opcode | (error | name)
  void EmitTRY_CATCH_SETUP(const Instruction* instr) {
    const int16_t error = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    asm_->mov(asm_->rcx, asm_->ptr[asm_->r13 + error * kJSValSize]);
    asm_->Call(&stub::TRY_CATCH_SETUP);
    asm_->mov(asm_->qword[asm_->r13 + offsetof(railgun::Frame, lexical_env_)], asm_->rax);
  }

  // opcode | (dst | index)
  void EmitLOAD_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::LOAD_NAME<true>);
    } else {
      asm_->Call(&stub::LOAD_NAME<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (src | name)
  void EmitSTORE_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t src = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    asm_->mov(asm_->rcx, asm_->ptr[asm_->r13 + src * kJSValSize]);
    if (code_->strict()) {
      asm_->Call(&stub::STORE_NAME<true>);
    } else {
      asm_->Call(&stub::STORE_NAME<false>);
    }
  }

  // opcode | (dst | name)
  void EmitDELETE_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::DELETE_NAME<true>);
    } else {
      asm_->Call(&stub::DELETE_NAME<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name)
  void EmitINCREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementName<1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementName<1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name)
  void EmitDECREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementName<-1, 1, true>);
    } else {
      asm_->Call(&stub::IncrementName<-1, 1, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name)
  void EmitPOSTFIX_INCREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementName<1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementName<1, 0, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name)
  void EmitPOSTFIX_DECREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::IncrementName<-1, 0, true>);
    } else {
      asm_->Call(&stub::IncrementName<-1, 0, false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | (dst | name)
  void EmitTYPEOF_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const int16_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->ptr[asm_->r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(asm_->rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::TYPEOF_NAME<true>);
    } else {
      asm_->Call(&stub::TYPEOF_NAME<false>);
    }
    asm_->mov(asm_->qword[asm_->r13 + dst * kJSValSize], asm_->rax);
  }

  // opcode | jmp
  void EmitJUMP_BY(const Instruction* instr) {
    const int32_t jump = instr[1].jump.to;
    const uint32_t to = (instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second.counter;
    const std::string label = MakeLabel(num);
    asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
  }

  // opcode | dst
  void EmitLOAD_ARGUMENTS(const Instruction* instr) {
    const int32_t dst = Reg(instr[1].i32[0]);
    asm_->mov(asm_->rdi, asm_->r12);
    asm_->mov(asm_->rsi, asm_->r13);
    if (code_->strict()) {
      asm_->Call(&stub::LOAD_ARGUMENTS<true>);
    } else {
      asm_->Call(&stub::LOAD_ARGUMENTS<false>);
    }
    asm_->mov(asm_->ptr[asm_->r13 + dst * kJSValSize], asm_->rax);
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
                  const Xbyak::Reg64& tmp2,
                  const char* label,
                  Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    asm_->mov(tmp1, detail::jsval64::kNumberMask);
    asm_->and(tmp1, target);
    asm_->mov(tmp2, detail::jsval64::kNumberMask);
    asm_->cmp(tmp1, tmp2);
    asm_->jne(label, type);
  }

  void NullOrUndefinedGuard(const Xbyak::Reg64& target,
                            const Xbyak::Reg64& tmp, const char* label,
                            Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    // (1000)2 = 8
    // Null is (0010)2 and Undefined is (1010)2
    asm_->mov(tmp, ~UINT64_C(8));
    asm_->and(tmp, target);
    asm_->cmp(tmp, detail::jsval64::kNull);
    asm_->je(label, type);
  }

  void AddingInt32OverflowGuard(const Xbyak::Reg32& lhs,
                                const Xbyak::Reg32& rhs,
                                const Xbyak::Reg32& out,
                                const char* label,
                                Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    asm_->mov(out, lhs);
    asm_->add(out, rhs);
    asm_->jo(label, type);
  }

  void SubtractingInt32OverflowGuard(const Xbyak::Reg32& lhs,
                                     const Xbyak::Reg32& rhs,
                                     const Xbyak::Reg32& out,
                                     const char* label,
                                     Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    asm_->mov(out, lhs);
    asm_->sub(out, rhs);
    asm_->jo(label, type);
  }

  void MultiplyingInt32OverflowGuard(const Xbyak::Reg32& lhs,
                                     const Xbyak::Reg32& rhs,
                                     const Xbyak::Reg32& out,
                                     const char* label,
                                     Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    asm_->mov(out, lhs);
    asm_->imul(out, rhs);
    asm_->jo(label, type);
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
  UnresolvedAddressMap unresolved_address_map_;
  HandlerLinks handler_links_;
  Codes codes_;
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
