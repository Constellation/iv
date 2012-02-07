#ifndef IV_LV5_RAILGUN_COMPILER_H_
#define IV_LV5_RAILGUN_COMPILER_H_
#include <algorithm>
#include <gc/gc.h>
#include <iv/detail/tuple.h>
#include <iv/detail/unordered_map.h>
#include <iv/detail/memory.h>
#include <iv/utils.h>
#include <iv/ast_visitor.h>
#include <iv/noncopyable.h>
#include <iv/conversions.h>
#include <iv/static_assert.h>
#include <iv/enable_if.h>
#include <iv/unicode.h>
#include <iv/utils.h>
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsstring.h>
#include <iv/lv5/jsregexp.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/context_utils.h>
#include <iv/lv5/utility.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/thunk_fwd.h>
#include <iv/lv5/railgun/register_id.h>
#include <iv/lv5/railgun/instruction_fwd.h>
#include <iv/lv5/railgun/core_data.h>
#include <iv/lv5/railgun/code.h>
#include <iv/lv5/railgun/scope.h>
#include <iv/lv5/railgun/condition.h>
#include <iv/lv5/railgun/analyzer.h>
#include <iv/lv5/railgun/continuation_status.h>
#include <iv/lv5/railgun/direct_threading.h>

namespace iv {
namespace lv5 {
namespace railgun {

class Compiler : private core::Noncopyable<Compiler>, public AstVisitor {
 public:
  typedef std::tuple<uint16_t,
                     std::vector<std::size_t>*,
                     std::vector<std::size_t>*> JumpEntry;
  typedef std::unordered_map<const BreakableStatement*, JumpEntry> JumpTable;
  typedef std::tuple<Code*,
                     const FunctionLiteral*,
                     std::shared_ptr<VariableScope> > CodeInfo;
  typedef std::vector<CodeInfo> CodeInfoStack;

  enum LevelType {
    WITH,
    FINALLY
  };

  typedef std::tuple<LevelType,
                     std::vector<std::size_t>*,
                     RegisterID, RegisterID, RegisterID> LevelEntry;
  typedef std::vector<LevelEntry> LevelStack;

  typedef std::unordered_map<
      double,
      int32_t,
      std::hash<double>, JSDoubleEquals> JSDoubleToIndexMap;

  typedef std::unordered_map<core::UString, int32_t> JSStringToIndexMap;

  friend class ThunkList;

  explicit Compiler(Context* ctx)
    : ctx_(ctx),
      code_(NULL),
      core_(NULL),
      data_(NULL),
      script_(NULL),
      code_info_stack_(),
      jump_table_(),
      level_stack_(),
      registers_(),
      thunklist_(this),
      ignore_result_(false),
      dst_(),
      eval_result_(),
      symbol_to_index_map_(),
      jsstring_to_index_map_(),
      double_to_index_map_(),
      dynamic_env_level_(),
      continuation_status_(),
      current_variable_scope_(),
      temporary_() {
  }

  Code* Compile(const FunctionLiteral& global, JSScript* script) {
    Code* code = NULL;
    {
      script_ = script;
      core_ = CoreData::New();
      data_ = core_->data();
      data_->reserve(4 * core::Size::KB);
      code = new Code(ctx_, script_, global, core_, Code::GLOBAL);
      EmitFunctionCode<Code::GLOBAL>(global, code,
                                     std::shared_ptr<VariableScope>());
    }
    CompileEpilogue(code);
    return code;
  }

  Code* CompileFunction(const FunctionLiteral& function, JSScript* script) {
    Code* code = NULL;
    {
      script_ = script;
      core_ = CoreData::New();
      data_ = core_->data();
      data_->reserve(core::Size::KB);
      current_variable_scope_ =
          std::shared_ptr<VariableScope>(new GlobalScope());
      code = new Code(ctx_, script_, function, core_, Code::FUNCTION);
      EmitFunctionCode<Code::FUNCTION>(function, code, current_variable_scope_);
      current_variable_scope_ = current_variable_scope_->upper();
    }
    CompileEpilogue(code);
    return code;
  }

  Code* CompileEval(const FunctionLiteral& eval, JSScript* script) {
    Code* code = NULL;
    {
      script_ = script;
      core_ = CoreData::New();
      data_ = core_->data();
      data_->reserve(core::Size::KB);
      code = new Code(ctx_, script_, eval, core_, Code::EVAL);
      if (eval.strict()) {
        current_variable_scope_ =
            std::shared_ptr<VariableScope>(new EvalScope());
        EmitFunctionCode<Code::FUNCTION>(eval, code,
                                         current_variable_scope_, true);
        current_variable_scope_ = current_variable_scope_->upper();
      } else {
        current_variable_scope_ = std::shared_ptr<VariableScope>();
        EmitFunctionCode<Code::EVAL>(eval, code, current_variable_scope_);
      }
    }
    CompileEpilogue(code);
    return code;
  }

