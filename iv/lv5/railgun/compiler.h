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
  typedef core::Token Token;

  class Jump {
   public:
    typedef std::unordered_map<const BreakableStatement*, Jump> Table;
    typedef std::vector<std::size_t> Targets;
    Jump(uint32_t level, Targets* breaks, Targets* continues)
      : level_(level),
        breaks_(breaks),
        continues_(continues) {
    }
    uint32_t level() const { return level_; }
    Targets* breaks() const { return breaks_; }
    Targets* continues() const { return continues_; }
   private:
    uint32_t level_;
    Targets* breaks_;
    Targets* continues_;
  };

  typedef std::tuple<Code*,
                     const FunctionLiteral*,
                     std::shared_ptr<VariableScope> > CodeInfo;
  typedef std::vector<CodeInfo> CodeInfoStack;

  class Level {
   public:
    enum Type {
      FINALLY,
      ITERATOR,
      ENV
    };

    typedef std::vector<Level> Stack;

    Level(Type type, Jump::Targets* holes)
      : type_(type),
        holes_(holes),
        jmp_(),
        ret_(),
        flag_() {
    }

    Type type() const { return type_; }

    Jump::Targets* holes() const { return holes_; }

    RegisterID jmp() const { return jmp_; }
    void set_jmp(RegisterID jmp) {
      jmp_ = jmp;
    }

    RegisterID ret() const { return ret_; }
    void set_ret(RegisterID ret) {
      ret_ = ret;
    }

    RegisterID flag() const { return flag_; }
    void set_flag(RegisterID flag) {
      flag_ = flag;
    }

   private:
    Type type_;
    Jump::Targets* holes_;
    RegisterID jmp_;
    RegisterID ret_;
    RegisterID flag_;
  };

  typedef std::unordered_map<
      double,
      int32_t,
      std::hash<double>, JSDoubleEquals> JSDoubleToIndexMap;

  typedef std::unordered_map<core::UString, int32_t> JSStringToIndexMap;

  class ArraySite;

  friend class ThunkPool;
  friend class ArraySite;
  friend class CallSite;

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
      thunkpool_(this),
      ignore_result_(false),
      dst_(),
      eval_result_(),
      symbol_to_index_map_(),
      jsstring_to_index_map_(),
      double_to_index_map_(),
      continuation_status_(),
      current_variable_scope_(),
      temporary_() {
  }

  // entry points

  // Global Code
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

  // Function Code
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

  // Direct call to eval Code
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

  // Indirect call to eval Code
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
  void CompileEpilogue(Code* code) {
    // optimiazation or direct threading
#if defined(IV_LV5_RAILGUN_USE_DIRECT_THREADED_CODE)
    // direct threading label translation
    const DirectThreadingDispatchTable& table = VM::DispatchTable();
    Code::Data* data = code->GetData();
    for (Code::Data::iterator it = data->begin(),
         last = data->end(); it != last;) {
      const uint32_t opcode = it->u32[0];
      it->label = table[opcode];
      std::advance(it, kOPLength[opcode]);
    }
#endif
    core_->SetCompiled();
    ctx_->global_data()->RegExpClear();
  }

  void CodeContextPrologue(Code* code) {
    set_code(code);
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
  // implementation is in compiler_statement.h

  class BreakTarget;
  class ContinueTarget;
  class TryTarget;

  void EmitStatement(const Statement* stmt) {
    stmt->Accept(this);
  }

  RegisterID EmitUnrollingLevel(uint16_t from, uint16_t to,
                                bool cont,
                                RegisterID dst = RegisterID());

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

  class DestGuard;

  template<OP::Type PropOP, OP::Type ElementOP>
  RegisterID EmitElement(const IndexAccess* prop, RegisterID dst);

  template<typename Call>
  class CallSite;

  template<OP::Type op, typename Call>
  RegisterID EmitCall(const Call& call, RegisterID dst);

  RegisterID EmitOptimizedLookup(OP::Type op, uint32_t index, RegisterID dst);

  template<core::Token::Type token>
  void EmitLogicalPath(const BinaryOperation* binary);

  void EmitIdentifierAccessAssign(const Assignment* assign,
                                  const Expression* target,
                                  Symbol symbol);

  void EmitIdentifierAccessBinaryAssign(const Assignment* assign,
                                        const Expression* target,
                                        Symbol sym);

  RegisterID EmitConcat(const BinaryOperation* start,
                        const Expression* lhs, RegisterID dst);

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

  RegisterID EmitExpressionInternal(const Expression* expr,
                                    RegisterID dst,
                                    bool ignore_result) {
    EmitExpressionContext context(this, dst, ignore_result);
    expr->Accept(this);
    assert(!dst || dst == dst_);
    return dst_;
  }

  RegisterID EmitExpression(const Expression* expr) {
    return EmitExpressionInternal(expr, RegisterID(), false);
  }

  // force write
  RegisterID EmitExpressionToDest(const Expression* expr, RegisterID dst) {
    return EmitExpressionInternal(expr, dst, false);
  }

  void EmitExpressionIgnoreResult(const Expression* expr) {
    EmitExpressionInternal(expr, RegisterID(), true);
  }

  LookupInfo Lookup(const Symbol sym) {
    return current_variable_scope_->Lookup(sym);
  }

  // may return dummy symbol (symbol::kDummySymbol)
  Symbol PropertyName(const Expression* key) const {
    if (const StringLiteral* str = key->AsStringLiteral()) {
      return context::Intern(ctx_, str->value());
    } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
      return context::Intern(ctx_, num->value());
    } else {
      return symbol::kDummySymbol;
    }
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

  void EmitStore(Symbol sym, RegisterID src) {
    const uint32_t index = SymbolToNameIndex(sym);
    const LookupInfo info = Lookup(sym);
    switch (info.type()) {
      case LookupInfo::HEAP: {
        Emit<OP::STORE_HEAP>(
            Instruction::SW(src, index),
            Instruction::UInt32(
                info.heap_location(),
                current_variable_scope_->scope_nest_count() - info.scope()));
        return;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        Emit<OP::STORE_GLOBAL>(Instruction::SW(src, index), 0u, 0u);
        return;
      }
      case LookupInfo::LOOKUP: {
        Emit<OP::STORE_NAME>(Instruction::SW(src, index));
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

  RegisterID Temporary(RegisterID a = RegisterID(),
                       RegisterID b = RegisterID(),
                       RegisterID c = RegisterID(),
                       RegisterID d = RegisterID()) {
    if (a && a->IsTemporary()) {
      return a;
    }
    if (b && b->IsTemporary()) {
      return b;
    }
    if (c && c->IsTemporary()) {
      return c;
    }
    if (d && d->IsTemporary()) {
      return d;
    }
    return registers_.Acquire();
  }

  static bool NotOrdered(RegisterID target) {
    return !target || target->IsTemporary();
  }

  // determine which register is used
  // if dst is not ignored, use dst.
  RegisterID Dest(RegisterID dst,
                  RegisterID cand1 = RegisterID(),
                  RegisterID cand2 = RegisterID()) {
    if (dst) {
      return dst;
    }

    if (cand1 && cand1->IsTemporary()) {
      if (cand2 && cand2->IsTemporary()) {
        if (cand1->register_offset() > cand2->register_offset()) {
          // cand2 is smaller number than cand1
          return cand2;
        }
      }
      return cand1;
    }

    if (cand2 && cand2->IsTemporary()) {
      return cand2;
    }
    return registers_.Acquire();
  }

  RegisterID GetLocal(Symbol sym) {
    const LookupInfo info = Lookup(sym);
    if (info.type() == LookupInfo::STACK) {
      return registers_.LocalID(info.register_location());
    }
    return RegisterID();
  }

  RegisterID EmitMV(RegisterID to, RegisterID from) {
    if (!to) {
      return from;
    }
    if (to != from) {
      thunkpool_.Spill(to);
      Emit<OP::MV>(Instruction::Reg2(to, from));
    }
    return to;
  }

  RegisterID SpillRegister(RegisterID from) {
    RegisterID to = Temporary();
    Emit<OP::MV>(Instruction::Reg2(to, from));
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

  void EmitUnsafe(OP::Type op) {
    data_->push_back(op);
  }

  void EmitUnsafe(OP::Type op, Instruction arg) {
    data_->push_back(op);
    data_->push_back(arg);
  }

  void EmitUnsafe(OP::Type op, Instruction arg1, Instruction arg2) {
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
  }

  void EmitUnsafe(OP::Type op, Instruction arg1,
                  Instruction arg2, Instruction arg3) {
    data_->push_back(op);
    data_->push_back(arg1);
    data_->push_back(arg2);
    data_->push_back(arg3);
  }

  void EmitUnsafe(OP::Type op,
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
    (*data_)[code_->start() + from + 1].jump.to = to - from;
  }

  // enter code functions

  void EmitFunctionBindingInstantiation(const FunctionLiteral& lit,
                                        bool is_eval_decl) {
    FunctionScope* env =
        static_cast<FunctionScope*>(current_variable_scope_.get());
    registers_.Clear(env->stack_size(), env->heap_size());

    assert(code_->names_.empty());

    // save eval result or not
    if (current_variable_scope_->UseExpressionReturn()) {
      eval_result_ = Temporary();
    }

    if (env->scope()->needs_heap_scope()) {
      assert(!env->heap().empty());
      assert(code_->names_.empty());
      Emit<OP::BUILD_ENV>(
          Instruction::UInt32(env->heap_size(), env->mutable_start()));
      code_->set_needs_declarative_environment(true);
      for (FunctionScope::HeapVariables::const_iterator
           it = env->heap().begin(), last = env->heap().end();
           it != last; ++it) {
        // set symbols
        const Symbol sym = *it;
        const uint32_t index = code_->names_.size();
        symbol_to_index_map_[sym] = index;
        code_->names_.push_back(sym);
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
        const LookupInfo info = Lookup(it->first);
        if (info.type() != LookupInfo::STACK) {
          const uint32_t index = SymbolToNameIndex(it->first);
          assert(info.register_location() < 0 && "only arguments");
          EmitInstantiate(index, info,
                          registers_.LocalID(info.register_location()));
        }
      }
      if (env->scope()->IsArgumentsRealized()) {
        InstantiateArguments();
      }

      if (env->LoadCalleeNeeded()) {
        InstantiateLoadCallee(
            SymbolToNameIndex(lit.name().Address()->symbol()));
      }
    }

    code_->set_heap_size(registers_.heap_size());
    code_->set_stack_size(registers_.stack_size());
  }

  void EmitPatchingBindingInstantiation(const FunctionLiteral& lit, bool eval) {
    // not create new environment
    // simply use given it.
    //   for example,
    //     * direct call to eval in normal code
    //     * global code
    registers_.Clear(0, 0);

    // save eval result or not
    if (current_variable_scope_->UseExpressionReturn()) {
      eval_result_ = Temporary();
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
        Emit<OP::INSTANTIATE_DECLARATION_BINDING>(
            Instruction::UInt32(index, flag));
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
        Emit<OP::INSTANTIATE_VARIABLE_BINDING>(
            Instruction::UInt32(index, flag));
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
            new CodeScope<TYPE>(&lit, upper, &scope, is_eval_decl));
    {
      // binding instantiation
      if (TYPE == Code::FUNCTION) {
        EmitFunctionBindingInstantiation(lit, is_eval_decl);
      } else {
        EmitPatchingBindingInstantiation(lit, TYPE == Code::EVAL);
      }
    }
    thunkpool_.Initialize(code);
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
            EmitExpressionToDest(func, local);
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
      code_->set_empty(true);
    }

    // return eval result
    if (current_variable_scope_->UseExpressionReturn()) {
      Emit<OP::RETURN>(eval_result_);
    } else {
      if (!continuation_status_.IsDeadStatement()) {
        // insert return undefined
        RegisterID tmp = Temporary();
        Emit<OP::LOAD_UNDEFINED>(tmp);
        Emit<OP::RETURN>(tmp);
      }
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
          Emit<OP::INITIALIZE_HEAP_IMMUTABLE>(
              Instruction::SW(src, info.heap_location()));
        } else {
          Emit<OP::STORE_HEAP>(
              Instruction::SW(src, index),
              Instruction::UInt32(
                  info.heap_location(),
                  current_variable_scope_->scope_nest_count() - info.scope()));
        }
        return;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        Emit<OP::STORE_GLOBAL>(Instruction::SW(src, index), 0u, 0u);
        return;
      }
      case LookupInfo::LOOKUP: {
        Emit<OP::STORE_NAME>(Instruction::SW(src, index));
        return;
      }
      default: {
        // do nothing
      }
    }
  }

  void InstantiateArguments() {
    const LookupInfo info = Lookup(symbol::arguments());
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::LOAD_ARGUMENTS>(info.register_location());
    } else {
      RegisterID reg = Temporary();
      Emit<OP::LOAD_ARGUMENTS>(reg);
      EmitInstantiate(SymbolToNameIndex(symbol::arguments()), info, reg);
    }
  }

  void InstantiateLoadCallee(uint32_t index) {
    const Symbol sym = code_->names_[index];
    const LookupInfo info = Lookup(sym);
    assert(registers_.Callee()->IsConstant());
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::MV>(Instruction::Reg2(GetLocal(sym), registers_.Callee()));
    } else {
      EmitInstantiate(index, info, registers_.Callee());
    }
  }

  // accessors

  void set_code(Code* code) { code_ = code; }

  Code* code() const { return code_; }

  std::size_t CurrentSize() const {
    return data_->size() - code_->start();
  }

  bool IsCodeEmpty() const { return data_->size() == code_->start(); }

  const Level::Stack& level_stack() const { return level_stack_; }

  // try - catch - finally nest level
  // use for break / continue exile by executing finally block
  std::size_t CurrentLevel() const { return level_stack_.size(); }

  void RegisterJumpTarget(const BreakableStatement* stmt,
                          Jump::Targets* breaks) {
    jump_table_.insert(
        std::make_pair(stmt, Jump(CurrentLevel(), breaks, NULL)));
  }

  void RegisterJumpTarget(const IterationStatement* stmt,
                          Jump::Targets* breaks,
                          Jump::Targets* continues) {
    jump_table_.insert(
        std::make_pair(stmt, Jump(CurrentLevel(), breaks, continues)));
  }

  void UnRegisterJumpTarget(const BreakableStatement* stmt) {
    jump_table_.erase(stmt);
  }

  void PushLevelFinally(Jump::Targets* vec) {
    level_stack_.push_back(Level(Level::FINALLY, vec));
  }

  void PushLevelEnv() {
    level_stack_.push_back(Level(Level::ENV, NULL));
  }

  void PushLevelIterator(RegisterID iterator) {
    Level level(Level::ITERATOR, NULL);
    level.set_ret(iterator);
    level_stack_.push_back(level);
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
  Jump::Table jump_table_;
  Level::Stack level_stack_;
  Registers registers_;
  ThunkPool thunkpool_;
  bool ignore_result_;
  RegisterID dst_;
  RegisterID eval_result_;
  std::unordered_map<Symbol, uint32_t> symbol_to_index_map_;
  JSStringToIndexMap jsstring_to_index_map_;
  JSDoubleToIndexMap double_to_index_map_;
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
