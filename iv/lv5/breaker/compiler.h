// breaker::Compiler
//
// This compiler parses railgun::opcodes and emits native code.
// Some primitive operations and branch operations are
// emitted as raw native code,
// and basic complex opcodes are emitted as call to stub function, that is,
// Context Threading JIT.
//
// Stub function implementations are in stub.h
#ifndef IV_LV5_BREAKER_COMPILER_H_
#define IV_LV5_BREAKER_COMPILER_H_
#include <iv/debug.h>
#include <iv/byteorder.h>
#include <iv/lv5/jsglobal.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/lv5/breaker/helper.h>
#include <iv/lv5/railgun/railgun.h>
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/jsfunction.h>
#include <iv/lv5/breaker/type.h>
#include <iv/lv5/breaker/mono_ic.h>
#include <iv/lv5/breaker/poly_ic.h>
#include <iv/lv5/breaker/stub.h>
namespace iv {
namespace lv5 {
namespace breaker {

static const std::size_t kEncodeRotateN =
    core::math::CTZ64(lv5::detail::jsval64::kDoubleOffset);

class Compiler {
 public:
  // introducing railgun to this scope
  typedef railgun::Instruction Instruction;
  typedef railgun::OP OP;

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
  typedef std::unordered_map<std::size_t,
                             Assembler::RepatchSite> UnresolvedAddressMap;
  typedef std::vector<
      std::pair<Assembler::RepatchSite, std::size_t> > RepatchSites;
  typedef std::vector<railgun::Code*> Codes;
  typedef std::unordered_map<const Instruction*, std::size_t> HandlerLinks;

  static const int kJSValSize = sizeof(JSVal);

  static const int32_t kInvalidUsedOffset = INT32_MIN;

  explicit Compiler(Context* ctx, railgun::Code* top)
    : ctx_(ctx),
      top_(top),
      code_(nullptr),
      asm_(new Assembler),
      native_code_(new NativeCode(asm_)),
      jump_map_(),
      entry_points_(),
      unresolved_address_map_(),
      handler_links_(),
      codes_(),
      counter_(0),
      previous_instr_(nullptr),
      last_used_(kInvalidUsedOffset),
      last_used_candidate_(),
      type_record_() {
    top_->core_data()->set_native_code(native_code_);
  }

  static uint64_t RotateLeft64(uint64_t val, uint64_t amount) {
    return (val << amount) | (val >> (64 - amount));
  }