  Code* CompileIndirectEval(const FunctionLiteral& eval, JSScript* script) {
    Code* code = NULL;
    {
      script_ = script;
      core_ = CoreData::New();
      data_ = core_->data();
      data_->reserve(core::Size::KB);
      code = new Code(ctx_, script_, eval, core_, Code::EVAL);
      if (eval.strict()) {
        current_variable_scope_ =
            std::shared_ptr<VariableScope>(new GlobalScope());
        EmitFunctionCode<Code::FUNCTION>(eval,
                                         code, current_variable_scope_, true);
        current_variable_scope_ = current_variable_scope_->upper();
      } else {
        current_variable_scope_ = std::shared_ptr<VariableScope>();
        EmitFunctionCode<Code::EVAL>(eval, code, current_variable_scope_);
      }
    }
    CompileEpilogue(code);
    return code;
  }

 private:
  class BreakTarget : private core::Noncopyable<> {
   public:
    BreakTarget(Compiler* compiler,
                const BreakableStatement* stmt)
      : compiler_(compiler),
        stmt_(stmt),
        breaks_() {
      compiler_->RegisterJumpTarget(stmt, &breaks_);
    }

    ~BreakTarget() {
      compiler_->UnRegisterJumpTarget(stmt_);
    }

    void EmitJumps(std::size_t break_target) {
      for (std::vector<std::size_t>::const_iterator it = breaks_.begin(),
           last = breaks_.end(); it != last; ++it) {
        compiler_->EmitJump(break_target, *it);
      }
    }

   protected:
    explicit BreakTarget(Compiler* compiler)
      : compiler_(compiler),
        breaks_() {
    }

    Compiler* compiler_;
    const BreakableStatement* stmt_;
    std::vector<std::size_t> breaks_;
  };

  class ContinueTarget : protected BreakTarget {
   public:
    ContinueTarget(Compiler* compiler,
                   const IterationStatement* stmt)
      : BreakTarget(compiler),
        continues_() {
      stmt_ = stmt;
      compiler_->RegisterJumpTarget(stmt, &breaks_, &continues_);
    }

    void EmitJumps(std::size_t break_target,
                   std::size_t continue_target) {
      BreakTarget::EmitJumps(break_target);
      for (std::vector<std::size_t>::const_iterator it = continues_.begin(),
           last = continues_.end(); it != last; ++it) {
        compiler_->EmitJump(continue_target, *it);
      }
    }

   private:
    std::vector<std::size_t> continues_;
  };

  class TryTarget : private core::Noncopyable<> {
   public:
    TryTarget(Compiler* compiler, bool has_finally)
      : compiler_(compiler),
        has_finally_(has_finally),
        vec_() {
      if (has_finally_) {
        compiler_->PushLevelFinally(&vec_);
      }
    }

    void EmitJumps(std::size_t finally_target) {
      assert(has_finally_);
      const LevelStack& stack = compiler_->level_stack();
      const LevelEntry& entry = stack.back();
      assert(std::get<0>(entry) == FINALLY);
      assert(std::get<1>(entry) == &vec_);
      const std::vector<std::size_t>* vec = std::get<1>(entry);
      for (std::vector<std::size_t>::const_iterator it = vec->begin(),
           last = vec->end(); it != last; ++it) {
        compiler_->EmitJump(finally_target, *it);
      }
      compiler_->PopLevel();
    }

   private:
    Compiler* compiler_;
    bool has_finally_;
    std::vector<std::size_t> vec_;
  };

  class DynamicEnvLevelCounter : private core::Noncopyable<> {
   public:
    explicit DynamicEnvLevelCounter(Compiler* compiler)
      : compiler_(compiler) {
      compiler_->DynamicEnvLevelUp();
    }
    ~DynamicEnvLevelCounter() {
      compiler_->DynamicEnvLevelDown();
    }
   private:
    Compiler* compiler_;
  };

  void CompileEpilogue(Code* code) {
    // optimiazation or direct threading
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
    // direct threading label translation
    const DirectThreadingDispatchTable& table = VM::DispatchTable();
    Code::Data* data = code->GetData();
    for (Code::Data::iterator it = data->begin(),
         last = data->end(); it != last;) {
      const uint32_t opcode = it->value;
      it->label = table[opcode];
      std::advance(it, kOPLength[opcode]);
    }
#endif
    core_->SetCompiled();
    ctx_->global_data()->RegExpClear();
  }

  void CodeContextPrologue(Code* code) {
    set_code(code);
    set_dynamic_env_level(0);
    jump_table_.clear();
    level_stack_.clear();
    dst_.reset();
    eval_result_.reset();
    symbol_to_index_map_.clear();
    jsstring_to_index_map_.clear();
    double_to_index_map_.clear();
    continuation_status_.Clear();
    code->set_start(data_->size());
  }

