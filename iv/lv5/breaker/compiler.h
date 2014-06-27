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

  struct JumpInfo {
    JumpInfo()
      : exception_handled(false),
        label() {
    }

    JumpInfo(bool handled)
      : exception_handled(handled),
        label() {
    }

    bool exception_handled;
    Xbyak::Label label;
  };

  typedef std::unordered_map<railgun::Code*, std::size_t> EntryPointMap;
  typedef std::unordered_map<uint32_t, JumpInfo> JumpMap;
  typedef std::unordered_map<std::size_t,
                             Assembler::RepatchSite> UnresolvedAddressMap;
  typedef std::vector<railgun::Code*> Codes;
  typedef std::unordered_map<const Instruction*, std::size_t> HandlerLinks;

  static const int kJSValSize = sizeof(JSVal);

  static const int32_t kInvalidUsedOffset = INT32_MIN;

  explicit Compiler(Context* ctx, railgun::Code* top);

  static uint64_t RotateLeft64(uint64_t val, uint64_t amount) {
    return (val << amount) | (val >> (64 - amount));
  }

  ~Compiler();

  void Initialize(railgun::Code* code);

  void Compile(railgun::Code* code);

  // scan jump target for setting labels
  void ScanJumps();

  // Returns previous and instr are in basic block
  bool SplitBasicBlock(const Instruction* previous, const Instruction* instr);

  void Main();

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
  void LoadVR(const Xbyak::Reg64& out, register_t offset);

  void LoadVRs(const Xbyak::Reg64& out1, register_t offset1,
               const Xbyak::Reg64& out2, register_t offset2);

  // opcode
  void EmitNOP(const Instruction* instr);

  // opcode
  void EmitENTER(const Instruction* instr);

  // opcode | (dst | src)
  void EmitMV(const Instruction* instr);

  // opcode | (size | mutable_start)
  void EmitBUILD_ENV(const Instruction* instr);

  // opcode | src
  void EmitWITH_SETUP(const Instruction* instr);

  // opcode | (jmp | flag)
  void EmitRETURN_SUBROUTINE(const Instruction* instr);

  // opcode | (dst | offset)
  void EmitLOAD_CONST(const Instruction* instr);

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
  void EmitBINARY_INSTANCEOF(const Instruction* instr,
                             OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_IN(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_EQ(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_STRICT_EQ(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_NE(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_STRICT_NE(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_AND(const Instruction* instr, OP::Type fused = OP::NOP);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_XOR(const Instruction* instr);

  // opcode | (dst | lhs | rhs)
  void EmitBINARY_BIT_OR(const Instruction* instr);

  // opcode | (dst | src)
  void EmitUNARY_POSITIVE(const Instruction* instr);

  // opcode | (dst | src)
  void EmitUNARY_NEGATIVE(const Instruction* instr);

  // opcode | (dst | src)
  void EmitUNARY_NOT(const Instruction* instr);

  // opcode | (dst | src)
  void EmitUNARY_BIT_NOT(const Instruction* instr);

  // opcode | src
  void EmitTHROW(const Instruction* instr);

  // opcode
  void EmitDEBUGGER(const Instruction* instr);

  // opcode | src
  void EmitTO_NUMBER(const Instruction* instr);

  // opcode | src
  void EmitTO_PRIMITIVE_AND_TO_STRING(const Instruction* instr);

  // opcode | (dst | start | count)
  void EmitCONCAT(const Instruction* instr);

  // opcode name
  void EmitRAISE(const Instruction* instr);

  // opcode | (dst | src)
  void EmitTYPEOF(const Instruction* instr);

  // opcode | (obj | item) | (index | type)
  void EmitSTORE_OBJECT_INDEXED(const Instruction* instr);

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_DATA(const Instruction* instr);

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_GET(const Instruction* instr);

  // opcode | (obj | item) | (offset | merged)
  void EmitSTORE_OBJECT_SET(const Instruction* instr);

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitLOAD_PROP(const Instruction* instr);

  // opcode | (base | src | index) | nop | nop
  void EmitSTORE_PROP(const Instruction* instr);

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitDELETE_PROP(const Instruction* instr);

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitINCREMENT_PROP(const Instruction* instr);

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitDECREMENT_PROP(const Instruction* instr);

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitPOSTFIX_INCREMENT_PROP(const Instruction* instr);

  // opcode | (dst | base | name) | nop | nop | nop
  void EmitPOSTFIX_DECREMENT_PROP(const Instruction* instr);

  // opcode
  void EmitPOP_ENV(const Instruction* instr);

  // opcode | (ary | reg) | (index | size)
  void EmitINIT_VECTOR_ARRAY_ELEMENT(const Instruction* instr);

  // opcode | (ary | reg) | (index | size)
  void EmitINIT_SPARSE_ARRAY_ELEMENT(const Instruction* instr);

  // opcode | (dst | size)
  void EmitLOAD_ARRAY(const Instruction* instr);

  // opcode | (dst | offset)
  void EmitDUP_ARRAY(const Instruction* instr);

  // opcode | (dst | code)
  void EmitLOAD_FUNCTION(const Instruction* instr);

  // opcode | (dst | offset)
  void EmitLOAD_REGEXP(const Instruction* instr);

  // opcode | dst | map
  void EmitLOAD_OBJECT(const Instruction* instr);

  // opcode | (dst | base | element)
  void EmitLOAD_ELEMENT(const Instruction* instr);

  // opcode | (base | element | src)
  void EmitSTORE_ELEMENT(const Instruction* instr);

  // opcode | (dst | base | element)
  void EmitDELETE_ELEMENT(const Instruction* instr);

  // opcode | (dst | base | element)
  void EmitINCREMENT_ELEMENT(const Instruction* instr);

  // opcode | (dst | base | element)
  void EmitDECREMENT_ELEMENT(const Instruction* instr);

  // opcode | (dst | base | element)
  void EmitPOSTFIX_INCREMENT_ELEMENT(const Instruction* instr);

  // opcode | (dst | base | element)
  void EmitPOSTFIX_DECREMENT_ELEMENT(const Instruction* instr);

  // opcode | dst
  void EmitRESULT(const Instruction* instr);

  // opcode | src
  void EmitRETURN(const Instruction* instr);

  // opcode | (dst | index) | nop | nop
  void EmitLOAD_GLOBAL(const Instruction* instr);

  // opcode | (src | name) | nop | nop
  void EmitSTORE_GLOBAL(const Instruction* instr);

  // opcode | dst | slot
  void EmitLOAD_GLOBAL_DIRECT(const Instruction* instr);

  // opcode | src | slot
  void EmitSTORE_GLOBAL_DIRECT(const Instruction* instr);

  // opcode | (dst | name) | nop | nop
  void EmitDELETE_GLOBAL(const Instruction* instr);

  // opcode | (dst | name) | nop | nop
  void EmitTYPEOF_GLOBAL(const Instruction* instr);

  // opcode | (dst | imm | index) | (offset | nest)
  void EmitLOAD_HEAP(const Instruction* instr);

  // opcode | (src | imm | name) | (offset | nest)
  void EmitSTORE_HEAP(const Instruction* instr);

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitDELETE_HEAP(const Instruction* instr);

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitINCREMENT_HEAP(const Instruction* instr);

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitDECREMENT_HEAP(const Instruction* instr);

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitPOSTFIX_INCREMENT_HEAP(const Instruction* instr);

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitPOSTFIX_DECREMENT_HEAP(const Instruction* instr);

  // opcode | (dst | imm | name) | (offset | nest)
  void EmitTYPEOF_HEAP(const Instruction* instr);

  // opcode | (callee | offset | argc_with_this)
  void EmitCALL(const Instruction* instr);

  // opcode | (callee | offset | argc_with_this)
  void EmitCONSTRUCT(const Instruction* instr);

  // opcode | (callee | offset | argc_with_this)
  void EmitEVAL(const Instruction* instr);

  // opcode | (name | configurable)
  void EmitINSTANTIATE_DECLARATION_BINDING(const Instruction* instr);

  // opcode | (name | configurable)
  void EmitINSTANTIATE_VARIABLE_BINDING(const Instruction* instr);

  // opcode | (src | offset)
  void EmitINITIALIZE_HEAP_IMMUTABLE(const Instruction* instr);

  // opcode | src
  void EmitINCREMENT(const Instruction* instr);

  // opcode | src
  void EmitDECREMENT(const Instruction* instr);

  // opcode | (dst | src)
  void EmitPOSTFIX_INCREMENT(const Instruction* instr);

  // opcode | (dst | src)
  void EmitPOSTFIX_DECREMENT(const Instruction* instr);

  // opcode | (dst | basedst | name)
  void EmitPREPARE_DYNAMIC_CALL(const Instruction* instr);

  // opcode | (jmp | cond)
  void EmitIF_FALSE(const Instruction* instr);

  // opcode | (jmp | cond)
  void EmitIF_TRUE(const Instruction* instr);

  // opcode | (jmp : addr | flag)
  void EmitJUMP_SUBROUTINE(const Instruction* instr);

  // opcode | (jmp : iterator | enumerable)
  void EmitFORIN_SETUP(const Instruction* instr);

  // opcode | (jmp : dst | iterator)
  void EmitFORIN_ENUMERATE(const Instruction* instr);

  // opcode | iterator
  void EmitFORIN_LEAVE(const Instruction* instr);

  // opcode | (error | name)
  void EmitTRY_CATCH_SETUP(const Instruction* instr);

  // opcode | (dst | index)
  void EmitLOAD_NAME(const Instruction* instr);

  // opcode | (src | name)
  void EmitSTORE_NAME(const Instruction* instr);

  // opcode | (dst | name)
  void EmitDELETE_NAME(const Instruction* instr);

  // opcode | (dst | name)
  void EmitINCREMENT_NAME(const Instruction* instr);

  // opcode | (dst | name)
  void EmitDECREMENT_NAME(const Instruction* instr);

  // opcode | (dst | name)
  void EmitPOSTFIX_INCREMENT_NAME(const Instruction* instr);

  // opcode | (dst | name)
  void EmitPOSTFIX_DECREMENT_NAME(const Instruction* instr);

  // opcode | (dst | name)
  void EmitTYPEOF_NAME(const Instruction* instr);

  // opcode | jmp
  void EmitJUMP_BY(const Instruction* instr);

  // opcode | dst
  void EmitLOAD_ARGUMENTS(const Instruction* instr);

  void ConvertDoubleToJSVal(const Xbyak::Reg64& target);

  void ConvertBooleanToJSVal(const Xbyak::Reg8& cond,
                             const Xbyak::Reg64& result);

  void Int32Guard(
      register_t reg,
      const Xbyak::Reg64& target,
      const std::string& label,
      Xbyak::CodeGenerator::LabelType near = Xbyak::CodeGenerator::T_AUTO);

  void NumberGuard(
      register_t reg,
      const Xbyak::Reg64& target,
      const Xbyak::Label* bailout,
      Xbyak::CodeGenerator::LabelType near = Xbyak::CodeGenerator::T_AUTO);

  void LoadCellTag(const Xbyak::Reg64& target, const Xbyak::Reg32& out);

  void CompareCellTag(const Xbyak::Reg64& target, uint8_t tag);

  void LoadClassTag(
      const Xbyak::Reg64& target,
      const Xbyak::Reg64& tmp,
      const Xbyak::Reg32& out);

  void EmptyGuard(
      const Xbyak::Reg64& target,
      const std::string& label,
      Xbyak::CodeGenerator::LabelType near = Xbyak::CodeGenerator::T_AUTO);

  void NotEmptyGuard(
      const Xbyak::Reg64& target,
      const std::string& label,
      Xbyak::CodeGenerator::LabelType near = Xbyak::CodeGenerator::T_AUTO);

  void NotNullOrUndefinedGuard(
      const Xbyak::Reg64& target,
      const Xbyak::Reg64& tmp,
      const Xbyak::Label* label,
      Xbyak::CodeGenerator::LabelType near = Xbyak::CodeGenerator::T_AUTO);

  void CheckObjectCoercible(register_t reg,
                            const Xbyak::Reg64& target,
                            const Xbyak::Reg64& tmp);

  void DenseArrayGuard(
      register_t base,
      const Xbyak::Reg64& target,
      const Xbyak::Reg64& tmp,
      const Xbyak::Reg64& tmp2,
      bool store_check,
      const std::string& label,
      Xbyak::CodeGenerator::LabelType near = Xbyak::CodeGenerator::T_AUTO);

  void EmitConstantDest(const TypeEntry& entry, register_t dst);

  Xbyak::Label& LookupLabel(const Instruction* op_instr);

  void Jump(const Instruction* instr);

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

  inline Context* ctx() const { return ctx_; }

  inline NativeCode* native_code() const { return native_code_; }

  void LookupHeapEnv(const Xbyak::Reg64& target, uint32_t nest);

  void LoadDouble(
      register_t reg,
      const Xbyak::Xmm& xmm,
      const Xbyak::Reg64& scratch,
      const Xbyak::Label* bailout,
      Xbyak::CodeGenerator::LabelType near = Xbyak::CodeGenerator::T_NEAR);

  void BoxDouble(const Xbyak::Xmm& src,
                 const Xbyak::Xmm& scratch,
                 const Xbyak::Reg64& dst64,
                 const Xbyak::Label* exit);

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
  const Instruction* previous_instr_;
  int32_t last_used_;
  int32_t last_used_candidate_;
  TypeRecord type_record_;
};

// external interfaces
void Compile(Context* ctx, railgun::Code* code);

railgun::Code* CompileGlobal(
    Context* ctx,
    const FunctionLiteral& global, railgun::JSScript* script);

railgun::Code* CompileFunction(
    Context* ctx,
    const FunctionLiteral& func, railgun::JSScript* script);

railgun::Code* CompileEval(
    Context* ctx,
    const FunctionLiteral& eval, railgun::JSScript* script);

railgun::Code* CompileIndirectEval(
    Context* ctx,
    const FunctionLiteral& eval,
    railgun::JSScript* script);

} } }  // namespace iv::lv5::breaker
#endif  // IV_LV5_BREAKER_COMPILER_H_