  ~Compiler() {
    // Compilation is finished.
    // Link jumps / calls and set each entry point to Code.
    asm_->ready();

    // link entry points
    for (const auto& pair : entry_points_) {
      pair.first->set_executable(asm_->GainExecutableByOffset(pair.second));
    }

    // Repatch phase
    // link jump subroutine
    for (const auto& pair : unresolved_address_map_) {
      uint64_t ptr =
          core::BitCast<uint64_t>(asm_->GainExecutableByOffset(pair.first));
      assert(ptr % 2 == 0);
      // encode ptr value to JSVal invalid number
      // double offset value is
      // 1000000000000000000000000000000000000000000000000
      ptr += 0x1;
      const uint64_t result = RotateLeft64(ptr, kEncodeRotateN);
      pair.second.Repatch(asm_, result);
    }

    // link exception handlers
    for (railgun::Code* code : codes_) {
      const Instruction* first_instr = code->begin();
      railgun::ExceptionTable& table = code->exception_table();
      for (railgun::Handler& handler : table) {
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

  void Initialize(railgun::Code* code) {
    code_ = code;
    codes_.push_back(code);
    jump_map_.clear();
    entry_points_.insert(std::make_pair(code, asm_->size()));
    previous_instr_ = nullptr;
    kill_last_used();
    type_record_.Init(code);
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
      if (r::OP::IsJump(opcode)) {
        const int32_t jump = instr[1].jump.to;
        const uint32_t to = index + jump;
        jump_map_.insert(std::make_pair(to, JumpInfo(counter_++)));
      }
      std::advance(instr, length);
    }

    // and handlers table
    const r::ExceptionTable& table = code_->exception_table();
    for (const railgun::Handler& handler : table) {
      // should override (because handled option is true)
      jump_map_[handler.begin()] = JumpInfo(counter_++, true);
      jump_map_[handler.end()] = JumpInfo(counter_++, true);
    }
  }

  // Returns previous and instr are in basic block
  bool SplitBasicBlock(const Instruction* previous, const Instruction* instr) {
    const int32_t index = instr - code_->begin();
    const JumpMap::iterator it = jump_map_.find(index);
    if (it != jump_map_.end()) {
      // this opcode is jump target
      // split basic block
      asm_->L(MakeLabel(it->second.counter).c_str());
      if (it->second.exception_handled) {
        // store handler range
        handler_links_.insert(std::make_pair(instr, asm_->size()));
      }
      return false;
    }
    return true;
  }

  void Main() {
    namespace r = railgun;

    // generate local label scope
    const Assembler::LocalLabelScope local_label_scope(asm_);

    // emit prologue

    // general storage space
    // We can access this space by qword[rsp + k64Size * 0] a.k.a. qword[rsp]
    asm_->push(r12);
    asm_->sub(qword[r14 + offsetof(Frame, ret)], k64Size * kStackPayload);

    const Instruction* total_first_instr = code_->core_data()->data()->data();
    const Instruction* previous = nullptr;
    const Instruction* instr = code_->begin();
    for (const Instruction* last = code_->end(); instr != last;) {
      const uint32_t opcode = instr->GetOP();
      const uint32_t length = r::kOPLength[opcode];
      set_last_used_candidate(kInvalidUsedOffset);

      const bool in_basic_block = SplitBasicBlock(previous, instr);
      if (!in_basic_block) {
        set_previous_instr(nullptr);
        kill_last_used();
        type_record_.Clear();
      } else {
        set_previous_instr(previous);
      }

      native_code()->AttachBytecodeOffset(asm_->size(), instr - total_first_instr);

      switch (opcode) {
        case r::OP::NOP:
          EmitNOP(instr);
          break;
        case r::OP::ENTER:
          EmitENTER(instr);
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
        case r::OP::RAISE:
          EmitRAISE(instr);
          break;
        case r::OP::TYPEOF:
          EmitTYPEOF(instr);
          break;
        case r::OP::STORE_OBJECT_INDEXED:
          EmitSTORE_OBJECT_INDEXED(instr);
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
        case r::OP::LOAD_PROP:
          EmitLOAD_PROP(instr);
          break;
        case r::OP::LOAD_PROP_GENERIC:
        case r::OP::LOAD_PROP_OWN:
        case r::OP::LOAD_PROP_PROTO:
        case r::OP::LOAD_PROP_CHAIN:
          UNREACHABLE();
          break;
        case r::OP::STORE_PROP:
          EmitSTORE_PROP(instr);
          break;
        case r::OP::STORE_PROP_GENERIC:
          UNREACHABLE();
          break;
        case r::OP::DELETE_PROP:
          EmitDELETE_PROP(instr);
          break;
        case r::OP::INCREMENT_PROP:
          EmitINCREMENT_PROP(instr);
          break;
        case r::OP::DECREMENT_PROP:
          EmitDECREMENT_PROP(instr);
          break;
        case r::OP::POSTFIX_INCREMENT_PROP:
          EmitPOSTFIX_INCREMENT_PROP(instr);
          break;
        case r::OP::POSTFIX_DECREMENT_PROP:
          EmitPOSTFIX_DECREMENT_PROP(instr);
          break;
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
        case r::OP::IF_FALSE_BINARY_LT:
          EmitBINARY_LT(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_LT:
          EmitBINARY_LT(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_LTE:
          EmitBINARY_LTE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_LTE:
          EmitBINARY_LTE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_GT:
          EmitBINARY_GT(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_GT:
          EmitBINARY_GT(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_GTE:
          EmitBINARY_GTE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_GTE:
          EmitBINARY_GTE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_INSTANCEOF:
          EmitBINARY_INSTANCEOF(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_INSTANCEOF:
          EmitBINARY_INSTANCEOF(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_IN:
          EmitBINARY_IN(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_IN:
          EmitBINARY_IN(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_EQ:
          EmitBINARY_EQ(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_EQ:
          EmitBINARY_EQ(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_NE:
          EmitBINARY_NE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_NE:
          EmitBINARY_NE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_STRICT_EQ:
          EmitBINARY_STRICT_EQ(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_STRICT_EQ:
          EmitBINARY_STRICT_EQ(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_STRICT_NE:
          EmitBINARY_STRICT_NE(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_STRICT_NE:
          EmitBINARY_STRICT_NE(instr, OP::IF_TRUE);
          break;
        case r::OP::IF_FALSE_BINARY_BIT_AND:
          EmitBINARY_BIT_AND(instr, OP::IF_FALSE);
          break;
        case r::OP::IF_TRUE_BINARY_BIT_AND:
          EmitBINARY_BIT_AND(instr, OP::IF_TRUE);
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
        case r::OP::LOAD_GLOBAL_DIRECT:
          EmitLOAD_GLOBAL_DIRECT(instr);
          break;
        case r::OP::STORE_GLOBAL:
          EmitSTORE_GLOBAL(instr);
          break;
        case r::OP::STORE_GLOBAL_DIRECT:
          EmitSTORE_GLOBAL_DIRECT(instr);
          break;
        case r::OP::DELETE_GLOBAL:
          EmitDELETE_GLOBAL(instr);
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
        case r::OP::DUP_ARRAY:
          EmitDUP_ARRAY(instr);
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
      previous = instr;
      std::advance(instr, length);
      set_last_used(last_used_candidate());
    }
    // because handler makes range label to end.
    SplitBasicBlock(instr, code_->end());
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
  //   r14 : breaker frame
  //   r15 : number mask immediate

  static register_t Reg(register_t reg) {
    return railgun::FrameConstant<>::kFrameSize + reg;
  }

  static bool IsConstantID(register_t reg) {
    return reg >= Reg(railgun::FrameConstant<>::kConstantOffset);
  }

  static uint32_t ExtractConstantOffset(register_t reg) {
    assert(IsConstantID(reg));
    return reg -
        railgun::FrameConstant<>::kFrameSize -
        railgun::FrameConstant<>::kConstantOffset;
  }

  // Load virtual register
  void LoadVR(const Xbyak::Reg64& out, register_t offset) {
    const bool break_result = out.getIdx() == rax.getIdx();

    if (IsConstantID(offset)) {
      // This is constant register.
      const JSVal val = code_->constants()[ExtractConstantOffset(offset)];
      asm_->mov(out, Extract(val));
      if (break_result) {
        kill_last_used();
      }
      return;
    }

    if (last_used() == offset) {
      if (!break_result) {
        asm_->mov(out, rax);
      }
    } else {
      const TypeEntry entry = type_record_.Get(offset);
      if (entry.IsConstant()) {
        // constant propagation in basic block level
        asm_->mov(out, Extract(entry.constant()));
        if (break_result) {
          kill_last_used();
        }
        return;
      }
      asm_->mov(out, ptr[r13 + offset * kJSValSize]);
      if (break_result) {
        kill_last_used();
      }
    }
  }

  void LoadVRs(const Xbyak::Reg64& out1, register_t offset1,
               const Xbyak::Reg64& out2, register_t offset2) {
    if (last_used() == offset1) {
      LoadVR(out1, offset1);
      LoadVR(out2, offset2);
    } else {
      LoadVR(out2, offset2);
      LoadVR(out1, offset1);
    }
  }

  // opcode
  void EmitNOP(const Instruction* instr) {
    // save previous register because NOP does nothing
    set_last_used_candidate(last_used());
  }

  // opcode
  void EmitENTER(const Instruction* instr) {
    // check arity and stack overflow


    // initialize registers with JSUndefined
    uint32_t i = 0;
    const uint32_t iz = code_->registers();
    const uint64_t undefined = Extract(JSUndefined);
    for (; i < iz; ++i) {
      const register_t dst = Reg(i);
      asm_->mov(qword[r13 + dst * kJSValSize], undefined);
    }

    // initialize this value
    if (code_->IsThisMaterialized() && !code_->strict()) {
      const Assembler::LocalLabelScope scope(asm_);
      asm_->mov(rsi, ptr[r13 + kJSValSize * Reg(railgun::FrameConstant<>::kThisOffset)]);  // NOLINT

      asm_->mov(rdi, detail::jsval64::kValueMask);
      asm_->test(rdi, rsi);
      asm_->jnz(".not_cell", Xbyak::CodeGenerator::T_NEAR);

      // object or string
      const std::ptrdiff_t offset =
          IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ClassOffset();
      asm_->mov(rdi, qword[rsi + offset]);
      asm_->test(rdi, rdi);  // class is nullptr => String...
      asm_->jnz(".end", Xbyak::CodeGenerator::T_NEAR);  // object
      // string fall-through

      // object materialization
      asm_->L(".object_initialization"); {
        asm_->mov(rdi, r14);
        asm_->Call(&stub::TO_OBJECT);
        asm_->mov(qword[r13 + kJSValSize * Reg(railgun::FrameConstant<>::kThisOffset)], rax);  // NOLINT
        asm_->jmp(".end", Xbyak::CodeGenerator::T_NEAR);
      }

      asm_->L(".not_cell"); {
        asm_->mov(rdi, rsi);
        asm_->and(rdi, -9);
        asm_->cmp(rdi, detail::jsval64::kNull);
        asm_->jne(".object_initialization");
        // null or undefined
        asm_->mov(rax, core::BitCast<uintptr_t>(ctx_->global_obj()));
        asm_->mov(qword[r13 + kJSValSize * Reg(railgun::FrameConstant<>::kThisOffset)], rax);  // NOLINT
      }
      asm_->L(".end");
    }
  }

  // opcode | (dst | src)
  void EmitMV(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    LoadVR(rax, src);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);

    // type propagation
    type_record_.Put(dst, type_record_.Get(src));
  }

  // opcode | (size | mutable_start)
  void EmitBUILD_ENV(const Instruction* instr) {
    const uint32_t size = instr[1].u32[0];
    const uint32_t mutable_start = instr[1].u32[1];
    asm_->mov(rdi, r12);
    asm_->mov(rsi, r13);
    asm_->mov(edx, size);
    asm_->mov(ecx, mutable_start);
    asm_->Call(&stub::BUILD_ENV);
  }

  // opcode | src
  void EmitWITH_SETUP(const Instruction* instr) {
    const register_t src = Reg(instr[1].i32[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, r13);
    LoadVR(rdx, src);
    asm_->Call(&stub::WITH_SETUP);
  }

  // opcode | (jmp | flag)
  void EmitRETURN_SUBROUTINE(const Instruction* instr) {
    const register_t jump = Reg(instr[1].i16[0]);
    const register_t flag = Reg(instr[1].i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rcx, flag);
      asm_->cmp(ecx, railgun::VM::kJumpFromSubroutine);
      asm_->jne(".RETURN_TO_HANDLING");

      // encoded address value
      LoadVR(rcx, jump);
      asm_->ror(rcx, kEncodeRotateN);
      asm_->sub(rcx, 1);
      asm_->jmp(rcx);

      asm_->L(".RETURN_TO_HANDLING");
      asm_->mov(rdi, r14);
      LoadVR(rsi, jump);
      asm_->Call(&stub::THROW);
    }
  }

  // opcode | (dst | offset)
  void EmitLOAD_CONST(const Instruction* instr) {
    // extract constant bytes
    // append immediate value to machine code
    //
    // This method is used when constant register is overflow
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[1].ssw.u32;
    const JSVal val = code_->constants()[offset];
    asm_->mov(rax, Extract(val));
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);

    type_record_.Put(dst, TypeEntry(val));
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_ADD(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_SUBTRACT(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_MULTIPLY(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_DIVIDE(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_MODULO(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_LSHIFT(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_RSHIFT(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_RSHIFT_LOGICAL(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_LT(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_LTE(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_GT(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_GTE(const Instruction* instr, OP::Type fused = OP::NOP);

  template<typename Traits>
  void EmitCompare(const Instruction* instr, OP::Type fused);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_INSTANCEOF(const Instruction* instr, OP::Type fused = OP::NOP) {
    const register_t lhs = Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
    const register_t rhs = Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVRs(rsi, lhs, rdx, rhs);
      asm_->mov(rdi, r14);
      asm_->Call(&stub::BINARY_INSTANCEOF);
      if (fused != OP::NOP) {
        // fused jump opcode
        const std::string label = MakeLabel(instr);
        asm_->cmp(rax, Extract(JSTrue));
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        return;
      }

      const register_t dst = Reg(instr[1].i16[0]);
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::Instanceof(type_record_.Get(lhs), type_record_.Get(rhs)));
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_IN(const Instruction* instr, OP::Type fused = OP::NOP) {
    const register_t lhs = Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
    const register_t rhs = Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVRs(rsi, lhs, rdx, rhs);
      asm_->mov(rdi, r14);
      asm_->Call(&stub::BINARY_IN);
      if (fused != OP::NOP) {
        // fused jump opcode
        const std::string label = MakeLabel(instr);
        asm_->cmp(rax, Extract(JSTrue));
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        return;
      }

      const register_t dst = Reg(instr[1].i16[0]);
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::In(type_record_.Get(lhs), type_record_.Get(rhs)));
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_EQ(const Instruction* instr, OP::Type fused = OP::NOP) {
    const register_t lhs = Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
    const register_t rhs = Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVRs(rsi, lhs, rdx, rhs);
      Int32Guard(lhs, rsi, ".BINARY_EQ_SLOW");
      Int32Guard(rhs, rdx, ".BINARY_EQ_SLOW");
      asm_->cmp(esi, edx);

      if (fused != OP::NOP) {
        // fused jump opcode
        const std::string label = MakeLabel(instr);
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->jmp(".BINARY_EQ_EXIT");

        asm_->L(".BINARY_EQ_SLOW");
        asm_->mov(rdi, r14);
        asm_->Call(&stub::BINARY_EQ);
        asm_->cmp(rax, Extract(JSTrue));
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->L(".BINARY_EQ_EXIT");
        return;
      }

      const register_t dst = Reg(instr[1].i16[0]);
      asm_->sete(cl);
      ConvertBooleanToJSVal(cl, rax);
      asm_->jmp(".BINARY_EQ_EXIT");

      asm_->L(".BINARY_EQ_SLOW");
      asm_->mov(rdi, r14);
      asm_->Call(&stub::BINARY_EQ);

      asm_->L(".BINARY_EQ_EXIT");
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::Equal(type_record_.Get(lhs), type_record_.Get(rhs)));
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_STRICT_EQ(const Instruction* instr, OP::Type fused = OP::NOP) {
    // CAUTION:(Constellation)
    // Because stub::BINARY_STRICT_EQ is not require Frame as first argument,
    // so register layout is different from BINARY_ other ops.
    // BINARY_STRICT_NE too.
    const register_t lhs = Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
    const register_t rhs = Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVRs(rdi, lhs, rsi, rhs);
      Int32Guard(lhs, rdi, ".BINARY_STRICT_EQ_SLOW");
      Int32Guard(rhs, rsi, ".BINARY_STRICT_EQ_SLOW");
      asm_->cmp(esi, edi);

      if (fused != OP::NOP) {
        // fused jump opcode
        const std::string label = MakeLabel(instr);
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->jmp(".BINARY_STRICT_EQ_EXIT");

        asm_->L(".BINARY_STRICT_EQ_SLOW");
        asm_->Call(&stub::BINARY_STRICT_EQ);
        asm_->cmp(rax, Extract(JSTrue));
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->L(".BINARY_STRICT_EQ_EXIT");
        return;
      }

      const register_t dst = Reg(instr[1].i16[0]);
      asm_->sete(cl);
      ConvertBooleanToJSVal(cl, rax);
      asm_->jmp(".BINARY_STRICT_EQ_EXIT");

      asm_->L(".BINARY_STRICT_EQ_SLOW");
      asm_->Call(&stub::BINARY_STRICT_EQ);

      asm_->L(".BINARY_STRICT_EQ_EXIT");
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::StrictEqual(type_record_.Get(lhs), type_record_.Get(rhs)));
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_NE(const Instruction* instr, OP::Type fused = OP::NOP) {
    const register_t lhs = Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
    const register_t rhs = Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVRs(rsi, lhs, rdx, rhs);
      Int32Guard(lhs, rsi, ".BINARY_NE_SLOW");
      Int32Guard(rhs, rdx, ".BINARY_NE_SLOW");
      asm_->cmp(esi, edx);

      if (fused != OP::NOP) {
        // fused jump opcode
        const std::string label = MakeLabel(instr);
        if (fused == OP::IF_TRUE) {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->jmp(".BINARY_NE_EXIT");

        asm_->L(".BINARY_NE_SLOW");
        asm_->mov(rdi, r14);
        asm_->Call(&stub::BINARY_NE);
        asm_->cmp(rax, Extract(JSTrue));
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->L(".BINARY_NE_EXIT");
        return;
      }

      const register_t dst = Reg(instr[1].i16[0]);
      asm_->setne(cl);
      ConvertBooleanToJSVal(cl, rax);
      asm_->jmp(".BINARY_NE_EXIT");

      asm_->L(".BINARY_NE_SLOW");
      asm_->mov(rdi, r14);
      asm_->Call(&stub::BINARY_NE);

      asm_->L(".BINARY_NE_EXIT");
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::NotEqual(type_record_.Get(lhs), type_record_.Get(rhs)));
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_STRICT_NE(const Instruction* instr, OP::Type fused = OP::NOP) {
    // CAUTION:(Constellation)
    // Because stub::BINARY_STRICT_EQ is not require Frame as first argument,
    // so register layout is different from BINARY_ other ops.
    // BINARY_STRICT_NE too.
    const register_t lhs = Reg((fused == OP::NOP) ? instr[1].i16[1] : instr[1].jump.i16[0]);
    const register_t rhs = Reg((fused == OP::NOP) ? instr[1].i16[2] : instr[1].jump.i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVRs(rdi, lhs, rsi, rhs);
      Int32Guard(lhs, rdi, ".BINARY_STRICT_NE_SLOW");
      Int32Guard(rhs, rsi, ".BINARY_STRICT_NE_SLOW");
      asm_->cmp(esi, edi);

      if (fused != OP::NOP) {
        // fused jump opcode
        const std::string label = MakeLabel(instr);
        if (fused == OP::IF_TRUE) {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->jmp(".BINARY_STRICT_NE_EXIT");

        asm_->L(".BINARY_STRICT_NE_SLOW");
        asm_->Call(&stub::BINARY_STRICT_NE);
        asm_->cmp(rax, Extract(JSTrue));
        if (fused == OP::IF_TRUE) {
          asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        } else {
          asm_->jne(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
        }
        asm_->L(".BINARY_STRICT_NE_EXIT");
        return;
      }

      const register_t dst = Reg(instr[1].i16[0]);
      asm_->setne(cl);
      ConvertBooleanToJSVal(cl, rax);
      asm_->jmp(".BINARY_STRICT_NE_EXIT");

      asm_->L(".BINARY_STRICT_NE_SLOW");
      asm_->Call(&stub::BINARY_STRICT_NE);

      asm_->L(".BINARY_STRICT_NE_EXIT");
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::StrictNotEqual(type_record_.Get(lhs), type_record_.Get(rhs)));
    }
  }

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_AND(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_XOR(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_OR(const Instruction* instr);

  // opcode | (dst | src)
  void EmitUNARY_POSITIVE(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rax, src);
      asm_->mov(rsi, rax);
      asm_->and(rsi, r15);
      asm_->jnz(".UNARY_POSITIVE_EXIT");

      asm_->mov(rdi, r14);
      asm_->mov(rsi, rax);
      asm_->Call(&stub::TO_NUMBER);

      asm_->L(".UNARY_POSITIVE_EXIT");
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::Positive(type_record_.Get(src)));
    }
  }

  // opcode | (dst | src)
  void EmitUNARY_NEGATIVE(const Instruction* instr) {
    static const uint64_t layout = Extract(JSVal(-0.0));
    static const uint64_t layout2 = Extract(JSVal(-static_cast<double>(INT32_MIN)));
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    {
      // Because ECMA262 number value is defined as double,
      // -0 should be double -0.0.
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rax, src);
      Int32Guard(src, rax, ".UNARY_NEGATIVE_SLOW");
      asm_->test(eax, eax);
      asm_->jz(".UNARY_NEGATIVE_MINUS_ZERO");
      asm_->neg(eax);
      asm_->jo(".UNARY_NEGATIVE_INT32_MIN");
      asm_->or(rax, r15);
      asm_->jmp(".UNARY_NEGATIVE_EXIT");

      asm_->L(".UNARY_NEGATIVE_MINUS_ZERO");
      asm_->mov(rax, layout);
      asm_->jmp(".UNARY_NEGATIVE_EXIT");

      asm_->L(".UNARY_NEGATIVE_INT32_MIN");
      asm_->mov(rax, layout2);
      asm_->jmp(".UNARY_NEGATIVE_EXIT");

      asm_->L(".UNARY_NEGATIVE_SLOW");
      asm_->mov(rdi, r14);
      asm_->mov(rsi, rax);
      asm_->Call(&stub::UNARY_NEGATIVE);

      asm_->L(".UNARY_NEGATIVE_EXIT");
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::Negative(type_record_.Get(src)));
    }
  }

  // opcode | (dst | src)
  void EmitUNARY_NOT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    LoadVR(rdi, src);
    asm_->Call(&stub::UNARY_NOT);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry::Not(type_record_.Get(src)));
  }

  // opcode | (dst | src)
  void EmitUNARY_BIT_NOT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rsi, src);
      Int32Guard(src, rsi, ".UNARY_BIT_NOT_SLOW");
      asm_->not(esi);
      asm_->mov(eax, esi);
      asm_->or(rax, r15);
      asm_->jmp(".UNARY_BIT_NOT_EXIT");

      asm_->L(".UNARY_BIT_NOT_SLOW");
      asm_->mov(rdi, r14);
      asm_->Call(&stub::UNARY_BIT_NOT);

      asm_->L(".UNARY_BIT_NOT_EXIT");
      asm_->mov(qword[r13 + dst * kJSValSize], rax);
      set_last_used_candidate(dst);
      type_record_.Put(dst, TypeEntry::BitwiseNot(type_record_.Get(src)));
    }
  }

  // opcode | src
  void EmitTHROW(const Instruction* instr) {
    const register_t src = Reg(instr[1].i32[0]);
    LoadVR(rsi, src);
    asm_->mov(rdi, r14);
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
    const register_t src = Reg(instr[1].i32[0]);
    const TypeEntry src_type_entry = type_record_.Get(src);

    if (src_type_entry.type().IsNumber()) {
      // no effect
      return;
    }

    LoadVR(rsi, src);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::TO_NUMBER);
  }

  // opcode | src
  void EmitTO_PRIMITIVE_AND_TO_STRING(const Instruction* instr) {
    const register_t src = Reg(instr[1].i32[0]);
    const TypeEntry src_type_entry = type_record_.Get(src);
    const TypeEntry dst_type_entry = TypeEntry::ToPrimitiveAndToString(src_type_entry);

    if (src_type_entry.type().IsString()) {
      // no effect
      type_record_.Put(src, dst_type_entry);
      return;
    }

    LoadVR(rsi, src);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::TO_PRIMITIVE_AND_TO_STRING);
    asm_->mov(qword[r13 + src * kJSValSize], rax);
    set_last_used_candidate(src);
    type_record_.Put(src, dst_type_entry);
  }

  // opcode | (dst | start | count)
  void EmitCONCAT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t start = Reg(instr[1].ssw.i16[1]);
    const uint32_t count = instr[1].ssw.u32;
    assert(!IsConstantID(start));
    asm_->lea(rsi, ptr[r13 + start * kJSValSize]);
    asm_->mov(rdi, r14);
    asm_->mov(edx, count);
    asm_->Call(&stub::CONCAT);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::String()));
  }

  // opcode name
  void EmitRAISE(const Instruction* instr) {
    const Error::Code code = static_cast<Error::Code>(instr[1].u32[0]);
    const uint32_t constant = instr[1].u32[1];
    JSString* str = code_->constants()[constant].string();
    asm_->mov(rdi, r14);
    asm_->mov(esi, code);
    asm_->mov(rdx, core::BitCast<uint64_t>(str));
    asm_->Call(&stub::RAISE);
  }

  // opcode | (dst | src)
  void EmitTYPEOF(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    const TypeEntry src_type_entry = type_record_.Get(src);
    const TypeEntry dst_type_entry = TypeEntry::TypeOf(ctx_, src_type_entry);

    if (dst_type_entry.IsConstant()) {
      EmitConstantDest(dst_type_entry, dst);
      type_record_.Put(dst, dst_type_entry);
      return;
    }

    LoadVR(rsi, src);
    asm_->mov(rdi, r12);
    asm_->Call(&stub::TYPEOF);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_type_entry);
  }

  // opcode | (obj | item) | (index | type)
  void EmitSTORE_OBJECT_INDEXED(const Instruction* instr) {
    const register_t obj = Reg(instr[1].i16[0]);
    const register_t item = Reg(instr[1].i16[1]);
    const uint32_t index = instr[2].u32[0];
    const uint32_t type = instr[2].u32[1];
    LoadVRs(rsi, obj, rdx, item);
    asm_->mov(rdi, r12);
    asm_->mov(ecx, index);
    switch (type) {
      case ObjectLiteral::DATA:
        asm_->Call(&stub::STORE_OBJECT_INDEXED<ObjectLiteral::DATA>);
        break;
      case ObjectLiteral::GET:
        asm_->Call(&stub::STORE_OBJECT_INDEXED<ObjectLiteral::GET>);
        break;
      case ObjectLiteral::SET:
        asm_->Call(&stub::STORE_OBJECT_INDEXED<ObjectLiteral::SET>);
        break;
    }
  }

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_DATA(const Instruction* instr) {
    const register_t obj = Reg(instr[1].i16[0]);
    const register_t item = Reg(instr[1].i16[1]);
    const uint32_t offset = instr[2].u32[0];
    LoadVRs(rdi, obj, rax, item);
    const std::ptrdiff_t data_offset =
        IV_CAST_OFFSET(radio::Cell*, JSObject*) +
        JSObject::SlotsOffset() +
        JSObject::Slots::DataOffset();
    asm_->mov(rdi, qword[rdi + data_offset]);
    asm_->mov(qword[rdi + kJSValSize * offset], rax);
  }

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_GET(const Instruction* instr) {
    const register_t obj = Reg(instr[1].i16[0]);
    const register_t item = Reg(instr[1].i16[1]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t merged = instr[2].u32[1];
    if (merged) {
      LoadVRs(rdi, obj, rax, item);
      const std::ptrdiff_t data_offset =
          IV_CAST_OFFSET(radio::Cell*, JSObject*) +
          JSObject::SlotsOffset() +
          JSObject::Slots::DataOffset();
      asm_->mov(rdi, qword[rdi + data_offset]);
      asm_->mov(rdi, qword[rdi + kJSValSize * offset]);
      // rdi is Accessor Cell
      const std::ptrdiff_t getter_offset =
          IV_CAST_OFFSET(radio::Cell*, Accessor*) +
          Accessor::GetterOffset();
      const std::ptrdiff_t cell_to_jsobject =
          IV_CAST_OFFSET(radio::Cell*, JSObject*);
      if (cell_to_jsobject != 0) {
        asm_->add(rax, cell_to_jsobject);
      }
      asm_->mov(qword[rdi + getter_offset], rax);
    } else {
      LoadVRs(rsi, obj, rdx, item);
      asm_->mov(rdi, r12);
      asm_->mov(ecx, offset);
      asm_->Call(&stub::STORE_OBJECT_GET);
    }
  }

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_SET(const Instruction* instr) {
    const register_t obj = Reg(instr[1].i16[0]);
    const register_t item = Reg(instr[1].i16[1]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t merged = instr[2].u32[1];
    if (merged) {
      LoadVRs(rdi, obj, rax, item);
      const std::ptrdiff_t data_offset =
          IV_CAST_OFFSET(radio::Cell*, JSObject*) +
          JSObject::SlotsOffset() +
          JSObject::Slots::DataOffset();
      asm_->mov(rdi, qword[rdi + data_offset]);
      asm_->mov(rdi, qword[rdi + kJSValSize * offset]);
      // rdi is Accessor Cell
      const std::ptrdiff_t getter_offset =
          IV_CAST_OFFSET(radio::Cell*, Accessor*) +
          Accessor::SetterOffset();
      const std::ptrdiff_t cell_to_jsobject =
          IV_CAST_OFFSET(radio::Cell*, JSObject*);
      if (cell_to_jsobject != 0) {
        asm_->add(rax, cell_to_jsobject);
      }
      asm_->mov(qword[rdi + getter_offset], rax);
    } else {
      LoadVRs(rsi, obj, rdx, item);
      asm_->mov(rdi, r12);
      asm_->mov(ecx, offset);
      asm_->Call(&stub::STORE_OBJECT_SET);
    }
  }

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitLOAD_PROP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t base = Reg(instr[1].ssw.i16[1]);
    const Symbol name = code_->names()[instr[1].ssw.u32];

    const TypeEntry& base_entry = type_record_.Get(base);

    TypeEntry dst_entry = TypeEntry(Type::Unknown());
    if (name == symbol::length()) {
      if (base_entry.type().IsArray() ||
          base_entry.type().IsFunction()) {
        dst_entry = TypeEntry(Type::Number());
      } else if (base_entry.type().IsString()) {
        if (base_entry.IsConstant()) {
          const int32_t length = base_entry.constant().string()->size();
          dst_entry = TypeEntry(length);
          asm_->mov(rax, Extract(JSVal::Int32(length)));
          asm_->mov(qword[r13 + dst * kJSValSize], rax);
          set_last_used_candidate(dst);
          type_record_.Put(dst, dst_entry);
          return;
        } else {
          dst_entry = TypeEntry(Type::Int32());
        }
      }
    }

    const Assembler::LocalLabelScope scope(asm_);

    LoadVR(rsi, base);

    if (symbol::IsArrayIndexSymbol(name)) {
      // generate Array index fast path
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      DenseArrayGuard(base, rsi, rdi, rdx, false, ".ARRAY_FAST_PATH_EXIT");

      // check index is not out of range
      const std::ptrdiff_t vector_offset =
          IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ElementsOffset() + IndexedElements::VectorOffset();
      const std::ptrdiff_t size_offset =
          vector_offset + IndexedElements::DenseArrayVector::SizeOffset();
      asm_->cmp(qword[rsi + size_offset], index);
      asm_->jbe(".ARRAY_FAST_PATH_EXIT");

      // load element from index directly
      const std::ptrdiff_t data_offset =
          vector_offset + IndexedElements::DenseArrayVector::DataOffset();
      asm_->mov(rax, qword[rsi + data_offset]);
      asm_->mov(rax, qword[rax + kJSValSize * index]);

      // check element is not JSEmpty
      NotEmptyGuard(rax, ".ARRAY_FAST_PATH_EXIT");
      asm_->jmp(".EXIT");
      asm_->L(".ARRAY_FAST_PATH_EXIT");

      // load from value
      asm_->mov(rdi, r14);
      asm_->mov(rdx, Extract(JSVal::UInt32(index)));
      asm_->Call(&stub::LOAD_ELEMENT);
    } else {
      asm_->mov(rdi, r14);
      LoadPropertyIC* ic(new LoadPropertyIC(native_code(), name, code_->strict()));
      native_code()->BindIC(ic);
      asm_->mov(rdx, core::BitCast<uint64_t>(ic));
      ic->Call(asm_);
    }
    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);

    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_entry);
  }

  // opcode | (base | src | index) | nop | nop
  void EmitSTORE_PROP(const Instruction* instr) {
    const register_t base = Reg(instr[1].ssw.i16[0]);
    const register_t src = Reg(instr[1].ssw.i16[1]);
    const Symbol name = code_->names()[instr[1].ssw.u32];

    const Assembler::LocalLabelScope scope(asm_);

    LoadVRs(rsi, base, rdx, src);

    if (symbol::IsArrayIndexSymbol(name)) {
      // generate Array index fast path
      const uint32_t index = symbol::GetIndexFromSymbol(name);
      StoreElementIC* ic(new StoreElementIC(native_code(), code_->strict(), index));
      native_code()->BindIC(ic);

      if (index < IndexedElements::kMaxVectorSize) {
        DenseArrayGuard(base, rsi, rdi, r11, true, ".ARRAY_EXIT", Xbyak::CodeGenerator::T_NEAR);

        // check index is not out of range
        const std::ptrdiff_t vector_offset =
            IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ElementsOffset() + IndexedElements::VectorOffset();
        const std::ptrdiff_t size_offset =
            vector_offset + IndexedElements::DenseArrayVector::SizeOffset();
        asm_->cmp(qword[rsi + size_offset], index);
        asm_->jbe(".ARRAY_CAPACITY");

        // fast path
        const std::ptrdiff_t data_offset =
            vector_offset + IndexedElements::DenseArrayVector::DataOffset();
        asm_->mov(rax, qword[rsi + data_offset]);
        asm_->mov(r11, qword[rax + kJSValSize * index]);
        asm_->test(r11, r11);
        asm_->jz(".ARRAY_IC_PATH");
        asm_->mov(qword[rax + kJSValSize * index], rdx);
        asm_->jmp(".EXIT", Xbyak::CodeGenerator::T_NEAR);

        // capacity check
        // This path may introduce length grow
        asm_->L(".ARRAY_CAPACITY");
        const std::ptrdiff_t capacity_offset =
            vector_offset + IndexedElements::DenseArrayVector::CapacityOffset();
        asm_->cmp(qword[rsi + capacity_offset], index);
        asm_->jbe(".ARRAY_EXIT");
        asm_->mov(r11d, 1);  // this is flag

        asm_->L(".ARRAY_IC_PATH");
        asm_->mov(rdi, r14);
        asm_->mov(ecx, index);
        asm_->mov(r8, core::BitCast<uint64_t>(ic));
        ic->Call(asm_);
        asm_->jmp(".EXIT");
      }

      // store element generic
      asm_->L(".ARRAY_EXIT");
      CheckObjectCoercible(base, rsi, rdi);
      asm_->mov(rdi, r14);
      asm_->mov(rcx, Extract(JSVal::UInt32(index)));
      asm_->mov(r8, core::BitCast<uint64_t>(ic));
      asm_->Call(&stub::STORE_ELEMENT_GENERIC);
    } else {
      asm_->mov(rdi, r14);
      StorePropertyIC* ic(new StorePropertyIC(native_code(), name, code_->strict()));
      native_code()->BindIC(ic);
      asm_->mov(rcx, core::BitCast<uint64_t>(ic));
      ic->Call(asm_);
    }
    asm_->L(".EXIT");
  }

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitDELETE_PROP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t base = Reg(instr[1].ssw.i16[1]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    LoadVR(rsi, base);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::DELETE_PROP<true>);
    } else {
      asm_->Call(&stub::DELETE_PROP<false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Boolean()));
  }

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitINCREMENT_PROP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t base = Reg(instr[1].ssw.i16[1]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    LoadVR(rsi, base);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_PROP<1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_PROP<1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitDECREMENT_PROP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t base = Reg(instr[1].ssw.i16[1]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    LoadVR(rsi, base);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_PROP<-1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_PROP<-1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitPOSTFIX_INCREMENT_PROP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t base = Reg(instr[1].ssw.i16[1]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    LoadVR(rsi, base);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_PROP<1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_PROP<1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitPOSTFIX_DECREMENT_PROP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t base = Reg(instr[1].ssw.i16[1]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    LoadVR(rsi, base);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_PROP<-1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_PROP<-1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode
  void EmitPOP_ENV(const Instruction* instr) {
    asm_->mov(rdi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdi, ptr[rdi + JSEnv::OuterOffset()]);
    asm_->mov(ptr[r13 + offsetof(railgun::Frame, lexical_env_)], rdi);
    // save previous register because NOP does nothing
    set_last_used_candidate(last_used());
  }

  // opcode | (ary | reg) | (index | size)
  void EmitINIT_VECTOR_ARRAY_ELEMENT(const Instruction* instr) {
    const register_t ary = Reg(instr[1].i16[0]);
    const register_t reg = Reg(instr[1].i16[1]);
    const uint32_t index = instr[2].u32[0];
    const uint32_t size = instr[2].u32[1];
    LoadVR(rdi, ary);
    assert(!IsConstantID(reg));
    asm_->lea(rsi, ptr[r13 + reg * kJSValSize]);
    asm_->mov(edx, index);
    asm_->mov(ecx, size);
    asm_->Call(&stub::INIT_VECTOR_ARRAY_ELEMENT);
  }

  // opcode | (ary | reg) | (index | size)
  void EmitINIT_SPARSE_ARRAY_ELEMENT(const Instruction* instr) {
    const register_t ary = Reg(instr[1].i16[0]);
    const register_t reg = Reg(instr[1].i16[1]);
    const uint32_t index = instr[2].u32[0];
    const uint32_t size = instr[2].u32[1];
    LoadVR(rdi, ary);
    assert(!IsConstantID(reg));
    asm_->lea(rsi, ptr[r13 + reg * kJSValSize]);
    asm_->mov(edx, index);
    asm_->mov(ecx, size);
    asm_->Call(&stub::INIT_SPARSE_ARRAY_ELEMENT);
  }

  // opcode | (dst | size)
  void EmitLOAD_ARRAY(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t size = instr[1].ssw.u32;
    asm_->mov(rdi, r12);
    asm_->mov(esi, size);
    asm_->Call(&stub::LOAD_ARRAY);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Array()));
  }

  // opcode | (dst | offset)
  void EmitDUP_ARRAY(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[1].ssw.u32;
    asm_->mov(rdi, r12);
    asm_->mov(rsi, Extract(code_->constants()[offset]));
    asm_->Call(&stub::DUP_ARRAY);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    type_record_.Put(dst, TypeEntry(Type::Array()));
  }

  // opcode | (dst | code)
  void EmitLOAD_FUNCTION(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    railgun::Code* target = code_->codes()[instr[1].ssw.u32];
    asm_->mov(rdi, r12);
    asm_->mov(rsi, core::BitCast<uint64_t>(target));
    asm_->mov(rdx, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->Call(&JSJITFunction::New);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Function()));
  }

  // opcode | (dst | offset)
  void EmitLOAD_REGEXP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    JSRegExp* regexp =
        static_cast<JSRegExp*>(code_->constants()[instr[1].ssw.u32].object());
    asm_->mov(rdi, r12);
    asm_->mov(rsi, core::BitCast<uint64_t>(regexp));
    asm_->Call(&stub::LOAD_REGEXP);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Object()));
  }

  // opcode | dst | map
  void EmitLOAD_OBJECT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i32[0]);
    asm_->mov(rdi, r12);
    asm_->mov(rsi, core::BitCast<uint64_t>(instr[2].map));
    asm_->Call(&stub::LOAD_OBJECT);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Object()));
  }

  // opcode | (dst | base | element)
  void EmitLOAD_ELEMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t base = Reg(instr[1].i16[1]);
    const register_t element = Reg(instr[1].i16[2]);

    const TypeEntry dst_entry = TypeEntry(Type::Unknown());

    const Assembler::LocalLabelScope scope(asm_);

    LoadVRs(rsi, base, rdx, element);

    {
      // check element is int32_t and element >= 0
      Int32Guard(element, rdx, ".ARRAY_FAST_PATH_EXIT");

      // generate Array index fast path
      DenseArrayGuard(base, rsi, rdi, r11, false, ".ARRAY_FAST_PATH_EXIT");

      // check index is not out of range
      const std::ptrdiff_t vector_offset =
          IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ElementsOffset() + IndexedElements::VectorOffset();
      const std::ptrdiff_t size_offset =
          vector_offset + IndexedElements::DenseArrayVector::SizeOffset();
      asm_->mov(ecx, edx);
      asm_->cmp(rcx, qword[rsi + size_offset]);
      asm_->jae(".ARRAY_FAST_PATH_EXIT");

      // load element from index directly
      const std::ptrdiff_t data_offset =
          vector_offset + IndexedElements::DenseArrayVector::DataOffset();
      asm_->mov(rax, qword[rsi + data_offset]);
      asm_->mov(rax, qword[rax + rcx * kJSValSize]);

      // check element is not JSEmpty
      NotEmptyGuard(rax, ".ARRAY_FAST_PATH_EXIT");
      asm_->jmp(".EXIT");
      asm_->L(".ARRAY_FAST_PATH_EXIT");
    }

    CheckObjectCoercible(base, rsi, rdi);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::LOAD_ELEMENT);

    asm_->L(".EXIT");
    asm_->mov(qword[r13 + dst * kJSValSize], rax);

    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_entry);
  }

  // opcode | (base | element | src)
  void EmitSTORE_ELEMENT(const Instruction* instr) {
    const register_t base = Reg(instr[1].i16[0]);
    const register_t element = Reg(instr[1].i16[1]);
    const register_t src = Reg(instr[1].i16[2]);

    const Assembler::LocalLabelScope scope(asm_);

    LoadVRs(rsi, base, rcx, element);
    LoadVR(rdx, src);

    // check element is int32_t and element >= 0
    Int32Guard(element, rcx, ".ARRAY_EXIT", Xbyak::CodeGenerator::T_NEAR);
    asm_->cmp(ecx, 0);
    asm_->jl(".ARRAY_EXIT", Xbyak::CodeGenerator::T_NEAR);

    // generate Array index fast path
    DenseArrayGuard(base, rsi, rdi, r11, true, ".ARRAY_EXIT", Xbyak::CodeGenerator::T_NEAR);

    // check index is not out of range
    const std::ptrdiff_t vector_offset =
        IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ElementsOffset() + IndexedElements::VectorOffset();
    const std::ptrdiff_t size_offset =
        vector_offset + IndexedElements::DenseArrayVector::SizeOffset();
    asm_->mov(rdi, rcx);
    asm_->mov(ecx, ecx);
    asm_->cmp(qword[rsi + size_offset], rcx);
    asm_->jbe(".ARRAY_CAPACITY");

    // store element directly
    const std::ptrdiff_t data_offset =
        vector_offset + IndexedElements::DenseArrayVector::DataOffset();
    asm_->mov(rax, qword[rsi + data_offset]);
    asm_->mov(r11, qword[rax + rcx * kJSValSize]);
    asm_->test(r11, r11);  // check empty
    asm_->jz(".ARRAY_IC_PATH");
    asm_->mov(qword[rax + rcx * kJSValSize], rdx);
    asm_->jmp(".EXIT");

    // capacity check
    // This path may introduce length grow
    asm_->L(".ARRAY_CAPACITY");
    const std::ptrdiff_t capacity_offset =
        vector_offset + IndexedElements::DenseArrayVector::CapacityOffset();
    asm_->cmp(qword[rsi + capacity_offset], rcx);
    asm_->jbe(".ARRAY_RESTORE_INDEX");
    asm_->mov(r11d, 1);  // this is flag

    asm_->L(".ARRAY_IC_PATH");
    StoreElementIC* ic(new StoreElementIC(native_code(), code_->strict()));
    native_code()->BindIC(ic);

    asm_->mov(rdi, r14);
    asm_->mov(r8, core::BitCast<uint64_t>(ic));
    ic->Call(asm_);
    asm_->jmp(".EXIT");

    asm_->L(".ARRAY_RESTORE_INDEX");
    asm_->mov(rcx, rdi);

    // store element generic
    asm_->L(".ARRAY_EXIT");
    CheckObjectCoercible(base, rsi, rdi);
    asm_->mov(rdi, r14);
    asm_->mov(r8, core::BitCast<uint64_t>(ic));
    asm_->Call(&stub::STORE_ELEMENT_GENERIC);

    asm_->L(".EXIT");
  }

  // opcode | (dst | base | element)
  void EmitDELETE_ELEMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t base = Reg(instr[1].i16[1]);
    const register_t element = Reg(instr[1].i16[2]);
    LoadVRs(rsi, base, rdx, element);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    if (code_->strict()) {
      asm_->Call(&stub::DELETE_ELEMENT<true>);
    } else {
      asm_->Call(&stub::DELETE_ELEMENT<false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Boolean()));
  }

  // opcode | (dst | base | element)
  void EmitINCREMENT_ELEMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t base = Reg(instr[1].i16[1]);
    const register_t element = Reg(instr[1].i16[2]);
    LoadVRs(rsi, base, rdx, element);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_ELEMENT<1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_ELEMENT<1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | base | element)
  void EmitDECREMENT_ELEMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t base = Reg(instr[1].i16[1]);
    const register_t element = Reg(instr[1].i16[2]);
    LoadVRs(rsi, base, rdx, element);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_ELEMENT<-1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_ELEMENT<-1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | base | element)
  void EmitPOSTFIX_INCREMENT_ELEMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t base = Reg(instr[1].i16[1]);
    const register_t element = Reg(instr[1].i16[2]);
    LoadVRs(rsi, base, rdx, element);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_ELEMENT<1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_ELEMENT<1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | base | element)
  void EmitPOSTFIX_DECREMENT_ELEMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t base = Reg(instr[1].i16[1]);
    const register_t element = Reg(instr[1].i16[2]);
    LoadVRs(rsi, base, rdx, element);
    asm_->mov(rdi, r14);
    CheckObjectCoercible(base, rsi, rcx);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_ELEMENT<-1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_ELEMENT<-1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | dst
  void EmitRESULT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i32[0]);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);