  void CodeContextEpilogue(Code* code) {
    code->set_end(data_->size());
    code->set_temporary_registers(registers_.size());
  }

  // Statement

  void EmitStatement(const Statement* stmt) {
    stmt->Accept(this);
  }

  RegisterID EmitLevel(uint16_t from, uint16_t to,
                       RegisterID dst = RegisterID()) {
    for (; from > to; --from) {
      const LevelEntry& entry = level_stack_[from - 1];
      const LevelType type = std::get<0>(entry);
      if (type == FINALLY) {
        if (dst) {
          dst = EmitMV(std::get<3>(entry), dst);
        }
        const std::size_t finally_jump = CurrentSize();
        Emit<OP::JUMP_SUBROUTINE>(0, std::get<2>(entry), std::get<4>(entry));
        std::get<1>(entry)->push_back(finally_jump);
      } else {  // type == WITH
        assert(type == WITH);
        Emit<OP::POP_ENV>();
      }
    }
    return dst;
  }

  void Visit(const Block* block);
  void Visit(const FunctionStatement* stmt);
  void Visit(const FunctionDeclaration* func);
  void Visit(const Declaration* decl);
  void Visit(const VariableStatement* var);
  void Visit(const EmptyStatement* stmt);
  void Visit(const IfStatement* stmt);
  void Visit(const DoWhileStatement* stmt);
  void Visit(const WhileStatement* stmt);
  void Visit(const ForStatement* stmt);
  void Visit(const ForInStatement* stmt);
  void Visit(const ContinueStatement* stmt);
  void Visit(const BreakStatement* stmt);
  void Visit(const ReturnStatement* stmt);
  void Visit(const WithStatement* stmt);
  void Visit(const LabelledStatement* stmt);
  void Visit(const CaseClause* dummy);
  void Visit(const SwitchStatement* stmt);
  void Visit(const ThrowStatement* stmt);
  void Visit(const TryStatement* stmt);
  void Visit(const DebuggerStatement* stmt);
  void Visit(const ExpressionStatement* stmt);

  // Expression
  // implementation is in compiler_expression.h

  void Visit(const Assignment* assign);
  void Visit(const BinaryOperation* binary);
  void Visit(const ConditionalExpression* cond);
  void Visit(const UnaryOperation* unary);
  void Visit(const PostfixExpression* postfix);
  void Visit(const Assigned* assigned);
  void Visit(const StringLiteral* literal);
  void Visit(const NumberLiteral* literal);
  void Visit(const Identifier* literal);
  void Visit(const ThisLiteral* literal);
  void Visit(const NullLiteral* literal);
  void Visit(const TrueLiteral* literal);
  void Visit(const FalseLiteral* literal);
  void Visit(const RegExpLiteral* literal);
  void Visit(const ArrayLiteral* literal);
  void Visit(const ObjectLiteral* literal);
  void Visit(const FunctionLiteral* literal);
  void Visit(const IdentifierAccess* prop);
  void Visit(const IndexAccess* prop);
  void Visit(const FunctionCall* call);
  void Visit(const ConstructorCall* call);

  class EmitExpressionContext {
   public:
    EmitExpressionContext(Compiler* compiler,
                          RegisterID dst,
                          bool ignore_result)
      : compiler_(compiler),
        dst_(compiler->dst()),
        ignore_result_(compiler->ignore_result()) {
      compiler->set_dst(dst);
      compiler->set_ignore_result(ignore_result);
    }

    ~EmitExpressionContext() {
      compiler_->set_dst(dst_);
      compiler_->set_ignore_result(ignore_result_);
    }

    RegisterID dst() const { return dst_; }

   private:
    Compiler* compiler_;
    RegisterID dst_;
    bool ignore_result_;
  };

  class DestGuard {
   public:
    explicit DestGuard(Compiler* compiler)
      : compiler_(compiler) {
    }
    ~DestGuard() {
      assert(compiler_->dst());
    }
   private:
    Compiler* compiler_;
  };

  RegisterID EmitExpression(const Expression* expr,
                            RegisterID dst,
                            bool ignore_result) {
    EmitExpressionContext context(this, dst, ignore_result);
    expr->Accept(this);
    return dst_;
  }

  RegisterID EmitExpression(const Expression* expr) {
    return EmitExpression(expr, RegisterID(), false);
  }

  // force write
  RegisterID EmitExpression(const Expression* expr, RegisterID dst) {
    return EmitExpression(expr, dst, false);
  }

  LookupInfo Lookup(const Symbol sym) {
    return current_variable_scope_->Lookup(sym);
  }