    if (previous_instr() && previous_instr()->GetOP() == OP::CONSTRUCT) {
      // [[Construct]] always returns object.
      // This is ensured by ECMA262 spec.
      type_record_.Put(dst, TypeEntry(Type::Object()));
    } else {
      type_record_.Put(dst, TypeEntry(Type::Unknown()));
    }
  }

  // opcode | src
  void EmitRETURN(const Instruction* instr) {
    // Calling convension is different from railgun::VM.
    // Constructor checks value is object in call site, not callee site.
    // So r13 is still callee Frame.
    const register_t src = Reg(instr[1].i32[0]);
    LoadVR(rax, src);
    asm_->add(rsp, k64Size);
    asm_->add(qword[r14 + offsetof(Frame, ret)], k64Size * kStackPayload);
    asm_->ret();
  }

  // opcode | (dst | index) | nop | nop
  void EmitLOAD_GLOBAL(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];

    TypeEntry dst_entry(Type::Unknown());

    // Some variables are defined as non-configurable.
    const JSGlobal::Constant constant = JSGlobal::LookupConstant(name);
    if (constant.first) {
      asm_->mov(rax, Extract(constant.second));
      dst_entry = TypeEntry(constant.second);
    } else {
      GlobalIC* ic(new GlobalIC(code_->strict()));
      ic->CompileLoad(asm_, ctx_->global_obj(), code_, name);
      native_code()->BindIC(ic);
    }

    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_entry);
  }

  // opcode | (src | name) | nop | nop
  void EmitSTORE_GLOBAL(const Instruction* instr) {
    const register_t src = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    LoadVR(rax, src);
    GlobalIC* ic(new GlobalIC(code_->strict()));
    ic->CompileStore(asm_, ctx_->global_obj(), code_, name);
    native_code()->BindIC(ic);
  }

  // opcode | dst | slot
  void EmitLOAD_GLOBAL_DIRECT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i32[0]);
    StoredSlot* slot = instr[2].slot;
    TypeEntry dst_entry(Type::Unknown());
    asm_->mov(rax, core::BitCast<uint64_t>(slot));
    asm_->mov(rax, ptr[rax]);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, dst_entry);
  }

  // opcode | src | slot
  void EmitSTORE_GLOBAL_DIRECT(const Instruction* instr) {
    const register_t src = Reg(instr[1].i32[0]);
    StoredSlot* slot = instr[2].slot;

    const Assembler::LocalLabelScope scope(asm_);
    LoadVR(rax, src);
    asm_->mov(rcx, core::BitCast<uint64_t>(slot));
    asm_->test(word[rcx + kJSValSize], ATTR::W);
    asm_->jnz(".STORE");
    if (code_->strict()) {
      static const char* message = "modifying global variable failed";
      asm_->mov(rdi, r14);
      asm_->mov(rsi, Error::Type);
      asm_->mov(rdx, core::BitCast<uint64_t>(message));
      asm_->Call(&stub::THROW_WITH_TYPE_AND_MESSAGE);
    } else {
      asm_->jmp(".EXIT");
    }
    asm_->L(".STORE");
    asm_->mov(qword[rcx], rax);
    asm_->L(".EXIT");
  }

  // opcode | (dst | name) | nop | nop
  void EmitDELETE_GLOBAL(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(rdi, r14);
    asm_->mov(rsi, core::BitCast<uint64_t>(name));
    asm_->Call(&stub::DELETE_GLOBAL);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Boolean()));
  }

  // opcode | (dst | name) | nop | nop
  void EmitTYPEOF_GLOBAL(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(rdi, r14);
    asm_->mov(rsi, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::TYPEOF_GLOBAL<true>);
    } else {
      asm_->Call(&stub::TYPEOF_GLOBAL<false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::String()));
  }

  // opcode | (dst | imm | index) | (offset | nest)
  void EmitLOAD_HEAP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const bool immutable = !!instr[1].ssw.i16[1];
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];

    const Assembler::LocalLabelScope scope(asm_);

    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    LookupHeapEnv(rsi, nest);
    const ptrdiff_t target =
        IV_CAST_OFFSET(JSEnv*, JSDeclEnv*) +
        JSDeclEnv::StaticOffset() +
        JSDeclEnv::StaticVals::DataOffset();
    // pointer to the data
    asm_->mov(rax, qword[rsi + target]);
    asm_->mov(rax, qword[rax + kJSValSize * offset]);
    if (immutable) {
      EmptyGuard(rax, ".NOT_EMPTY");
      if (code_->strict()) {
        static const char* message = "uninitialized value access not allowed in strict code";
        asm_->mov(rdi, r14);
        asm_->mov(rsi, Error::Reference);
        asm_->mov(rdx, core::BitCast<uint64_t>(message));
        asm_->Call(&stub::THROW_WITH_TYPE_AND_MESSAGE);
      } else {
        asm_->mov(rax, Extract(JSUndefined));
      }
      asm_->L(".NOT_EMPTY");
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Unknown()));
  }

  // opcode | (src | imm | name) | (offset | nest)
  void EmitSTORE_HEAP(const Instruction* instr) {
    const register_t src = Reg(instr[1].ssw.i16[0]);
    const bool immutable = !!instr[1].ssw.i16[1];
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];

    const Assembler::LocalLabelScope scope(asm_);

    if (immutable) {
      if (code_->strict()) {
        static const char* message = "mutating immutable binding not allowed";
        asm_->mov(rdi, r14);
        asm_->mov(rsi, Error::Type);
        asm_->mov(rdx, core::BitCast<uint64_t>(message));
        asm_->Call(&stub::THROW_WITH_TYPE_AND_MESSAGE);
      }
      return;
    }

    LoadVR(rax, src);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    LookupHeapEnv(rsi, nest);
    const ptrdiff_t target =
        IV_CAST_OFFSET(JSEnv*, JSDeclEnv*) +
        JSDeclEnv::StaticOffset() +
        JSDeclEnv::StaticVals::DataOffset();
    // pointer to the data
    asm_->mov(rdi, qword[rsi + target]);
    asm_->mov(qword[rdi + kJSValSize * offset], rax);
  }

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitDELETE_HEAP(const Instruction* instr) {
    static const uint64_t layout = Extract(JSFalse);
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(qword[r13 + dst * kJSValSize], layout);
    type_record_.Put(dst, TypeEntry(Type::Boolean()));
  }

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitINCREMENT_HEAP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(rdi, r14);

    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    LookupHeapEnv(rsi, nest);

    asm_->mov(edx, offset);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_HEAP<1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_HEAP<1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitDECREMENT_HEAP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(rdi, r14);

    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    LookupHeapEnv(rsi, nest);

    asm_->mov(edx, offset);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_HEAP<-1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_HEAP<-1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitPOSTFIX_INCREMENT_HEAP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(rdi, r14);

    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    LookupHeapEnv(rsi, nest);

    asm_->mov(edx, offset);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_HEAP<1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_HEAP<1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitPOSTFIX_DECREMENT_HEAP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];
    asm_->mov(rdi, r14);

    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    LookupHeapEnv(rsi, nest);

    asm_->mov(edx, offset);
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_HEAP<-1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_HEAP<-1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitTYPEOF_HEAP(const Instruction* instr) {
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const bool immutable = !!instr[1].ssw.i16[1];
    const uint32_t offset = instr[2].u32[0];
    const uint32_t nest = instr[2].u32[1];

    const Assembler::LocalLabelScope scope(asm_);

    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    LookupHeapEnv(rsi, nest);
    const ptrdiff_t target =
        IV_CAST_OFFSET(JSEnv*, JSDeclEnv*) +
        JSDeclEnv::StaticOffset() +
        JSDeclEnv::StaticVals::DataOffset();
    // pointer to the data
    asm_->mov(rax, qword[rsi + target]);
    asm_->mov(rsi, qword[rax + kJSValSize * offset]);
    if (immutable) {
      EmptyGuard(rsi, ".NOT_EMPTY");
      if (code_->strict()) {
        static const char* message = "uninitialized value access not allowed in strict code";
        asm_->mov(rdi, r14);
        asm_->mov(rsi, Error::Reference);
        asm_->mov(rdx, core::BitCast<uint64_t>(message));
        asm_->Call(&stub::THROW_WITH_TYPE_AND_MESSAGE);
      } else {
        asm_->mov(rsi, Extract(JSUndefined));
      }
      asm_->L(".NOT_EMPTY");
    }

    asm_->mov(rdi, r12);
    asm_->Call(&stub::TYPEOF);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::String()));
  }

  // opcode | (callee | offset | argc_with_this)
  void EmitCALL(const Instruction* instr);
  // opcode | (callee | offset | argc_with_this)
  void EmitCONSTRUCT(const Instruction* instr);
  // opcode | (callee | offset | argc_with_this)
  void EmitEVAL(const Instruction* instr);

  // opcode | (name | configurable)
  void EmitINSTANTIATE_DECLARATION_BINDING(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].u32[0]];
    const bool configurable = instr[1].u32[1];
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, variable_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
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
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, variable_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
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
    const register_t src = Reg(instr[1].ssw.i16[0]);
    const uint32_t offset = instr[1].ssw.u32;
    asm_->mov(rdi, ptr[r13 + offsetof(railgun::Frame, variable_env_)]);
    LoadVR(rsi, src);
    asm_->mov(edx, offset);
    asm_->Call(&stub::INITIALIZE_HEAP_IMMUTABLE);
  }

  // opcode | src
  void EmitINCREMENT(const Instruction* instr) {
    const register_t src = Reg(instr[1].i32[0]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MAX) + 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rax, src);
      Int32Guard(src, rax, ".INCREMENT_SLOW");
      asm_->add(eax, 1);
      asm_->jo(".INCREMENT_OVERFLOW");

      asm_->or(rax, r15);
      asm_->jmp(".INCREMENT_EXIT");

      asm_->L(".INCREMENT_OVERFLOW");
      // overflow ==> INT32_MAX + 1
      asm_->mov(rax, overflow);
      asm_->jmp(".INCREMENT_EXIT");

      asm_->L(".INCREMENT_SLOW");
      asm_->mov(rdi, r14);
      asm_->mov(rsi, rax);
      asm_->Call(&stub::INCREMENT);

      asm_->L(".INCREMENT_EXIT");
      asm_->mov(qword[r13 + src * kJSValSize], rax);
      set_last_used_candidate(src);
      type_record_.Put(src, TypeEntry::Increment(type_record_.Get(src)));
    }
  }

  // opcode | src
  void EmitDECREMENT(const Instruction* instr) {
    const register_t src = Reg(instr[1].i32[0]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MIN) - 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rax, src);
      Int32Guard(src, rax, ".DECREMENT_SLOW");
      asm_->sub(eax, 1);
      asm_->jo(".DECREMENT_OVERFLOW");

      asm_->or(rax, r15);
      asm_->jmp(".DECREMENT_EXIT");

      // overflow ==> INT32_MIN - 1
      asm_->L(".DECREMENT_OVERFLOW");
      asm_->mov(rax, overflow);
      asm_->jmp(".DECREMENT_EXIT");

      asm_->L(".DECREMENT_SLOW");
      asm_->mov(rsi, rax);
      asm_->mov(rdi, r14);
      asm_->Call(&stub::DECREMENT);

      asm_->L(".DECREMENT_EXIT");
      asm_->mov(ptr[r13 + src * kJSValSize], rax);
      set_last_used_candidate(src);
      type_record_.Put(src, TypeEntry::Decrement(type_record_.Get(src)));
    }
  }

  // opcode | (dst | src)
  void EmitPOSTFIX_INCREMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MAX) + 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rsi, src);
      Int32Guard(src, rsi, ".INCREMENT_SLOW");
      asm_->mov(ptr[r13 + dst * kJSValSize], rsi);
      asm_->add(esi, 1);
      asm_->jo(".INCREMENT_OVERFLOW");

      asm_->mov(eax, esi);
      asm_->or(rax, r15);
      asm_->jmp(".INCREMENT_EXIT");

      // overflow ==> INT32_MAX + 1
      asm_->L(".INCREMENT_OVERFLOW");
      asm_->mov(rax, overflow);
      asm_->jmp(".INCREMENT_EXIT");

      asm_->L(".INCREMENT_SLOW");
      asm_->mov(rdi, r14);
      assert(!IsConstantID(dst));
      asm_->lea(rdx, ptr[r13 + dst * kJSValSize]);
      asm_->Call(&stub::POSTFIX_INCREMENT);

      asm_->L(".INCREMENT_EXIT");
      asm_->mov(ptr[r13 + src * kJSValSize], rax);
      set_last_used_candidate(src);
      {
        const TypeEntry from = type_record_.Get(src);
        type_record_.Put(dst, TypeEntry::ToNumber(from));
        type_record_.Put(src, TypeEntry::Increment(from));
      }
    }
  }

  // opcode | (dst | src)
  void EmitPOSTFIX_DECREMENT(const Instruction* instr) {
    const register_t dst = Reg(instr[1].i16[0]);
    const register_t src = Reg(instr[1].i16[1]);
    static const uint64_t overflow = Extract(JSVal(static_cast<double>(INT32_MIN) - 1));
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rsi, src);
      Int32Guard(src, rsi, ".DECREMENT_SLOW");
      asm_->mov(ptr[r13 + dst * kJSValSize], rsi);
      asm_->sub(esi, 1);
      asm_->jo(".DECREMENT_OVERFLOW");

      asm_->mov(eax, esi);
      asm_->or(rax, r15);
      asm_->jmp(".DECREMENT_EXIT");

      // overflow ==> INT32_MIN - 1
      asm_->L(".DECREMENT_OVERFLOW");
      asm_->mov(rax, overflow);
      asm_->jmp(".DECREMENT_EXIT");

      asm_->L(".DECREMENT_SLOW");
      asm_->mov(rdi, r14);
      assert(!IsConstantID(dst));
      asm_->lea(rdx, ptr[r13 + dst * kJSValSize]);
      asm_->Call(&stub::POSTFIX_DECREMENT);

      asm_->L(".DECREMENT_EXIT");
      asm_->mov(ptr[r13 + src * kJSValSize], rax);
      set_last_used_candidate(src);
      {
        const TypeEntry from = type_record_.Get(src);
        type_record_.Put(dst, TypeEntry::ToNumber(from));
        type_record_.Put(src, TypeEntry::Increment(from));
      }
    }
  }

  // opcode | (dst | basedst | name)
  void EmitPREPARE_DYNAMIC_CALL(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    const register_t base = Reg(instr[1].ssw.i16[1]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    assert(!IsConstantID(base));
    asm_->lea(rcx, ptr[r13 + base * kJSValSize]);
    asm_->Call(&stub::PREPARE_DYNAMIC_CALL);
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(base, TypeEntry(Type::Unknown()));
    type_record_.Put(dst, TypeEntry(Type::Unknown()));
  }

  // opcode | (jmp | cond)
  void EmitIF_FALSE(const Instruction* instr) {
    // TODO(Constelation) inlining this
    const register_t cond = Reg(instr[1].jump.i16[0]);
    const std::string label = MakeLabel(instr);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rdi, cond);

      // boolean and int32_t zero fast cases
      asm_->cmp(rdi, r15);
      asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);

      asm_->cmp(rdi, detail::jsval64::kFalse);
      asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);

      asm_->cmp(rdi, detail::jsval64::kTrue);
      asm_->je(".IF_FALSE_EXIT");

      asm_->Call(&stub::TO_BOOLEAN);
      asm_->test(eax, eax);
      asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);

      asm_->L(".IF_FALSE_EXIT");
    }
  }

  // opcode | (jmp | cond)
  void EmitIF_TRUE(const Instruction* instr) {
    // TODO(Constelation) inlining this
    const register_t cond = Reg(instr[1].jump.i16[0]);
    const std::string label = MakeLabel(instr);
    {
      const Assembler::LocalLabelScope scope(asm_);
      LoadVR(rdi, cond);

      // boolean and int32_t zero fast cases
      asm_->cmp(rdi, r15);
      asm_->je(".IF_TRUE_EXIT");

      asm_->cmp(rdi, detail::jsval64::kFalse);
      asm_->je(".IF_TRUE_EXIT");

      asm_->cmp(rdi, detail::jsval64::kTrue);
      asm_->je(label.c_str(), Xbyak::CodeGenerator::T_NEAR);

      asm_->Call(&stub::TO_BOOLEAN);
      asm_->test(eax, eax);
      asm_->jnz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);

      asm_->L(".IF_TRUE_EXIT");
    }
  }

  // opcode | (jmp : addr | flag)
  void EmitJUMP_SUBROUTINE(const Instruction* instr) {
    static const uint64_t layout = Extract(JSVal::Int32(railgun::VM::kJumpFromSubroutine));
    const register_t addr = Reg(instr[1].jump.i16[0]);
    const register_t flag = Reg(instr[1].jump.i16[1]);
    const std::string label = MakeLabel(instr);

    // register position and repatch afterward
    Assembler::RepatchSite site;
    site.Mov(asm_, rax);
    // Value is JSVal, but, this indicates pointer to address
    asm_->mov(qword[r13 + addr * kJSValSize], rax);
    asm_->mov(rax, layout);
    asm_->mov(qword[r13 + flag * kJSValSize], rax);
    asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);

    asm_->align(2);
    // Now, LSB of ptr to this place is 0
    // So we rotate this bit, make dummy double value
    // and store to virtual register
    unresolved_address_map_.insert(std::make_pair(asm_->size(), site));
  }

  // opcode | (jmp : iterator | enumerable)
  void EmitFORIN_SETUP(const Instruction* instr) {
    const register_t iterator = Reg(instr[1].jump.i16[0]);
    const register_t enumerable = Reg(instr[1].jump.i16[1]);
    const std::string label = MakeLabel(instr);
    LoadVR(rsi, enumerable);
    NotNullOrUndefinedGuard(rsi, rdi, label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    asm_->mov(rdi, r14);
    asm_->Call(&stub::FORIN_SETUP);
    asm_->mov(ptr[r13 + iterator * kJSValSize], rax);
    set_last_used_candidate(iterator);
  }

  // opcode | (jmp : dst | iterator)
  void EmitFORIN_ENUMERATE(const Instruction* instr) {
    const register_t dst = Reg(instr[1].jump.i16[0]);
    const register_t iterator = Reg(instr[1].jump.i16[1]);
    const std::string label = MakeLabel(instr);
    asm_->mov(rdi, r12);
    LoadVR(rsi, iterator);
    asm_->Call(&stub::FORIN_ENUMERATE);
    asm_->test(rax, rax);
    asm_->jz(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
    asm_->mov(ptr[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::String()));
  }

  // opcode | iterator
  void EmitFORIN_LEAVE(const Instruction* instr) {
    const register_t iterator = Reg(instr[1].i32[0]);
    asm_->mov(rdi, r12);
    LoadVR(rsi, iterator);
    asm_->Call(&stub::FORIN_LEAVE);
  }

  // opcode | (error | name)
  void EmitTRY_CATCH_SETUP(const Instruction* instr) {
    const register_t error = Reg(instr[1].ssw.i16[0]);
    const Symbol name = code_->names()[instr[1].ssw.u32];
    asm_->mov(rdi, r12);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    LoadVR(rcx, error);
    asm_->Call(&stub::TRY_CATCH_SETUP);
    asm_->mov(qword[r13 + offsetof(railgun::Frame, lexical_env_)], rax);
  }

  // opcode | (dst | index)
  void EmitLOAD_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::LOAD_NAME<true>);
    } else {
      asm_->Call(&stub::LOAD_NAME<false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Unknown()));
  }

  // opcode | (src | name)
  void EmitSTORE_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t src = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    LoadVR(rcx, src);
    if (code_->strict()) {
      asm_->Call(&stub::STORE_NAME<true>);
    } else {
      asm_->Call(&stub::STORE_NAME<false>);
    }
  }

  // opcode | (dst | name)
  void EmitDELETE_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::DELETE_NAME<true>);
    } else {
      asm_->Call(&stub::DELETE_NAME<false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Boolean()));
  }

  // opcode | (dst | name)
  void EmitINCREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_NAME<1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_NAME<1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | name)
  void EmitDECREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_NAME<-1, 1, true>);
    } else {
      asm_->Call(&stub::INCREMENT_NAME<-1, 1, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | name)
  void EmitPOSTFIX_INCREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_NAME<1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_NAME<1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | name)
  void EmitPOSTFIX_DECREMENT_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::INCREMENT_NAME<-1, 0, true>);
    } else {
      asm_->Call(&stub::INCREMENT_NAME<-1, 0, false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Number()));
  }

  // opcode | (dst | name)
  void EmitTYPEOF_NAME(const Instruction* instr) {
    const Symbol name = code_->names()[instr[1].ssw.u32];
    const register_t dst = Reg(instr[1].ssw.i16[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, ptr[r13 + offsetof(railgun::Frame, lexical_env_)]);
    asm_->mov(rdx, core::BitCast<uint64_t>(name));
    if (code_->strict()) {
      asm_->Call(&stub::TYPEOF_NAME<true>);
    } else {
      asm_->Call(&stub::TYPEOF_NAME<false>);
    }
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::String()));
  }

  // opcode | jmp
  void EmitJUMP_BY(const Instruction* instr) {
    Jump(instr);
  }

  // opcode | dst
  void EmitLOAD_ARGUMENTS(const Instruction* instr) {
    const int32_t dst = Reg(instr[1].i32[0]);
    asm_->mov(rdi, r14);
    asm_->mov(rsi, r13);
    if (code_->strict()) {
      asm_->Call(&stub::LOAD_ARGUMENTS<true>);
    } else {
      asm_->Call(&stub::LOAD_ARGUMENTS<false>);
    }
    asm_->mov(ptr[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
    type_record_.Put(dst, TypeEntry(Type::Arguments()));
  }

  // NaN is not handled
  void ConvertNotNaNDoubleToJSVal(const Xbyak::Reg64& target) {
    // Adding double offset is equivalent to subtracting number mask.
    static_assert(
        detail::jsval64::kDoubleOffset + detail::jsval64::kNumberMask == 0,
        "double offset + number mask should be 0");
    asm_->sub(target, r15);
  }

  void ConvertBooleanToJSVal(const Xbyak::Reg8& cond,
                             const Xbyak::Reg64& result) {
    assert(cond.getIdx() != result.getIdx());
    asm_->mov(result, detail::jsval64::kBooleanRepresentation);
    asm_->or(Xbyak::Reg8(result.getIdx()), cond);
  }

  void Int32Guard(register_t reg,
                  const Xbyak::Reg64& target,
                  const char* label,
                  Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    if (type_record_.Get(reg).type().IsInt32()) {
      // no check
      return;
    }
    asm_->cmp(target, r15);
    asm_->jb(label, type);
  }

  void NumberGuard(register_t reg,
                   const Xbyak::Reg64& target,
                   const char* label,
                   Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    if (type_record_.Get(reg).type().IsNumber()) {
      // no check
      return;
    }
    asm_->test(target, r15);
    asm_->jz(label, type);
  }

  void LoadCellTag(const Xbyak::Reg64& target, const Xbyak::Reg32& out) {
    // Because of Little Endianess
    static_assert(core::kLittleEndian, "System should be little endianess");
    asm_->mov(out, word[target + radio::Cell::TagOffset()]);  // NOLINT
  }

  void CompareCellTag(const Xbyak::Reg64& target, uint8_t tag) {
    asm_->cmp(word[target + radio::Cell::TagOffset()], tag);  // NOLINT
  }

  void LoadClassTag(const Xbyak::Reg64& target,
                    const Xbyak::Reg64& tmp,
                    const Xbyak::Reg32& out) {
    const std::ptrdiff_t offset = IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ClassOffset();
    asm_->mov(tmp, qword[target + offset]);
    asm_->mov(out, word[tmp + IV_OFFSETOF(Class, type)]);
  }

  void EmptyGuard(const Xbyak::Reg64& target,
                  const char* label,
                  Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    assert(Extract(JSEmpty) == 0);  // Because of null pointer
    asm_->test(target, target);
    asm_->jnz(label, type);
  }

  void NotEmptyGuard(const Xbyak::Reg64& target,
                     const char* label,
                     Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    assert(Extract(JSEmpty) == 0);  // Because of null pointer
    asm_->test(target, target);
    asm_->jz(label, type);
  }

  void NotNullOrUndefinedGuard(const Xbyak::Reg64& target,
                               const Xbyak::Reg64& tmp,
                               const char* label,
                               Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    // (1000)2 = 8
    // Null is (0010)2 and Undefined is (1010)2
    // ~UINT64_C(8) value is -9
    asm_->mov(tmp, target);
    asm_->and(tmp, -9);
    asm_->cmp(tmp, detail::jsval64::kNull);
    asm_->je(label, type);
  }

  void CheckObjectCoercible(register_t reg,
                            const Xbyak::Reg64& target,
                            const Xbyak::Reg64& tmp) {
    static const char* message = "null or undefined has no properties";
    const TypeEntry type = type_record_.Get(reg);
    if (type.type().IsNotUndefined() && type.type().IsNotNull()) {
      // no check
      return;
    }

    // (1000)2 = 8
    // Null is (0010)2 and Undefined is (1010)2
    // ~UINT64_C(8) value is -9
    const Assembler::LocalLabelScope scope(asm_);
    asm_->mov(tmp, target);
    asm_->and(tmp, -9);
    asm_->cmp(tmp, detail::jsval64::kNull);
    asm_->jne(".EXIT");
    asm_->mov(rdi, r14);
    asm_->mov(rsi, Error::Type);
    asm_->mov(rdx, core::BitCast<uint64_t>(message));
    asm_->Call(&stub::THROW_WITH_TYPE_AND_MESSAGE);
    asm_->L(".EXIT");
  }

  void DenseArrayGuard(register_t base,
                       const Xbyak::Reg64& target,
                       const Xbyak::Reg64& tmp,
                       const Xbyak::Reg64& tmp2,
                       bool store_check,
                       const char* label,
                       Xbyak::CodeGenerator::LabelType type = Xbyak::CodeGenerator::T_AUTO) {
    static_assert(core::kLittleEndian, "System should be little endianess");

    const TypeEntry type_entry = type_record_.Get(base);

    if (!type_entry.type().IsSomeObject()) {
      // check target is Cell
      asm_->mov(tmp, detail::jsval64::kValueMask);
      asm_->test(tmp, target);
      asm_->jnz(label, type);
    }

    // target is guaranteed as cell
    // load Class tag from object and check it is Array
    if (!type_entry.type().IsArray()) {
      const std::ptrdiff_t offset = IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ClassOffset();
      asm_->mov(tmp, qword[target + offset]);
      asm_->test(tmp, tmp);  // class is nullptr => String...
      asm_->jz(label, type);

      const std::ptrdiff_t get_method = Class::MethodTableOffset() + IV_OFFSETOF(MethodTable, GetOwnIndexedPropertySlot);
      // tmp is class
      asm_->mov(tmp2, core::BitCast<uintptr_t>(&JSObject::GetOwnIndexedPropertySlotMethod));
      asm_->cmp(qword[tmp + get_method], tmp2);
      asm_->jne(label, type);
      if (store_check) {
        const std::ptrdiff_t store_method = Class::MethodTableOffset() + IV_OFFSETOF(MethodTable, DefineOwnIndexedPropertySlot);
        asm_->mov(tmp2, core::BitCast<uintptr_t>(&JSObject::DefineOwnIndexedPropertySlotMethod));
        asm_->cmp(qword[tmp + store_method], tmp2);
        asm_->jne(label, type);
      }
    }
  }

  void EmitConstantDest(const TypeEntry& entry, register_t dst) {
    asm_->mov(rax, Extract(entry.constant()));
    asm_->mov(qword[r13 + dst * kJSValSize], rax);
    set_last_used_candidate(dst);
  }

  std::string MakeLabel(const Instruction* op_instr) const {
    const int32_t jump = op_instr[1].jump.to;
    const uint32_t to = (op_instr - code_->begin()) + jump;
    const std::size_t num = jump_map_.find(to)->second.counter;
    return MakeLabel(num);
  }

  static std::string MakeLabel(std::size_t num) {
    std::string str("IV_LV5_BREAKER_JT_");
    str.reserve(str.size() + 10);
    core::detail::UIntToStringWithRadix<uint64_t>(num,
                                                  std::back_inserter(str), 32);
    return str;
  }

  void Jump(const Instruction* instr) {
    assert(OP::IsJump(instr->GetOP()));
    const std::string label = MakeLabel(instr);
    asm_->jmp(label.c_str(), Xbyak::CodeGenerator::T_NEAR);
  }

  inline const Instruction* previous_instr() const { return previous_instr_; }

  inline void set_previous_instr(const Instruction* instr) {
    previous_instr_ = instr;
  }

  inline int32_t last_used() const { return last_used_; }

  inline void set_last_used(int32_t reg) {
    last_used_ = reg;
  }

  inline int32_t last_used_candidate() const { return last_used_candidate_; }

  inline void set_last_used_candidate(int32_t reg) {
    last_used_candidate_ = reg;
  }

  inline void kill_last_used() {
    set_last_used(kInvalidUsedOffset);
  }

  Context* ctx() const { return ctx_; }

  NativeCode* native_code() const { return native_code_; }

  void LookupHeapEnv(const Xbyak::Reg64& target, uint32_t nest) {
    for (uint32_t i = 0; i < nest; ++i) {
      asm_->mov(target, ptr[target + JSEnv::OuterOffset()]);
    }
  }

  Context* ctx_;
  railgun::Code* top_;
  railgun::Code* code_;
  Assembler* asm_;
  NativeCode* native_code_;
  JumpMap jump_map_;
  EntryPointMap entry_points_;
  UnresolvedAddressMap unresolved_address_map_;
  HandlerLinks handler_links_;
  Codes codes_;
  std::size_t counter_;
  const Instruction* previous_instr_;
  int32_t last_used_;
  int32_t last_used_candidate_;
  TypeRecord type_record_;
};

inline void CompileInternal(Compiler* compiler, railgun::Code* code) {
  compiler->Compile(code);
  for (railgun::Code* sub : code->codes()) {
    CompileInternal(compiler, sub);
  }
}

inline void Compile(Context* ctx, railgun::Code* code) {
  Compiler compiler(ctx, code);
  CompileInternal(&compiler, code);
}

// external interfaces
inline railgun::Code* CompileGlobal(
    Context* ctx,
    const FunctionLiteral& global, railgun::JSScript* script) {
  railgun::Code* code = railgun::CompileGlobal(ctx, global, script, true);
  if (code) {
    Compile(ctx, code);
  }
  return code;
}

inline railgun::Code* CompileFunction(
    Context* ctx,
    const FunctionLiteral& func, railgun::JSScript* script) {
  railgun::Code* code = railgun::CompileFunction(ctx, func, script, true);
  if (code) {
    Compile(ctx, code);
  }
  return code;
}

inline railgun::Code* CompileEval(
    Context* ctx,
    const FunctionLiteral& eval, railgun::JSScript* script) {
  railgun::Code* code = railgun::CompileEval(ctx, eval, script, true);
  if (code) {
    Compile(ctx, code);
  }
  return code;
}

inline railgun::Code* CompileIndirectEval(
    Context* ctx,
    const FunctionLiteral& eval,
    railgun::JSScript* script) {
  railgun::Code* code = railgun::CompileIndirectEval(ctx, eval, script, true);
  if (code) {
    Compile(ctx, code);
  }
  return code;
}

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_H_