  uint32_t SymbolToNameIndex(const Symbol& sym) {
    const std::unordered_map<Symbol, uint32_t>::const_iterator it =
        symbol_to_index_map_.find(sym);
    if (it != symbol_to_index_map_.end()) {
      // found, so return this index
      return it->second;
    }
    // not found, so push_back it to code
    const uint32_t index = code_->names_.size();
    symbol_to_index_map_[sym] = index;
    code_->names_.push_back(sym);
    assert(symbol_to_index_map_.size() == code_->names_.size());
    return index;
  }

  bool IsUsedReference(uint32_t index) {
    return Lookup(code_->names_[index]).type() != LookupInfo::UNUSED;
  }

  template<OP::Type PropOP, OP::Type ElementOP>
  RegisterID EmitElement(const IndexAccess* prop, RegisterID dst) {
    Thunk base(&thunklist_, EmitExpression(prop->target()));
    const Expression* key = prop->key();
    if (const StringLiteral* str = key->AsStringLiteral()) {
      const uint32_t index =
          SymbolToNameIndex(context::Intern(ctx_, str->value()));
      dst = Dest(dst, base.Release());
      thunklist_.Spill(dst);
      Emit<PropOP>(dst, base.reg(), index, 0, 0, 0, 0);
    } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
      const uint32_t index =
          SymbolToNameIndex(context::Intern(ctx_, num->value()));
      dst = Dest(dst, base.Release());
      thunklist_.Spill(dst);
      Emit<PropOP>(dst, base.reg(), index, 0, 0, 0, 0);
    } else {
      RegisterID element = EmitExpression(prop->key());
      dst = Dest(dst, base.Release(), element);
      thunklist_.Spill(dst);
      Emit<ElementOP>(dst, base.reg(), element);
    }
    return dst;
  }

  // Allocate Register if
  //   f(1, 2, 3)
  // is called, allocate like
  //   r3[this]
  //   r2[3]
  //   r1[2]
  //   r0[1]
  template<typename Call>
  class CallSite {
   public:
    explicit CallSite(const Call& call,
                      ThunkList* thunklist,
                      Registers* registers)
      : call_(call),
        callee_(),
        args_(argc_with_this()),
        start_(),
        registers_(registers) {
      // spill all thunks
      if (Analyzer::ExpressionHasAssignment(&call)) {
        thunklist->ForceSpill();
      }
      callee_ = registers->Acquire();
      start_ = registers->AcquireCallBase(argc_with_this());
    }

    int argc_with_this() const { return call_.args().size() + 1; }

    RegisterID GetFirstPosition() {
      return Arg(static_cast<int>(call_.args().size()) - 1);
    }

    RegisterID Arg(int i) {
        const int target = argc_with_this() - i - 2;
        if (!args_[target]) {
          args_[target] = registers_->Acquire(start_ + target);
        }
        return args_[target];
    }

    void EmitArguments(Compiler* compiler) {
      const Expressions& args = call_.args();
      {
        int i = 0;
        for (Expressions::const_iterator it = args.begin(),
             last = args.end(); it != last; ++it, ++i) {
          compiler->EmitExpression(*it, Arg(i));
        }
      }
    }

    RegisterID base() {
      return Arg(-1);
    }

    RegisterID callee() const {
      return callee_;
    }

   private:
    const Call& call_;
    RegisterID callee_;
    std::vector<RegisterID> args_;
    int32_t start_;
    Registers* registers_;
  };

  template<OP::Type op, typename Call>
  RegisterID EmitCall(const Call& call, RegisterID dst) {
    bool direct_call_to_eval = false;
    const Expression* target = call.target();
    CallSite<Call> site(call, &thunklist_, &registers_);

    if (target->IsValidLeftHandSide()) {
      if (const Identifier* ident = target->AsIdentifier()) {
        if (RegisterID local = GetLocal(ident->symbol())) {
          Emit<OP::MV>(site.callee(), local);
          Emit<OP::LOAD_UNDEFINED>(site.base());
        } else {
          // lookup dynamic call or not
          {
            const LookupInfo info = Lookup(ident->symbol());
            const uint32_t index = SymbolToNameIndex(ident->symbol());
            assert(info.type() != LookupInfo::STACK);
            if (info.type() == LookupInfo::LOOKUP) {
              Emit<OP::PREPARE_DYNAMIC_CALL>(site.callee(), site.base(), index);
            } else {
              EmitOptimizedLookup(OP::LOAD_NAME, index, site.callee());
              Emit<OP::LOAD_UNDEFINED>(site.base());
            }
          }
        }
        if (op == OP::CALL && ident->symbol() == symbol::eval()) {
          direct_call_to_eval = true;
        }
      } else if (const PropertyAccess* prop = target->AsPropertyAccess()) {
        if (const IdentifierAccess* ac = prop->AsIdentifierAccess()) {
          // IdentifierAccess
          EmitExpression(prop->target(), site.base());
          const uint32_t index = SymbolToNameIndex(ac->key());
          Emit<OP::LOAD_PROP>(site.callee(), site.base(), index, 0, 0, 0, 0);
        } else {
          // IndexAccess
          const IndexAccess* ai = prop->AsIndexAccess();
          EmitExpression(ai->target(), site.base());
          const Expression* key = ai->key();
          if (const StringLiteral* str = key->AsStringLiteral()) {
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, str->value()));
            Emit<OP::LOAD_PROP>(site.callee(), site.base(), index, 0, 0, 0, 0);
          } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, num->value()));
            Emit<OP::LOAD_PROP>(site.callee(), site.base(), index, 0, 0, 0, 0);
          } else {
            EmitExpression(ai->key(), site.callee());
            Emit<OP::LOAD_ELEMENT>(site.callee(), site.base(), site.callee());
          }
        }
      } else {
        EmitExpression(target, site.callee());
        Emit<OP::LOAD_UNDEFINED>(site.base());
      }
    } else {
      EmitExpression(target, site.callee());
      Emit<OP::LOAD_UNDEFINED>(site.base());
    }

    site.EmitArguments(this);

    dst = DestCallSite(dst, &site);
    thunklist_.Spill(dst);
    if (direct_call_to_eval) {
      Emit<OP::EVAL>(dst, site.callee(),
                     site.GetFirstPosition(),
                     static_cast<uint32_t>(site.argc_with_this()));
    } else {
      Emit<op>(dst, site.callee(),
               site.GetFirstPosition(),
               static_cast<uint32_t>(site.argc_with_this()));
    }
    assert(registers_.IsLiveTop(site.base()->reg()));
    return dst;
  }

  void EmitStore(Symbol sym, RegisterID src) {
    const uint32_t index = SymbolToNameIndex(sym);
    const LookupInfo info = Lookup(sym);
    const OP::Type op = OP::STORE_NAME;
    switch (info.type()) {
      case LookupInfo::HEAP: {
        Emit(OP::ToHeap(op), src, index, info.location(),
             current_variable_scope_->scope_nest_count() - info.scope());
        return;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        Emit(OP::ToGlobal(op), src, index, 0u, 0u);
        return;
      }
      case LookupInfo::LOOKUP: {
        Emit(op, src, index);
        return;
      }
      case LookupInfo::UNUSED: {
        return;
      }
      default: {
        UNREACHABLE();
      }
    }
  }

  RegisterID EmitOptimizedLookup(OP::Type op, uint32_t index, RegisterID dst) {
    dst = Dest(dst);
    const LookupInfo info = Lookup(code_->names_[index]);
    assert(info.type() != LookupInfo::STACK);
    switch (info.type()) {
      case LookupInfo::HEAP: {
        thunklist_.Spill(dst);
        Emit(OP::ToHeap(op),
             dst,
             index, info.location(),
             current_variable_scope_->scope_nest_count() - info.scope());
        return dst;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        thunklist_.Spill(dst);
        Emit(OP::ToGlobal(op), dst, index, 0u, 0u);
        return dst;
      }
      case LookupInfo::LOOKUP: {
        thunklist_.Spill(dst);
        Emit(op, dst, index);
        return dst;
      }
      case LookupInfo::UNUSED: {
        assert(op == OP::STORE_NAME);
        // do nothing
        return dst;
      }
      default: {
        UNREACHABLE();
      }
    }
    return dst;
  }

  // determine which register is used
  // if dst is not ignored, use dst.
  RegisterID Dest(RegisterID dst,
                  RegisterID cand1 = RegisterID(),
                  RegisterID cand2 = RegisterID()) {
    if (dst) {
      return dst;
    }

    if (cand1 && !cand1->IsLocal()) {
      return cand1;
    }

    if (cand2 && !cand2->IsLocal()) {
      return cand2;
    }
    return registers_.Acquire();
  }

  template<typename CallSite>
  RegisterID DestCallSite(RegisterID dst, CallSite* site) {
    if (dst) {
      return dst;
    }
    return site->callee();
  }

  RegisterID GetLocal(Symbol sym) {
    const LookupInfo info = Lookup(sym);
    if (info.type() == LookupInfo::STACK) {
      return registers_.LocalID(info.location());
    }
    return RegisterID();
  }

  RegisterID EmitMV(RegisterID to, RegisterID from) {
    if (!to) {
      return from;
    }
    if (to != from) {
      thunklist_.Spill(to);
      Emit<OP::MV>(to, from);
    }
    return to;
  }

  RegisterID SpillRegister(RegisterID from) {
    RegisterID to = registers_.Acquire();
    Emit<OP::MV>(to, from);
    return to;
  }

  // primitive emit

  template<OP::Type op>
  void Emit() {
    IV_STATIC_ASSERT(OPLength<op>::value == 1);
    data_->push_back(op);
  }

  template<OP::Type op>
  void Emit(Instruction arg) {
    IV_STATIC_ASSERT(OPLength<op>::value == 2);
    data_->push_back(op);
    data_->push_back(arg);
  }

  template<OP::Type op>
  void Emit(Instruction arg1, Instruction arg2) {
    IV_STATIC_ASSERT(OPLength<op>::value == 3);
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
  }

  template<OP::Type op>
  void Emit(Instruction arg1, Instruction arg2, Instruction arg3) {
    IV_STATIC_ASSERT(OPLength<op>::value == 4);
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
  }

  template<OP::Type op>
  void Emit(Instruction arg1, Instruction arg2,
            Instruction arg3, Instruction arg4) {
    IV_STATIC_ASSERT(OPLength<op>::value == 5);
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
    data_->push_back(arg4);
  }

  template<OP::Type op>
  void Emit(Instruction arg1, Instruction arg2,
            Instruction arg3, Instruction arg4, Instruction arg5) {
    IV_STATIC_ASSERT(OPLength<op>::value == 6);
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
    data_->push_back(arg4);
    data_->push_back(arg5);
  }

  template<OP::Type op>
  void Emit(Instruction arg1, Instruction arg2, Instruction arg3,
            Instruction arg4, Instruction arg5, Instruction arg6) {
    IV_STATIC_ASSERT(OPLength<op>::value == 7);
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
    data_->push_back(arg4);
    data_->push_back(arg5);
    data_->push_back(arg6);
  }

  template<OP::Type op>
  void Emit(Instruction arg1, Instruction arg2, Instruction arg3,
            Instruction arg4, Instruction arg5, Instruction arg6,
            Instruction arg7) {
    IV_STATIC_ASSERT(OPLength<op>::value == 8);
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
    data_->push_back(arg4);
    data_->push_back(arg5);
    data_->push_back(arg6);
    data_->push_back(arg7);
  }

  void Emit(OP::Type op) {
    data_->push_back(op);
  }

  void Emit(OP::Type op, Instruction arg) {
    data_->push_back(op);
    data_->push_back(arg);
  }

  void Emit(OP::Type op, Instruction arg1, Instruction arg2) {
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
  }

  void Emit(OP::Type op, Instruction arg1, Instruction arg2, Instruction arg3) {
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
  }

  void Emit(OP::Type op,
            Instruction arg1, Instruction arg2,
            Instruction arg3, Instruction arg4) {
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
    data_->push_back(arg4);
  }

  void EmitOPAt(OP::Type op, std::size_t index) {
    (*data_)[code_->start() + index] = op;
  }

  void EmitArgAt(Instruction arg, std::size_t index) {
    (*data_)[code_->start() + index] = arg;
  }

  void EmitJump(std::size_t to, std::size_t from) {
    EmitArgAt(Instruction::Diff(to, from), from + 1);
  }

  void EmitFunctionBindingInstantiation(const FunctionLiteral& lit,
                                        bool is_eval_decl) {
    FunctionScope* env =
        static_cast<FunctionScope*>(current_variable_scope_.get());
    registers_.Clear(env->stack_size());
    if (current_variable_scope_->UseExpressionReturn()) {
      eval_result_ = registers_.Acquire();
    }
    if (env->scope()->needs_heap_scope()) {
      Emit<OP::BUILD_ENV>(env->heap_size());
      assert(!env->heap().empty());
      for (FunctionScope::HeapVariables::const_iterator
           it = env->heap().begin(), last = env->heap().end();
           it != last; ++it) {
        Emit<OP::INSTANTIATE_HEAP_BINDING>(
            SymbolToNameIndex(it->first),
            std::get<1>(it->second),
            static_cast<uint32_t>(std::get<2>(it->second)));
      }
    }
    if (!is_eval_decl) {
      std::size_t param_count = 0;
      typedef std::unordered_map<Symbol, std::size_t> Symbol2Count;
      Symbol2Count sym2c;
      for (Code::Names::const_iterator it = code_->params().begin(),
           last = code_->params().end(); it != last; ++it, ++param_count) {
        sym2c[*it] = param_count;
      }
      for (Symbol2Count::const_iterator it = sym2c.begin(),
           last = sym2c.end(); it != last; ++it) {
        InstantiateLoadParam(SymbolToNameIndex(it->first), it->second);
      }
      if (env->scope()->IsArgumentsRealized()) {
        InstantiateArguments();
      }
      if (lit.IsFunctionNameExposed()) {
        InstantiateLoadCallee(
            SymbolToNameIndex(lit.name().Address()->symbol()));
      }
    }
    code_->set_heap_size(env->heap_size());
    code_->set_stack_size(env->stack_size());
  }

  void EmitPatchingBindingInstantiation(const FunctionLiteral& lit, bool eval) {
    assert(thunklist_.empty());
    registers_.Clear(0);
    if (current_variable_scope_->UseExpressionReturn()) {
      eval_result_ = registers_.Acquire();
    }
    std::unordered_set<Symbol> already_declared;
    const Scope& scope = lit.scope();
    typedef Scope::FunctionLiterals Functions;
    const Functions& functions = scope.function_declarations();
    const uint32_t flag = eval ? 1 : 0;
    for (Functions::const_iterator it = functions.begin(),
         last = functions.end(); it != last; ++it) {
      const Symbol name = (*it)->name().Address()->symbol();
      if (already_declared.find(name) == already_declared.end()) {
        already_declared.insert(name);
        const uint32_t index = SymbolToNameIndex(name);
        Emit<OP::INSTANTIATE_DECLARATION_BINDING>(index, flag);
      }
    }
    // variables
    typedef Scope::Variables Variables;
    const Variables& vars = scope.variables();
    for (Variables::const_iterator it = vars.begin(),
         last = vars.end(); it != last; ++it) {
      const Symbol name = it->first->symbol();
      if (already_declared.find(name) == already_declared.end()) {
        already_declared.insert(name);
        const uint32_t index = SymbolToNameIndex(name);
        Emit<OP::INSTANTIATE_VARIABLE_BINDING>(index, flag);
      }
    }
  }

  template<Code::CodeType TYPE>
  void EmitFunctionCode(
      const FunctionLiteral& lit,
      Code* code,
      std::shared_ptr<VariableScope> upper,
      bool is_eval_decl = false) {
    CodeContextPrologue(code);
    const std::size_t code_info_stack_size = code_info_stack_.size();
    const Scope& scope = lit.scope();
    current_variable_scope_ =
        std::shared_ptr<VariableScope>(
            new CodeScope<TYPE>(upper, &scope, is_eval_decl));
    dst_.reset();
    {
      // binding instantiation
      if (TYPE == Code::FUNCTION) {
        EmitFunctionBindingInstantiation(lit, is_eval_decl);
      } else {
        EmitPatchingBindingInstantiation(lit, TYPE == Code::EVAL);
      }
    }
    {
      // function declarations
      typedef Scope::FunctionLiterals Functions;
      const Functions& functions = scope.function_declarations();
      for (Functions::const_iterator it = functions.begin(),
           last = functions.end(); it != last; ++it) {
        const FunctionLiteral* const func = *it;
        const Symbol sym = func->name().Address()->symbol();
        const uint32_t index = SymbolToNameIndex(sym);
        if (IsUsedReference(index)) {
          if (RegisterID local = GetLocal(sym)) {
            EmitExpression(func, local);
          } else {
            RegisterID tmp = EmitExpression(func);
            EmitStore(sym, tmp);
          }
        }
      }
    }
    // main
    const Statements& stmts = lit.body();
    for (Statements::const_iterator it = stmts.begin(),
         last = stmts.end(); it != last; ++it) {
      (*it)->Accept(this);
      if (continuation_status_.IsDeadStatement()) {
        break;
      }
    }

    if (IsCodeEmpty()) {
      // only STOP_CODE
      code_->set_empty(true);
    }
    if (current_variable_scope_->UseExpressionReturn()) {
      Emit<OP::RETURN>(eval_result_);
    } else {
      Emit<OP::STOP_CODE>();
    }
    CodeContextEpilogue(code);

    const std::shared_ptr<VariableScope> target = current_variable_scope_;
    {
      // lazy code compile
      std::size_t code_info_stack_index = code_info_stack_size;
      for (Code::Codes::const_iterator it = code_->codes().begin(),
           last = code_->codes().end();
           it != last; ++it, ++code_info_stack_index) {
        const CodeInfo info = code_info_stack_[code_info_stack_index];
        assert(std::get<0>(info) == *it);
        EmitFunctionCode<Code::FUNCTION>(*std::get<1>(info),
                                         *it,
                                         std::get<2>(info));
      }
    }
    current_variable_scope_ = target->upper();
    // shrink code info stack
    code_info_stack_.erase(
        code_info_stack_.begin() + code_info_stack_size,
        code_info_stack_.end());
  }

  void EmitInstantiate(uint32_t index, const LookupInfo& info, RegisterID src) {
    switch (info.type()) {
      case LookupInfo::HEAP: {
        if (info.immutable()) {
          Emit<OP::INITIALIZE_HEAP_IMMUTABLE>(src, info.location());
        } else {
          Emit<OP::STORE_HEAP>(
              src, index, info.location(),
              current_variable_scope_->scope_nest_count() - info.scope());
        }
        return;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        Emit<OP::STORE_GLOBAL>(src, index, 0u, 0u);
        return;
      }
      case LookupInfo::LOOKUP: {
        Emit<OP::STORE_NAME>(src, index);
        return;
      }
      default: {
        // do nothing
      }
    }
  }

  void InstantiateLoadParam(uint32_t index, uint32_t param) {
    const LookupInfo info = Lookup(code_->names_[index]);
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::LOAD_PARAM>(info.location(), param);
    } else {
      RegisterID reg = registers_.Acquire();
      Emit<OP::LOAD_PARAM>(reg, param);
      EmitInstantiate(index, info, reg);
    }
  }

  void InstantiateArguments() {
    const LookupInfo info = Lookup(symbol::arguments());
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::LOAD_ARGUMENTS>(info.location());
    } else {
      RegisterID reg = registers_.Acquire();
      Emit<OP::LOAD_ARGUMENTS>(reg);
      EmitInstantiate(SymbolToNameIndex(symbol::arguments()), info, reg);
    }
  }

  void InstantiateLoadCallee(uint32_t index) {
    const LookupInfo info = Lookup(code_->names_[index]);
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::LOAD_CALLEE>(info.location());
    } else {
      RegisterID reg = registers_.Acquire();
      Emit<OP::LOAD_CALLEE>(reg);
      EmitInstantiate(index, info, reg);
    }
  }

  void set_code(Code* code) { code_ = code; }

  Code* code() const { return code_; }

  std::size_t CurrentSize() const {
    return data_->size() - code_->start();
  }

  bool IsCodeEmpty() const { return data_->size() == code_->start(); }

  const LevelStack& level_stack() const { return level_stack_; }

  // try - catch - finally nest level
  // use for break / continue exile by executing finally block
  std::size_t CurrentLevel() const { return level_stack_.size(); }

  void DynamicEnvLevelUp() { ++dynamic_env_level_; }

  void DynamicEnvLevelDown() { --dynamic_env_level_; }

  uint16_t dynamic_env_level() const {
    return dynamic_env_level_;
  }

  void set_dynamic_env_level(uint16_t dynamic_env_level) {
    dynamic_env_level_ = dynamic_env_level;
  }

  void RegisterJumpTarget(const BreakableStatement* stmt,
                          std::vector<std::size_t>* breaks) {
    jump_table_.insert(
        std::make_pair(
            stmt,
            std::make_tuple(
                CurrentLevel(),
                breaks,
                static_cast<std::vector<std::size_t>*>(NULL))));
  }

  void RegisterJumpTarget(const IterationStatement* stmt,
                          std::vector<std::size_t>* breaks,
                          std::vector<std::size_t>* continues) {
    jump_table_.insert(
        std::make_pair(
            stmt,
            std::make_tuple(
                CurrentLevel(),
                breaks,
                continues)));
  }

  void UnRegisterJumpTarget(const BreakableStatement* stmt) {
    jump_table_.erase(stmt);
  }

  void PushLevelFinally(std::vector<std::size_t>* vec) {
    level_stack_.push_back(
        std::make_tuple(FINALLY, vec,
                        RegisterID(), RegisterID(), RegisterID()));
  }

  void PushLevelWith() {
    level_stack_.push_back(
        std::make_tuple(WITH,
                        static_cast<std::vector<std::size_t>*>(NULL),
                        RegisterID(), RegisterID(), RegisterID()));
  }

  void PopLevel() { level_stack_.pop_back(); }

  bool ignore_result() const { return ignore_result_; }

  void set_ignore_result(bool val) { ignore_result_ = val; }

  RegisterID dst() const { return dst_; }

  void set_dst(RegisterID dst) { dst_ = dst; }

 private:
  Context* ctx_;
  Code* code_;
  CoreData* core_;
  Code::Data* data_;
  JSScript* script_;
  CodeInfoStack code_info_stack_;
  JumpTable jump_table_;
  LevelStack level_stack_;
  Registers registers_;
  ThunkList thunklist_;
  bool ignore_result_;
  RegisterID dst_;
  RegisterID eval_result_;
  std::unordered_map<Symbol, uint32_t> symbol_to_index_map_;
  JSStringToIndexMap jsstring_to_index_map_;
  JSDoubleToIndexMap double_to_index_map_;
  uint16_t dynamic_env_level_;
  ContinuationStatus continuation_status_;
  std::shared_ptr<VariableScope> current_variable_scope_;
  trace::Vector<Map*>::type temporary_;
};

inline Code* Compile(Context* ctx,
                     const FunctionLiteral& global, JSScript* script) {
  Compiler compiler(ctx);
  return compiler.Compile(global, script);
}

inline Code* CompileFunction(Context* ctx,
                             const FunctionLiteral& func, JSScript* script) {
  Compiler compiler(ctx);
  return compiler.CompileFunction(func, script);
}

inline Code* CompileEval(Context* ctx,
                         const FunctionLiteral& eval, JSScript* script) {
  Compiler compiler(ctx);
  return compiler.CompileEval(eval, script);
}

inline Code* CompileIndirectEval(Context* ctx,
                                 const FunctionLiteral& eval,
                                 JSScript* script) {
  Compiler compiler(ctx);
  return compiler.CompileIndirectEval(eval, script);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_COMPILER_H_
