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
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/op.h>
#include <iv/lv5/railgun/register_id.h>
#include <iv/lv5/railgun/instruction_fwd.h>
#include <iv/lv5/railgun/core_data.h>
#include <iv/lv5/railgun/code.h>
#include <iv/lv5/railgun/scope.h>
#include <iv/lv5/railgun/condition.h>
#include <iv/lv5/railgun/direct_threading.h>

namespace iv {
namespace lv5 {
namespace railgun {
namespace detail {

const Statement* kNextStatement = NULL;

}  // namespace detail

// save continuation status and find dead code
class ContinuationStatus : private core::Noncopyable<ContinuationStatus> {
 public:
  typedef std::unordered_set<const Statement*> ContinuationSet;

  ContinuationStatus()
    : current_() {
    Insert(detail::kNextStatement);
  }

  void Insert(const Statement* stmt) {
    current_.insert(stmt);
  }

  void Erase(const Statement* stmt) {
    current_.erase(stmt);
  }

  void Kill() {
    Erase(detail::kNextStatement);
  }

  bool Has(const Statement* stmt) const {
    return current_.count(stmt) != 0;
  }

  bool IsDeadStatement() const {
    return !Has(detail::kNextStatement);
  }

  void JumpTo(const BreakableStatement* target) {
    Kill();
    Insert(target);
  }

  void ResolveJump(const BreakableStatement* target) {
    if (Has(target)) {
      Erase(target);
      Insert(detail::kNextStatement);
    }
  }

  void Clear() {
    current_.clear();
    Insert(detail::kNextStatement);
  }

 private:
  ContinuationSet current_;
};

class Compiler
    : private core::Noncopyable<Compiler>,
      public AstVisitor {
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
    FORIN,
    FINALLY,
    SUB
  };

  typedef std::tuple<LevelType, RegisterID, std::vector<std::size_t>*> LevelEntry;
  typedef std::vector<LevelEntry> LevelStack;

  struct JSDoubleEquals {
    bool operator()(double lhs, double rhs) const {
      return lhs == rhs && (core::Signbit(lhs)) == core::Signbit(rhs);
    }
  };

  struct UStringPieceHash {
    std::size_t operator()(core::UStringPiece target) const {
      return core::Hash::StringToHash(target);
    }
  };

  struct UStringPieceEquals {
    bool operator()(core::UStringPiece lhs, core::UStringPiece rhs) const {
      return lhs == rhs;
    }
  };

  typedef std::unordered_map<
      double,
      uint32_t,
      std::hash<double>, JSDoubleEquals> JSDoubleToIndexMap;

  typedef std::unordered_map<
      core::UStringPiece,
      uint32_t,
      UStringPieceHash, UStringPieceEquals> JSStringToIndexMap;

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
      symbol_to_index_map_(),
      jsstring_to_index_map_(),
      double_to_index_map_(),
      dynamic_env_level_(),
      call_stack_depth_(),
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
      assert(std::get<2>(entry) == &vec_);
      for (std::vector<std::size_t>::const_iterator it = std::get<2>(entry)->begin(),
           last = std::get<2>(entry)->end(); it != last; ++it) {
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
    call_stack_depth_ = 0;
    jump_table_.clear();
    level_stack_.clear();
    symbol_to_index_map_.clear();
    jsstring_to_index_map_.clear();
    double_to_index_map_.clear();
    continuation_status_.Clear();
    code->set_start(data_->size());
  }

  void CodeContextEpilogue(Code* code) {
    code->set_end(data_->size());
    code->set_stack_depth(call_stack_depth_);
    code->set_registers(registers_.size());
  }

  // Statement
  //
  void EmitStatement(const Statement* stmt) {
    stmt->Accept(this);
  }

  void Visit(const Block* block) {
    BreakTarget jump(this, block);
    const Statements& stmts = block->body();
    for (Statements::const_iterator it = stmts.begin(),
         last = stmts.end(); it != last; ++it) {
      EmitStatement(*it);
      if (continuation_status_.IsDeadStatement()) {
        break;
      }
    }
    jump.EmitJumps(CurrentSize());
    continuation_status_.ResolveJump(block);
  }

  void Visit(const FunctionStatement* stmt) {
    const FunctionLiteral* func = stmt->function();
    assert(func->name());  // FunctionStatement must have name
    const Symbol name = func->name().Address()->symbol();
    const uint32_t index = SymbolToNameIndex(name);
    if (IsUsedReference(index)) {
      // be used or has side effect
      EmitStoreTo(func, name);
    }
  }

  void Visit(const FunctionDeclaration* func) { }

  void Visit(const VariableStatement* var) {
    const Declarations& decls = var->decls();
    for (Declarations::const_iterator it = decls.begin(),
         last = decls.end(); it != last; ++it) {
      Visit(*it);
    }
  }

  void Visit(const EmptyStatement* stmt) { }

  void Visit(const IfStatement* stmt) {
    const Condition::Type cond = Condition::Analyze(stmt->cond());
    std::size_t label;
    if (cond == Condition::COND_INDETERMINATE) {
      RegisterID dst = EmitNoSideEffectExpression(stmt->cond());
      label = CurrentSize() + 1;
      Emit<OP::IF_FALSE>(dst->reg(), 0);  // dummy index
    } else {
      label = CurrentSize();
    }

    if (const core::Maybe<const Statement> else_stmt = stmt->else_statement()) {
      if (cond != Condition::COND_FALSE) {
        // then statement block
        EmitStatement(stmt->then_statement());
      }

      if (continuation_status_.IsDeadStatement()) {
        continuation_status_.Insert(detail::kNextStatement);
      } else {
        continuation_status_.Insert(stmt);
      }

      const std::size_t second = CurrentSize();
      if (cond == Condition::COND_INDETERMINATE) {
        Emit<OP::JUMP_BY>(0);  // dummy index
        EmitJump(CurrentSize(), label);
      }

      if (cond != Condition::COND_TRUE) {
        EmitStatement(else_stmt.Address());
      }
      if (continuation_status_.Has(stmt)) {
        continuation_status_.Erase(stmt);
        if (continuation_status_.IsDeadStatement()) {
          continuation_status_.Insert(detail::kNextStatement);
        }
      }

      if (cond == Condition::COND_INDETERMINATE) {
        EmitJump(CurrentSize(), second);
      }
    } else {
      if (cond == Condition::COND_FALSE) {
        return;
      }
      // then statement block
      EmitStatement(stmt->then_statement());
      if (continuation_status_.IsDeadStatement()) {
        // recover if this IfStatement is not dead code
        continuation_status_.Insert(detail::kNextStatement);
      }
      if (cond == Condition::COND_INDETERMINATE) {
        EmitJump(CurrentSize(), label);
      }
    }
  }

  void Visit(const DoWhileStatement* stmt) {
    ContinueTarget jump(this, stmt);
    const std::size_t start_index = CurrentSize();

    EmitStatement(stmt->body());

    const std::size_t cond_index = CurrentSize();

    {
      RegisterID dst = EmitNoSideEffectExpression(stmt->cond());
      Emit<OP::IF_TRUE>(dst->reg(),
                        Instruction::Diff(start_index, CurrentSize()));
    }

    jump.EmitJumps(CurrentSize(), cond_index);

    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
  }

  void Visit(const WhileStatement* stmt) {
    ContinueTarget jump(this, stmt);
    const std::size_t start_index = CurrentSize();

    const Condition::Type cond = Condition::Analyze(stmt->cond());
    if (cond == Condition::COND_FALSE) {
      // like:
      //  while (false) {
      //  }
      return;
    }

    RegisterID dst;
    if (cond == Condition::COND_INDETERMINATE) {
      dst = EmitNoSideEffectExpression(stmt->cond());
    }

    if (stmt->body()->IsEffectiveStatement()) {
      if (cond == Condition::COND_INDETERMINATE) {
        const std::size_t label = CurrentSize() + 1;
        Emit<OP::IF_FALSE>(dst->reg(), 0);  // dummy index
        dst.reset();

        EmitStatement(stmt->body());

        Emit<OP::JUMP_BY>(Instruction::Diff(start_index, CurrentSize()));
        EmitJump(CurrentSize(), label);
      } else {
        assert(cond == Condition::COND_TRUE);

        EmitStatement(stmt->body());

        Emit<OP::JUMP_BY>(Instruction::Diff(start_index, CurrentSize()));
      }
      jump.EmitJumps(CurrentSize(), start_index);
      continuation_status_.ResolveJump(stmt);
    } else {
      if (cond == Condition::COND_INDETERMINATE) {
        Emit<OP::IF_TRUE>(
            dst->reg(),
            Instruction::Diff(start_index, CurrentSize()));
      } else {
        assert(cond == Condition::COND_TRUE);
        Emit<OP::JUMP_BY>(Instruction::Diff(start_index, CurrentSize()));
      }
    }

    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
  }

  void Visit(const ForStatement* stmt) {
    ContinueTarget jump(this, stmt);

    if (const core::Maybe<const Statement> maybe = stmt->init()) {
      const Statement* init = maybe.Address();
      if (init->AsVariableStatement()) {
        EmitStatement(init);
      } else {
        assert(init->AsExpressionStatement());
        // not evaluate as ExpressionStatement
        // because ExpressionStatement returns statement value
        EmitNoSideEffectExpression(init->AsExpressionStatement()->expr());
      }
    }

    const std::size_t start_index = CurrentSize();
    const core::Maybe<const Expression> cond = stmt->cond();
    std::size_t label = 0;

    if (cond) {
      RegisterID dst = EmitNoSideEffectExpression(cond.Address());
      label = CurrentSize() + 1;
      Emit<OP::IF_FALSE>(dst->reg(), 0);  // dummy index
    }

    EmitStatement(stmt->body());

    const std::size_t prev_next = CurrentSize();
    if (const core::Maybe<const Expression> next = stmt->next()) {
      EmitNoSideEffectExpression(next.Address());
    }

    Emit<OP::JUMP_BY>(Instruction::Diff(start_index, CurrentSize()));

    if (cond) {
      EmitJump(CurrentSize(), label);
    }

    jump.EmitJumps(CurrentSize(), prev_next);

    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
  }

  void Visit(const ForInStatement* stmt) {
    ContinueTarget jump(this, stmt);

    const Expression* lhs = NULL;
    Symbol for_decl = symbol::kDummySymbol;
    if (const VariableStatement* var = stmt->each()->AsVariableStatement()) {
      const Declaration* decl = var->decls().front();
      Visit(decl);
      for_decl = decl->name()->symbol();
    } else {
      // LeftHandSideExpression
      assert(stmt->each()->AsExpressionStatement());
      lhs = stmt->each()->AsExpressionStatement()->expr();
    }

    {
      RegisterID dst = EmitExpression(stmt->enumerable());
      PushLevelForIn();
      const std::size_t for_in_setup_jump = CurrentSize() + 2;
      Emit<OP::FORIN_SETUP>(dst->reg(), dst->reg(), 0);  // dummy index

      const std::size_t start_index = CurrentSize();
      {
        RegisterID tmp = registers_.Acquire();
        Emit<OP::FORIN_ENUMERATE>(tmp->reg(), dst->reg(), 0);  // dummy index

        if (!lhs || lhs->AsIdentifier()) {
          // Identifier
          if (lhs) {
            for_decl = lhs->AsIdentifier()->symbol();
          }
          EmitStoreTo(tmp, for_decl);
        } else if (lhs->AsPropertyAccess()) {
          // PropertyAccess
          if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
            // IdentifierAccess
            RegisterID base = EmitNoSideEffectExpression(ac->target());
            const uint32_t index = SymbolToNameIndex(ac->key());
            Emit<OP::STORE_PROP>(base->reg(), tmp->reg(), index, 0, 0, 0, 0);
          } else {
            // IndexAccess
            const IndexAccess* idx = lhs->AsIndexAccess();
            const Expression* key = idx->key();
            if (const StringLiteral* str = key->AsStringLiteral()) {
              RegisterID base = EmitNoSideEffectExpression(idx->target());
              const uint32_t index =
                  SymbolToNameIndex(context::Intern(ctx_, str->value()));
              Emit<OP::STORE_PROP>(base->reg(), tmp->reg(), index, 0, 0, 0, 0);
            } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
              RegisterID base = EmitNoSideEffectExpression(idx->target());
              const uint32_t index =
                  SymbolToNameIndex(context::Intern(ctx_, num->value()));
              Emit<OP::STORE_PROP>(base->reg(), tmp->reg(), index, 0, 0, 0, 0);
            } else {
              RegisterID base = EmitExpression(idx->target());
              RegisterID element = EmitExpression(idx->key());
              Emit<OP::STORE_ELEMENT>(base->reg(), element->reg(), tmp->reg());
            }
          }
        } else {
          // FunctionCall
          // ConstructorCall
          tmp.reset();
          EmitNoSideEffectExpression(lhs);
          Emit<OP::RAISE_REFERENCE>();
        }
      }

      EmitStatement(stmt->body());

      Emit<OP::JUMP_BY>(Instruction::Diff(start_index, CurrentSize()));

      const std::size_t end_index = CurrentSize();
      EmitJump(end_index, start_index + 2);
      EmitJump(end_index, for_in_setup_jump);

      jump.EmitJumps(end_index, start_index);
    }
    PopLevel();
    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
  }

  void EmitLevel(uint16_t from, uint16_t to) {
    for (; from > to; --from) {
      const LevelEntry& le = level_stack_[from - 1];
      const LevelType type = std::get<0>(le);
      if (type == FINALLY) {
        const std::size_t finally_jump = CurrentSize() + 1;
        Emit<OP::JUMP_SUBROUTINE>(std::get<1>(le)->reg(), 0);
        std::get<2>(le)->push_back(finally_jump);
      } else if (type == WITH) {
        Emit<OP::POP_ENV>();
      }
    }
  }

  void Visit(const ContinueStatement* stmt) {
    const JumpEntry& entry = jump_table_[stmt->target()];
    EmitLevel(CurrentLevel(), std::get<0>(entry));
    const std::size_t arg_index = CurrentSize();
    Emit<OP::JUMP_BY>(0);
    std::get<2>(entry)->push_back(arg_index);
    continuation_status_.JumpTo(stmt->target());
  }

  void Visit(const BreakStatement* stmt) {
    if (!stmt->target() && !stmt->label().IsDummy()) {
      // through
    } else {
      const JumpEntry& entry = jump_table_[stmt->target()];
      EmitLevel(CurrentLevel(), std::get<0>(entry));
      const std::size_t arg_index = CurrentSize();
      Emit<OP::JUMP_BY>(0);  // dummy
      std::get<1>(entry)->push_back(arg_index);
    }
    continuation_status_.JumpTo(stmt->target());
  }

  void Visit(const ReturnStatement* stmt) {
    RegisterID dst;
    if (const core::Maybe<const Expression> expr = stmt->expr()) {
      if (CurrentLevel() == 0) {
        dst = EmitNoSideEffectExpression(expr.Address());
      } else {
        dst = EmitExpression(expr.Address());
      }
    } else {
      dst = registers_.Acquire();
      Emit<OP::LOAD_UNDEFINED>(dst->reg());
    }

    if (CurrentLevel() == 0) {
      Emit<OP::RETURN>(dst->reg());
    } else {
      // nested finally has found
      // set finally jump targets
      EmitLevel(CurrentLevel(), 0);
      Emit<OP::RETURN>(dst->reg());
    }
    continuation_status_.Kill();
  }

  void Visit(const WithStatement* stmt) {
    RegisterID dst = EmitNoSideEffectExpression(stmt->context());
    Emit<OP::WITH_SETUP>(dst->reg());
    PushLevelWith();
    {
      DynamicEnvLevelCounter counter(this);
      current_variable_scope_ =
          std::shared_ptr<VariableScope>(
              new WithScope(current_variable_scope_));
      EmitStatement(stmt->body());
      current_variable_scope_ = current_variable_scope_->upper();
    }
    PopLevel();
    Emit<OP::POP_ENV>();
  }

  void Visit(const LabelledStatement* stmt) {
    EmitStatement(stmt->body());
  }

  void Visit(const SwitchStatement* stmt) {
    BreakTarget jump(this, stmt);
    typedef SwitchStatement::CaseClauses CaseClauses;
    const CaseClauses& clauses = stmt->clauses();
    bool has_default_clause = false;
    std::size_t label = 0;
    std::vector<std::size_t> indexes(clauses.size());
    {
      RegisterID cond = EmitExpression(stmt->expr());
      std::vector<std::size_t>::iterator idx = indexes.begin();
      std::vector<std::size_t>::iterator default_it = indexes.end();
      for (CaseClauses::const_iterator it = clauses.begin(),
           last = clauses.end(); it != last; ++it, ++idx) {
        if (const core::Maybe<const Expression> expr = (*it)->expr()) {
          // case
          RegisterID tmp = EmitExpression(expr.Address());
          Emit<OP::BINARY_STRICT_EQ>(tmp->reg(), cond->reg(), tmp->reg());
          *idx = CurrentSize() + 1;
          Emit<OP::IF_TRUE>(tmp->reg(), 0);  // dummy index
        } else {
          // default
          default_it = idx;
        }
      }
      if (default_it != indexes.end()) {
        *default_it = CurrentSize();
        has_default_clause = true;
      } else {
        // all cases are not equal and no default case
        label = CurrentSize();
      }
      Emit<OP::JUMP_BY>(0);
    }
    {
      std::vector<std::size_t>::const_iterator idx = indexes.begin();
      for (CaseClauses::const_iterator it = clauses.begin(),
           last = clauses.end(); it != last; ++it, ++idx) {
        EmitJump(CurrentSize(), *idx);
        const Statements& stmts = (*it)->body();
        for (Statements::const_iterator stmt_it = stmts.begin(),
             stmt_last = stmts.end(); stmt_it != stmt_last; ++stmt_it) {
          EmitStatement(*stmt_it);
          if (continuation_status_.IsDeadStatement()) {
            break;
          }
        }
        if (continuation_status_.IsDeadStatement()) {
          if ((it + 1) != last) {
            continuation_status_.Insert(detail::kNextStatement);
          }
        }
      }
    }
    jump.EmitJumps(CurrentSize());
    if (!has_default_clause) {
      EmitJump(CurrentSize(), label);
    }

    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement() && !has_default_clause) {
      continuation_status_.Insert(detail::kNextStatement);
    }
  }

  void Visit(const ThrowStatement* stmt) {
    RegisterID dst = EmitNoSideEffectExpression(stmt->expr());
    Emit<OP::THROW>(dst->reg());
    continuation_status_.Kill();
  }

  void Visit(const TryStatement* stmt) {
    const std::size_t try_start = CurrentSize();
    const bool has_catch = stmt->catch_block();
    const bool has_finally = stmt->finally_block();
    TryTarget target(this, has_finally);
    RegisterID jmp;
    if (has_finally) {
      jmp = registers_.Acquire();
      std::get<1>(level_stack_[CurrentLevel() - 1]) = jmp;
    }
    EmitStatement(stmt->body());
    if (has_finally) {
      const std::size_t finally_jump = CurrentSize() + 1;
      Emit<OP::JUMP_SUBROUTINE>(jmp->reg(), 0);  // dummy index
      std::get<2>(level_stack_[CurrentLevel() - 1])->push_back(finally_jump);
    }
    const std::size_t label = CurrentSize();
    Emit<OP::JUMP_BY>(0);  // dummy index

    std::size_t catch_return_label_index = 0;
    if (const core::Maybe<const Block> block = stmt->catch_block()) {
      if (continuation_status_.IsDeadStatement()) {
        continuation_status_.Insert(detail::kNextStatement);
      } else {
        continuation_status_.Insert(stmt);
      }
      const Symbol catch_symbol = stmt->catch_name().Address()->symbol();
      {
        RegisterID error = registers_.Acquire();
        code_->RegisterHandler<Handler::CATCH>(
            try_start,
            CurrentSize(),
            error->reg(),
            dynamic_env_level());
        Emit<OP::TRY_CATCH_SETUP>(error->reg(), SymbolToNameIndex(catch_symbol));
      }
      PushLevelWith();
      {
        DynamicEnvLevelCounter counter(this);
        current_variable_scope_ =
            std::shared_ptr<VariableScope>(
                new CatchScope(current_variable_scope_, catch_symbol));
        block.Address()->Accept(this);  // STMT
        current_variable_scope_ = current_variable_scope_->upper();
      }
      PopLevel();
      Emit<OP::POP_ENV>();
      if (has_finally) {
        const std::size_t finally_jump = CurrentSize() + 1;
        Emit<OP::JUMP_SUBROUTINE>(jmp->reg(), 0);  // dummy index
        std::get<2>(level_stack_[CurrentLevel() - 1])->push_back(finally_jump);
      }
      catch_return_label_index = CurrentSize();
      Emit<OP::JUMP_BY>(0);  // dummy index

      if (continuation_status_.Has(stmt)) {
        continuation_status_.Erase(stmt);
        if (continuation_status_.IsDeadStatement()) {
          continuation_status_.Insert(detail::kNextStatement);
        }
      }
    }

    if (const core::Maybe<const Block> block = stmt->finally_block()) {
      const std::size_t finally_start = CurrentSize();
      if (continuation_status_.IsDeadStatement()) {
        continuation_status_.Insert(detail::kNextStatement);
      } else {
        continuation_status_.Insert(stmt);
      }
      code_->RegisterHandler<Handler::FINALLY>(
          try_start,
          finally_start,
          jmp->reg(),
          dynamic_env_level());
      target.EmitJumps(finally_start);

      PushLevelSub();
      EmitStatement(block.Address());
      Emit<OP::RETURN_SUBROUTINE>(jmp->reg());
      PopLevel();

      if (continuation_status_.Has(stmt)) {
        continuation_status_.Erase(stmt);
      } else {
        if (!continuation_status_.IsDeadStatement()) {
          continuation_status_.Kill();
        }
      }
    }
    // try last
    EmitJump(CurrentSize(), label);
    // catch last
    if (has_catch) {
      EmitJump(CurrentSize(), catch_return_label_index);
    }
  }

  void Visit(const DebuggerStatement* stmt) {
    Emit<OP::DEBUGGER>();
  }

  void Visit(const ExpressionStatement* stmt) {
    // TODO(Constellation) fix return value
    if (current_variable_scope_->UseExpressionReturn()) {
      EmitNoSideEffectExpression(stmt->expr());
    } else {
      EmitNoSideEffectExpression(stmt->expr());
    }
  }

  // Expression

  RegisterID EmitExpression(const Expression* expr) {
    return EmitExpression(expr, registers_.Acquire());
  }

  RegisterID EmitExpression(const Expression* expr, RegisterID tmp) {
    RegisterID prev = dst_;
    dst_ = tmp;
    expr->Accept(this);
    dst_ = prev;
    return tmp;
  }

  RegisterID EmitNoSideEffectExpression(const Expression* expr) {
    if (const Identifier* ident = expr->AsIdentifier()) {
      const LookupInfo info = Lookup(ident->symbol());
      if (info.type() == LookupInfo::STACK) {
        return registers_.LocalID(info.location());
      }
    }
    return EmitExpression(expr);
  }

  LookupInfo Lookup(const Symbol sym) {
    return current_variable_scope_->Lookup(sym);
  }

  void Visit(const Assignment* assign) {
    using core::Token;
    const Token::Type token = assign->op();
    if (token == Token::TK_ASSIGN) {
      EmitAssign(assign->left(), assign->right(), dst_);
    } else {
      const Expression* lhs = assign->left();
      const Expression* rhs = assign->right();
      if (!lhs->IsValidLeftHandSide()) {
        EmitExpression(lhs);
        dst_ = EmitExpression(rhs);
        Emit<OP::TO_NUMBER_AND_RAISE_REFERENCE>();
        return;
      }
      if (const Identifier* ident = lhs->AsIdentifier()) {
        // Identifier
        const uint32_t index = SymbolToNameIndex(ident->symbol());
        RegisterID load = EmitLoadName(index);
        EmitExpression(rhs, dst_);
        EmitAssignedBinaryOperation(token, dst_, load, dst_);
        EmitStoreTo(dst_, ident->symbol());
      } else if (lhs->AsPropertyAccess()) {
        // PropertyAccess
        if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
          // IdentifierAccess
          RegisterID base = EmitExpression(ac->target());
          const uint32_t index = SymbolToNameIndex(ac->key());
          Emit<OP::LOAD_PROP>(dst_->reg(), base->reg(), index, 0, 0, 0, 0);
          {
            RegisterID tmp = EmitExpression(rhs);
            EmitAssignedBinaryOperation(token, dst_, dst_, tmp);
          }
          Emit<OP::STORE_PROP>(base->reg(), dst_->reg(), index, 0, 0, 0, 0);
        } else {
          // IndexAccess
          const IndexAccess* idx = lhs->AsIndexAccess();
          RegisterID base = EmitExpression(idx->target());
          const Expression* key = idx->key();
          if (const StringLiteral* str = key->AsStringLiteral()) {
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, str->value()));
            Emit<OP::LOAD_PROP>(dst_->reg(), base->reg(), index, 0, 0, 0, 0);
            {
              RegisterID tmp = EmitExpression(rhs);
              EmitAssignedBinaryOperation(token, dst_, dst_, tmp);
            }
            Emit<OP::STORE_PROP>(base->reg(), dst_->reg(), index, 0, 0, 0, 0);
          } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, num->value()));
            Emit<OP::LOAD_PROP>(dst_->reg(), base->reg(), index, 0, 0, 0, 0);
            {
              RegisterID tmp = EmitExpression(rhs);
              EmitAssignedBinaryOperation(token, dst_, dst_, tmp);
            }
            Emit<OP::STORE_PROP>(base->reg(), dst_->reg(), index, 0, 0, 0, 0);
          } else {
            RegisterID element = EmitExpression(key);
            Emit<OP::LOAD_ELEMENT>(dst_->reg(), base->reg(), element->reg());
            {
              RegisterID tmp = EmitExpression(rhs);
              EmitAssignedBinaryOperation(token, dst_, dst_, tmp);
            }
            Emit<OP::STORE_ELEMENT>(base->reg(), element->reg(), dst_->reg());
          }
        }
      } else {
        // FunctionCall
        // ConstructorCall
        EmitExpression(lhs, dst_);
        {
          RegisterID tmp = EmitExpression(rhs, dst_);
          EmitAssignedBinaryOperation(token, dst_, dst_, tmp);
        }
        Emit<OP::RAISE_REFERENCE>();
      }
    }
  }

  void Visit(const BinaryOperation* binary) {
    using core::Token;
    const Token::Type token = binary->op();
    switch (token) {
      case Token::TK_LOGICAL_AND: {  // &&
        EmitExpression(binary->left(), dst_);
        const std::size_t label = CurrentSize() + 1;
        Emit<OP::IF_FALSE>(dst_->reg(), 0);  // dummy index
        EmitExpression(binary->right(), dst_);
        EmitJump(CurrentSize(), label);
        break;
      }

      case Token::TK_LOGICAL_OR: {  // ||
        EmitExpression(binary->left(), dst_);
        const std::size_t label = CurrentSize() + 1;
        Emit<OP::IF_TRUE>(dst_->reg(), 0);  // dummy index
        EmitExpression(binary->right(), dst_);
        EmitJump(CurrentSize(), label);
        break;
      }

      case Token::TK_ADD: {  // +
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_ADD>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_SUB: {  // -
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_SUBTRACT>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_SHR: {  // >>>
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_RSHIFT_LOGICAL>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_SAR: {  // >>
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_RSHIFT>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_SHL: {  // <<
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_LSHIFT>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_MUL: {  // *
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_MULTIPLY>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_DIV: {  // /
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_DIVIDE>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_MOD: {  // %
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_MODULO>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_LT: {  // <
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_LT>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_GT: {  // >
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_GT>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_LTE: {  // <=
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_LTE>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_GTE: {  // >=
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_GTE>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_INSTANCEOF: {  // instanceof
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_INSTANCEOF>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_IN: {  // in
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_IN>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_EQ: {  // ==
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_EQ>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_NE: {  // !=
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_NE>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_EQ_STRICT: {  // ===
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_STRICT_EQ>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_NE_STRICT: {  // !==
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_STRICT_NE>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_BIT_AND: {  // &
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_BIT_AND>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_BIT_XOR: {  // ^
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_BIT_XOR>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_BIT_OR: {  // |
        EmitExpression(binary->left(), dst_);
        RegisterID rhs = EmitExpression(binary->right());
        Emit<OP::BINARY_BIT_OR>(dst_->reg(), dst_->reg(), rhs->reg());
        break;
      }

      case Token::TK_COMMA: {  // ,
        EmitExpression(binary->left(), dst_);
        EmitExpression(binary->right(), dst_);
        break;
      }

      default:
        UNREACHABLE();
    }
  }

  void Visit(const ConditionalExpression* cond) {
    EmitExpression(cond->cond(), dst_);
    const std::size_t first = CurrentSize() + 1;
    Emit<OP::IF_FALSE>(dst_->reg(), 0);
    EmitExpression(cond->left(), dst_);
    const std::size_t second = CurrentSize();
    Emit<OP::JUMP_BY>(0);
    EmitJump(CurrentSize(), first);
    EmitExpression(cond->right(), dst_);
    EmitJump(CurrentSize(), second);
  }

  void Visit(const UnaryOperation* unary) {
    using core::Token;
    const Token::Type token = unary->op();
    switch (token) {
      case Token::TK_DELETE: {
        const Expression* expr = unary->expr();
        if (expr->IsValidLeftHandSide()) {
          // Identifier
          // PropertyAccess
          // FunctionCall
          // ConstructorCall
          if (const Identifier* ident = expr->AsIdentifier()) {
            // DELETE_NAME_STRICT is already rejected in parser
            assert(!code_->strict());
            EmitDeleteName(SymbolToNameIndex(ident->symbol()), dst_);
          } else if (expr->AsPropertyAccess()) {
            if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
              // IdentifierAccess
              EmitExpression(ac->target(), dst_);
              const uint32_t index = SymbolToNameIndex(ac->key());
              Emit<OP::DELETE_PROP>(dst_->reg(), dst_->reg(), index, 0, 0, 0, 0);
            } else {
              // IndexAccess
              EmitElement<OP::DELETE_PROP,
                          OP::DELETE_ELEMENT>(expr->AsIndexAccess(), dst_);
            }
          } else {
            EmitExpression(expr, dst_);
            Emit<OP::LOAD_TRUE>(dst_->reg());
          }
        } else {
          // other case is no effect
          // but accept expr
          EmitExpression(expr, dst_);
          Emit<OP::LOAD_TRUE>(dst_->reg());
        }
        break;
      }

      case Token::TK_VOID: {
        EmitExpression(unary->expr(), dst_);
        Emit<OP::LOAD_UNDEFINED>(dst_->reg());
        break;
      }

      case Token::TK_TYPEOF: {
        const Expression* expr = unary->expr();
        if (const Identifier* ident = expr->AsIdentifier()) {
          // maybe Global Reference
          EmitTypeofName(SymbolToNameIndex(ident->symbol()), dst_);
        } else {
          EmitExpression(expr, dst_);
          Emit<OP::TYPEOF>(dst_->reg(), dst_->reg());
        }
        break;
      }

      case Token::TK_INC:
      case Token::TK_DEC: {
        const Expression* expr = unary->expr();
        if (!expr->IsValidLeftHandSide()) {
          expr.Accept(this);
          Emit<OP::TO_NUMBER_AND_RAISE_REFERENCE>();
          return;
        }
        assert(expr->IsValidLeftHandSide());
        if (const Identifier* ident = expr->AsIdentifier()) {
          const uint32_t index = SymbolToNameIndex(ident->symbol());
          if (token == Token::TK_INC) {
            EmitIncrementName(index, dst_);
          } else {
            EmitDecrementName(index, dst_);
          }
        } else if (expr->AsPropertyAccess()) {
          if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
            // IdentifierAccess
            EmitExpression(ac->target(), dst_);
            const uint32_t index = SymbolToNameIndex(ac->key());
            if (token == Token::TK_INC) {
              Emit<OP::INCREMENT_PROP>(dst_->reg(), dst_->reg(), index, 0, 0, 0, 0);
            } else {
              Emit<OP::DECREMENT_PROP>(dst_->reg(), dst_->reg(), index, 0, 0, 0, 0);
            }
          } else {
            // IndexAccess
            const IndexAccess* idxac = expr->AsIndexAccess();
            if (token == Token::TK_INC) {
              EmitElement<OP::INCREMENT_PROP,
                          OP::INCREMENT_ELEMENT>(idxac, dst_);
            } else {
              EmitElement<OP::DECREMENT_PROP,
                          OP::DECREMENT_ELEMENT>(idxac, dst_);
            }
          }
        } else {
          EmitExpression(expr, dst_);
          if (token == Token::TK_INC) {
            Emit<OP::INCREMENT_CALL_RESULT>(dst_->reg());
          } else {
            Emit<OP::DECREMENT_CALL_RESULT>(dst_->reg());
          }
        }
        break;
      }

      case Token::TK_ADD: {
        EmitExpression(unary->expr(), dst_);
        Emit<OP::UNARY_POSITIVE>(dst_->reg(), dst_->reg());
        break;
      }

      case Token::TK_SUB: {
        EmitExpression(unary->expr(), dst_);
        Emit<OP::UNARY_NEGATIVE>(dst_->reg(), dst_->reg());
        break;
      }

      case Token::TK_BIT_NOT: {
        EmitExpression(unary->expr(), dst_);
        Emit<OP::UNARY_BIT_NOT>(dst_->reg(), dst_->reg());
        break;
      }

      case Token::TK_NOT: {
        EmitExpression(unary->expr(), dst_);
        Emit<OP::UNARY_NOT>(dst_->reg(), dst_->reg());
        break;
      }

      default:
        UNREACHABLE();
    }
  }

  void Visit(const PostfixExpression* postfix) {
    using core::Token;
    const Expression* expr = postfix->expr();
    const Token::Type token = postfix->op();
    if (!expr.IsValidLeftHandSide()) {
      expr.Accept(this);
      Emit<OP::TO_NUMBER_AND_RAISE_REFERENCE>();
      return;
    }
    if (const Identifier* ident = expr.AsIdentifier()) {
      const uint32_t index = SymbolToNameIndex(ident->symbol());
      if (token == Token::TK_INC) {
        EmitPostfixIncrementName(index, dst_);
      } else {
        EmitPostfixDecrementName(index, dst_);
      }
    } else if (expr->AsPropertyAccess()) {
      if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
        // IdentifierAccess
        EmitExpression(ac->target(), dst_);
        const uint32_t index = SymbolToNameIndex(ac->key());
        if (token == Token::TK_INC) {
          Emit<OP::POSTFIX_INCREMENT_PROP>(dst_->reg(), dst_->reg(),
                                           index, 0, 0, 0, 0);
        } else {
          Emit<OP::POSTFIX_DECREMENT_PROP>(dst_->reg(), dst_->reg(),
                                           index, 0, 0, 0, 0);
        }
      } else {
        // IndexAccess
        const IndexAccess* idxac = expr->AsIndexAccess();
        if (token == Token::TK_INC) {
          EmitElement<OP::POSTFIX_INCREMENT_PROP,
                      OP::POSTFIX_INCREMENT_ELEMENT>(idxac, dst_);
        } else {
          EmitElement<OP::POSTFIX_DECREMENT_PROP,
                      OP::POSTFIX_DECREMENT_ELEMENT>(idxac, dst_);
        }
      }
    } else {
      EmitExpression(expr, dst_);
      if (token == Token::TK_INC) {
        Emit<OP::POSTFIX_INCREMENT_CALL_RESULT>(dst_->reg());
      } else {
        Emit<OP::POSTFIX_DECREMENT_CALL_RESULT>(dst_->reg());
      }
    }
  }

  void Visit(const StringLiteral* lit) {
    const JSStringToIndexMap::const_iterator it =
        jsstring_to_index_map_.find(lit->value());
    if (it != jsstring_to_index_map_.end()) {
      // duplicate constant
      Emit<OP::LOAD_CONST>(dst_->reg(), it->second);
      return;
    }
    // new constant value
    const uint32_t index = code_->constants_.size();
    code_->constants_.push_back(
        JSString::New(
            ctx_,
            lit->value().begin(),
            lit->value().end(),
            core::character::IsASCII(lit->value().begin(),
                                     lit->value().end())));
    jsstring_to_index_map_.insert(std::make_pair(lit->value(), index));
    Emit<OP::LOAD_CONST>(dst_->reg(), index);
  }

  void Visit(const NumberLiteral* lit) {
    const double val = lit->value();

    const int32_t i32 = static_cast<int32_t>(val);
    if (val == i32 && (i32 || !core::Signbit(val))) {
      // boxing int32_t
      Instruction inst(0u);
      inst.i32 = i32;
      Emit<OP::LOAD_INT32>(dst_->reg(), inst);
      return;
    }

    const uint32_t ui32 = static_cast<uint32_t>(val);
    if (val == ui32 && (ui32 || !core::Signbit(val))) {
      // boxing uint32_t
      Emit<OP::LOAD_UINT32>(dst_->reg(), ui32);
      return;
    }

    const JSDoubleToIndexMap::const_iterator it =
        double_to_index_map_.find(val);
    if (it != double_to_index_map_.end()) {
      // duplicate constant pool
      Emit<OP::LOAD_CONST>(dst_->reg(), it->second);
      return;
    }

    // new constant value
    const uint32_t index = code_->constants_.size();
    code_->constants_.push_back(val);
    double_to_index_map_.insert(std::make_pair(val, index));
    Emit<OP::LOAD_CONST>(dst_->reg(), index);
  }

  void Visit(const Assigned* lit) { }

  void Visit(const Identifier* lit) {
    EmitLoadName(SymbolToNameIndex(lit->symbol()), dst_);
  }

  void Visit(const ThisLiteral* lit) {
    Emit<OP::LOAD_THIS>(dst_->reg());
  }

  void Visit(const NullLiteral* lit) {
    Emit<OP::LOAD_NULL>(dst_->reg());
  }

  void Visit(const TrueLiteral* lit) {
    Emit<OP::LOAD_TRUE>(dst_->reg());
  }

  void Visit(const FalseLiteral* lit) {
    Emit<OP::LOAD_FALSE>(dst_->reg());
  }

  void Visit(const RegExpLiteral* lit) {
    Emit<OP::BUILD_REGEXP>(dst_->reg(), code_->constants_.size());
    code_->constants_.push_back(
        JSRegExp::New(ctx_, lit->value(), lit->regexp()));
  }

  void Visit(const ArrayLiteral* lit) {
    typedef ArrayLiteral::MaybeExpressions Items;
    const Items& items = lit->items();
    Emit<OP::BUILD_ARRAY>(dst_->reg(), items.size());
    uint32_t current = 0;
    for (Items::const_iterator it = items.begin(),
         last = items.end(); it != last; ++it, ++current) {
      const core::Maybe<const Expression>& expr = *it;
      if (expr) {
        RegisterID item = EmitExpression(expr.Address());
        if (JSArray::kMaxVectorSize > current) {
          Emit<OP::INIT_VECTOR_ARRAY_ELEMENT>(dst_->reg(), item->reg(), current);
        } else {
          Emit<OP::INIT_SPARSE_ARRAY_ELEMENT>(dst_->reg(), item->reg(), current);
        }
      }
    }
  }

  void Visit(const ObjectLiteral* lit) {
    using std::get;
    typedef ObjectLiteral::Properties Properties;
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::BUILD_OBJECT>(dst_->reg(), 0u);
    std::unordered_map<Symbol, std::size_t> slots;
    const Properties& properties = lit->properties();
    for (Properties::const_iterator it = properties.begin(),
         last = properties.end(); it != last; ++it) {
      const ObjectLiteral::Property& prop = *it;
      const ObjectLiteral::PropertyDescriptorType type(get<0>(prop));
      const Symbol name = get<1>(prop);

      uint32_t merged = 0;
      uint32_t position = 0;
      std::unordered_map<Symbol, std::size_t>::const_iterator it2 =
          slots.find(name);
      if (it2 == slots.end()) {
        position = slots.size();
        slots.insert(std::make_pair(name, position));
      } else {
        merged = 1;  // already defined property
        position = it2->second;
      }

      RegisterID item = EmitExpression(get<2>(prop));
      if (type == ObjectLiteral::DATA) {
        Emit<OP::STORE_OBJECT_DATA>(dst_->reg(), item->reg(), position, merged);
      } else if (type == ObjectLiteral::GET) {
        Emit<OP::STORE_OBJECT_GET>(dst_->reg(), item->reg(), position, merged);
      } else {
        Emit<OP::STORE_OBJECT_SET>(dst_->reg(), item->reg(), position, merged);
      }
    }
    Map* map = Map::NewObjectLiteralMap(ctx_, slots.begin(), slots.end());
    temporary_.push_back(map);
    Instruction inst(0u);
    inst.map = map;
    // TODO(Constellation) fix this adding
    EmitArgAt(inst, arg_index);
  }

  void Visit(const FunctionLiteral* lit) {
    Code* const code = new Code(ctx_, script_, *lit, core_, Code::FUNCTION);
    const uint32_t index = code_->codes_.size();
    code_->codes_.push_back(code);
    code_info_stack_.push_back(
        std::make_tuple(code, lit, current_variable_scope_));
    Emit<OP::LOAD_FUNCTION>(dst_->reg(), index);
  }

  void Visit(const IdentifierAccess* prop) {
    EmitExpression(prop->target(), dst_);
    const uint32_t index = SymbolToNameIndex(prop->key());
    Emit<OP::LOAD_PROP>(dst_->reg(), dst_->reg(), index, 0, 0, 0, 0);
  }

  void Visit(const IndexAccess* prop) {
    EmitElement<OP::LOAD_PROP, OP::LOAD_ELEMENT>(prop, dst_);
  }

  void Visit(const FunctionCall* call) {
    EmitCall<OP::CALL>(*call, dst_);
  }

  void Visit(const ConstructorCall* call) {
    EmitCall<OP::CONSTRUCT>(*call, dst_);
  }

  void Visit(const Declaration* decl) {
    if (const core::Maybe<const Expression> maybe = decl->expr()) {
      const uint32_t index = SymbolToNameIndex(decl->name()->symbol());
      const Expression* expr = maybe.Address();
      if (IsUsedReference(index) || !Condition::NoSideEffect(expr)) {
        // be used or has side effect
        EmitStoreTo(expr, decl->name()->symbol());
      }
    }
  }

  void Visit(const CaseClause* dummy) { }

  // complex emit functions

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

  template<OP::Type PropOP,
           OP::Type ElementOP>
  void EmitElement(const IndexAccess* prop, RegisterID dst) {
    // TODO(Constellation) fix Store and Load
    EmitExpression(prop->target(), dst);
    const Expression* key = prop->key();
    if (const StringLiteral* str = key->AsStringLiteral()) {
      const uint32_t index =
          SymbolToNameIndex(context::Intern(ctx_, str->value()));
      Emit<PropOP>(dst->reg(), dst->reg(), index, 0, 0, 0, 0);
    } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
      const uint32_t index =
          SymbolToNameIndex(context::Intern(ctx_, num->value()));
      Emit<PropOP>(dst->reg(), dst->reg(), index, 0, 0, 0, 0);
    } else {
      RegisterID element = EmitExpression(prop->key());
      Emit<ElementOP>(dst->reg(), element->reg(), dst->reg());
    }
  }

  void EmitAssign(const Expression* lhs,
                  const Expression* rhs, RegisterID dst) {
    if (!lhs.IsValidLeftHandSide()) {
      lhs.Accept(this);
      rhs.Accept(this);
      Emit<OP::RAISE_REFERENCE>();
      stack_depth_.Down();
      return;
    }
    assert(lhs->IsValidLeftHandSide());
    if (const Identifier* ident = lhs->AsIdentifier()) {
      // Identifier
      EmitStoreTo(rhs, ident->symbol(), dst);
    } else if (lhs->AsPropertyAccess()) {
      // PropertyAccess
      if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
        // IdentifierAccess
        RegisterID base = EmitExpression(ac->target());
        EmitExpression(rhs, dst);
        const uint32_t index = SymbolToNameIndex(ac->key());
        Emit<OP::STORE_PROP>(base->reg(), dst->reg(),
                             index, 0, 0, 0, 0);
      } else {
        // IndexAccess
        const IndexAccess* idx = lhs->AsIndexAccess();
        RegisterID base = EmitExpression(idx->target());
        const Expression* key = idx->key();
        if (const StringLiteral* str = key->AsStringLiteral()) {
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, str->value()));
          EmitExpression(rhs, dst);
          Emit<OP::STORE_PROP>(base->reg(), dst->reg(), index, 0, 0, 0, 0);
        } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, num->value()));
          EmitExpression(rhs, dst);
          Emit<OP::STORE_PROP>(base->reg(), dst->reg(), index, 0, 0, 0, 0);
        } else {
          RegisterID element = EmitExpression(key);
          EmitExpression(rhs, dst);
          Emit<OP::STORE_ELEMENT>(base->reg(), element->reg(), dst->reg());
        }
      }
    } else {
      // FunctionCall
      // ConstructorCall
      EmitExpression(lhs, dst);
      EmitExpression(rhs, dst);
      Emit<OP::RAISE_REFERENCE>();
    }
  }

  template<OP::Type op, typename Call>
  void EmitCall(const Call& call, RegisterID dst) {
    bool direct_call_to_eval = false;
    const Expression* target = call.target();
    if (target->IsValidLeftHandSide()) {
      if (const Identifier* ident = target->AsIdentifier()) {
        const uint32_t index = SymbolToNameIndex(ident->symbol());
        EmitCallName(index, dst);
        if (op == OP::CALL && ident->symbol() == symbol::eval()) {
          direct_call_to_eval = true;
        }
      } else if (const PropertyAccess* prop = target->AsPropertyAccess()) {
        if (const IdentifierAccess* ac = prop->AsIdentifierAccess()) {
          // IdentifierAccess
          EmitExpression(prop->target(), dst);
          const uint32_t index = SymbolToNameIndex(ac->key());
          Emit<OP::CALL_PROP>(dst->reg(), dst->reg(), index, 0, 0, 0, 0);
        } else {
          // IndexAccess
          EmitElement<OP::CALL_PROP,
                      OP::CALL_ELEMENT>(prop->AsIndexAccess(), dst);
        }
      } else {
        EmitExpression(target, dst);
        Emit<OP::PUSH_UNDEFINED>();
      }
    } else {
      EmitExpression(target, dst);
      Emit<OP::PUSH_UNDEFINED>();
    }

    const Expressions& args = call.args();
    for (Expressions::const_iterator it = args.begin(),
         last = args.end(); it != last; ++it) {
      RegisterID item = EmitNoSideEffectExpression(*it);
      Emit<OP::PUSH>(item->reg());
    }
    call_stack_depth_ = std::max<uint32_t>(call_stack_depth_, args.size() + 1);

    if (direct_call_to_eval) {
      Emit<OP::EVAL>(dst->reg(), dst->reg(), args.size());
    } else {
      Emit<op>(dst->reg(), dst->reg(), args.size());
    }
  }

  void EmitAssignedBinaryOperation(
      core::Token::Type token,
      RegisterID dst, RegisterID lhs, RegisterID rhs) {
    using core::Token;
    switch (token) {
      case Token::TK_ASSIGN_ADD: {  // +=
        Emit<OP::BINARY_ADD>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_SUB: {  // -=
        Emit<OP::BINARY_SUBTRACT>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_MUL: {  // *=
        Emit<OP::BINARY_MULTIPLY>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_MOD: {  // %=
        Emit<OP::BINARY_MODULO>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_DIV: {  // /=
        Emit<OP::BINARY_DIVIDE>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_SAR: {  // >>=
        Emit<OP::BINARY_RSHIFT>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_SHR: {  // >>>=
        Emit<OP::BINARY_RSHIFT_LOGICAL>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_SHL: {  // <<=
        Emit<OP::BINARY_LSHIFT>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_BIT_AND: {  // &=
        Emit<OP::BINARY_BIT_AND>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_BIT_OR: {  // |=
        Emit<OP::BINARY_BIT_OR>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      case Token::TK_ASSIGN_BIT_XOR: {  // ^=
        Emit<OP::BINARY_BIT_XOR>(dst->reg(), lhs->reg(), rhs->reg());
        break;
      }

      default:
        UNREACHABLE();
    }
  }

  RegisterID LocalID(uint32_t id) {
    return registers_.LocalID(id);
  }

  void EmitStoreTo(RegisterID tmp, Symbol sym) {
    const uint32_t index = SymbolToNameIndex(sym);
    const LookupInfo info = Lookup(sym);
    const OP::Type op = OP::STORE_NAME;
    switch (info.type()) {
      case LookupInfo::STACK: {
        const OP::Type res = (info.immutable()) ?
            OP::ToLocalImmutable(op, code_->strict()) : OP::ToLocal(op);
        if (res == OP::RAISE_IMMUTABLE) {
          Emit(res, index);
        } else {
          EmitMV(info.location(), tmp->reg());
        }
        break;
      }
      case LookupInfo::HEAP: {
        Emit(OP::ToHeap(op), tmp->reg(), index, info.location(),
             current_variable_scope_->scope_nest_count() - info.scope());
        break;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        Emit(OP::ToGlobal(op), tmp->reg(), index, 0u, 0u);
        break;
      }
      case LookupInfo::LOOKUP: {
        Emit(op, tmp->reg(), index);
        break;
      }
      case LookupInfo::UNUSED: {
        // do nothing
        break;
      }
    }
  }

  void EmitStoreTo(const Expression* expr, Symbol sym,
                   RegisterID dst = RegisterID()) {
    const uint32_t index = SymbolToNameIndex(sym);
    const LookupInfo info = Lookup(sym);
    const OP::Type op = OP::STORE_NAME;
    switch (info.type()) {
      case LookupInfo::STACK: {
        const OP::Type res = (info.immutable()) ?
            OP::ToLocalImmutable(op, code_->strict()) : OP::ToLocal(op);
        if (res == OP::RAISE_IMMUTABLE) {
          EmitExpression(expr);
          Emit(res, index);
        } else {
          if (dst) {
            EmitExpression(expr, dst);
            EmitMV(info.location(), dst->reg());
          } else {
            EmitExpression(expr, LocalID(info.location()));
          }
        }
        break;
      }
      case LookupInfo::HEAP: {
        if (dst) {
          EmitExpression(expr, dst);
        } else {
          dst = EmitExpression(expr);
        }
        Emit(OP::ToHeap(op), dst->reg(), index, info.location(),
             current_variable_scope_->scope_nest_count() - info.scope());
        break;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        if (dst) {
          EmitExpression(expr, dst);
        } else {
          dst = EmitExpression(expr);
        }
        Emit(OP::ToGlobal(op), dst->reg(), index, 0u, 0u);
        break;
      }
      case LookupInfo::LOOKUP: {
        if (dst) {
          EmitExpression(expr, dst);
        } else {
          dst = EmitExpression(expr);
        }
        Emit(op, dst->reg(), index);
        break;
      }
      case LookupInfo::UNUSED: {
        if (dst) {
          EmitExpression(expr, dst);
        } else {
          dst = EmitExpression(expr);
        }
        // do nothing
        break;
      }
    }
  }

  RegisterID EmitOptimizedLookup(OP::Type op,
                                 uint32_t index, RegisterID dst) {
    const LookupInfo info = Lookup(code_->names_[index]);
    switch (info.type()) {
      case LookupInfo::STACK: {
        const OP::Type res = (info.immutable()) ?
            OP::ToLocalImmutable(op, code_->strict()) : OP::ToLocal(op);
        if (res == OP::RAISE_IMMUTABLE) {
          Emit(res, index);
        } else {
          Emit(res, dst->reg(), info.location());
        }
        return dst;
      }
      case LookupInfo::HEAP: {
        Emit(OP::ToHeap(op),
             dst->reg(),
             index, info.location(),
             current_variable_scope_->scope_nest_count() - info.scope());
        return dst;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        Emit(OP::ToGlobal(op), dst->reg(), index, 0u, 0u);
        return dst;
      }
      case LookupInfo::LOOKUP: {
        Emit(op, dst->reg(), index);
        return dst;
      }
      case LookupInfo::UNUSED: {
        assert(op == OP::STORE_NAME);
        // do nothing
        return dst;
      }
    }
  }

  // EmitLoadName is 2 pattern
  RegisterID EmitLoadName(uint32_t index, RegisterID dst = RegisterID()) {
    return EmitOptimizedLookup(OP::LOAD_NAME, index, dst);
  }

  RegisterID EmitCallName(uint32_t index, RegisterID dst) {
    return EmitOptimizedLookup(OP::CALL_NAME, index, dst);
  }

  RegisterID EmitIncrementName(uint32_t index, RegisterID dst) {
    return EmitOptimizedLookup(OP::INCREMENT_NAME, index, dst);
  }

  RegisterID EmitPostfixIncrementName(uint32_t index, RegisterID dst) {
    return EmitOptimizedLookup(OP::POSTFIX_INCREMENT_NAME, index, dst);
  }

  RegisterID EmitDecrementName(uint32_t index, RegisterID dst) {
    return EmitOptimizedLookup(OP::DECREMENT_NAME, index, dst);
  }

  RegisterID EmitPostfixDecrementName(uint32_t index, RegisterID dst) {
    return EmitOptimizedLookup(OP::POSTFIX_DECREMENT_NAME, index, dst);
  }

  RegisterID EmitTypeofName(uint32_t index, RegisterID dst) {
    return EmitOptimizedLookup(OP::TYPEOF_NAME, index, dst);
  }

  RegisterID EmitDeleteName(uint32_t index, RegisterID dst) {
    return EmitOptimizedLookup(OP::DELETE_NAME, index, dst);
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
        EmitLoadParam(SymbolToNameIndex(it->first), it->second);
      }
      if (env->scope()->IsArgumentsRealized()) {
        EmitArguments(SymbolToNameIndex(symbol::arguments()));
      }
      if (lit.IsFunctionNameExposed()) {
        EmitLoadCallee(SymbolToNameIndex(lit.name().Address()->symbol()));
      }
    }
    code_->set_heap_size(env->heap_size());
    code_->set_stack_size(env->stack_size());
  }

  void EmitPatchingBindingInstantiation(const FunctionLiteral& lit, bool eval) {
    registers_.Clear(0);
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
          EmitStoreTo(func, sym);
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
    Emit<OP::STOP_CODE>();
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

  void EmitMV(uint32_t to, uint32_t from) {
    if (to != from) {
      Emit<OP::MV>(to, from);
    }
  }

  void EmitInstantiate(uint32_t index, const LookupInfo& info, RegisterID src) {
    switch (info.type()) {
      case LookupInfo::HEAP: {
        if (info.immutable()) {
          Emit<OP::INITIALIZE_HEAP_IMMUTABLE>(src->reg(), info.location());
        } else {
          Emit<OP::STORE_HEAP>(
              src->reg(), index, info.location(),
              current_variable_scope_->scope_nest_count() - info.scope());
        }
        return;
      }
      case LookupInfo::GLOBAL: {
        // last 2 zeros are placeholders for PIC
        Emit<OP::STORE_GLOBAL>(src->reg(), index, 0u, 0u);
        return;
      }
      case LookupInfo::LOOKUP: {
        Emit<OP::STORE_NAME>(src->reg(), index);
        return;
      }
      default: {
        // do nothing
      }
    }
  }

  void EmitLoadParam(uint32_t index, uint32_t param) {
    const LookupInfo info = Lookup(code_->names_[index]);
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::LOAD_PARAM>(info.location(), param);
    } else {
      RegisterID reg = registers_.Acquire();
      Emit<OP::LOAD_PARAM>(reg->reg(), param);
      EmitInstantiate(index, info, reg);
    }
  }

  void EmitArguments(uint32_t index) {
    const LookupInfo info = Lookup(code_->names_[index]);
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::LOAD_ARGUMENTS>(info.location());
    } else {
      RegisterID reg = registers_.Acquire();
      Emit<OP::LOAD_ARGUMENTS>(reg->reg());
      EmitInstantiate(index, info, reg);
    }
  }

  void EmitLoadCallee(uint32_t index) {
    const LookupInfo info = Lookup(code_->names_[index]);
    if (info.type() == LookupInfo::STACK) {
      Emit<OP::LOAD_CALLEE>(info.location());
    } else {
      RegisterID reg = registers_.Acquire();
      Emit<OP::LOAD_CALLEE>(reg->reg());
      EmitInstantiate(index, info, reg);
    }
  }

  void set_code(Code* code) { code_ = code; }

  Code* code() const { return code_; }

  std::size_t CurrentSize() const {
    return data_->size() - code_->start();
  }

  bool IsCodeEmpty() const {
    return data_->size() == code_->start();
  }

  const LevelStack& level_stack() const {
    return level_stack_;
  }

  // try - catch - finally nest level
  // use for break / continue exile by executing finally block
  std::size_t CurrentLevel() const {
    return level_stack_.size();
  }

  void DynamicEnvLevelUp() {
    ++dynamic_env_level_;
  }

  void DynamicEnvLevelDown() {
    --dynamic_env_level_;
  }

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
    level_stack_.push_back(std::make_tuple(FINALLY, RegisterID(), vec));
  }

  void PushLevelWith() {
    level_stack_.push_back(
        std::make_tuple(WITH, RegisterID(),
                        static_cast<std::vector<std::size_t>*>(NULL)));
  }

  void PushLevelForIn() {
    level_stack_.push_back(
        std::make_tuple(FORIN, RegisterID(),
                        static_cast<std::vector<std::size_t>*>(NULL)));
  }

  void PushLevelSub() {
    level_stack_.push_back(
        std::make_tuple(SUB, RegisterID(),
                        static_cast<std::vector<std::size_t>*>(NULL)));
  }

  void PopLevel() { level_stack_.pop_back(); }

  Context* ctx_;
  Code* code_;
  CoreData* core_;
  Code::Data* data_;
  JSScript* script_;
  CodeInfoStack code_info_stack_;
  JumpTable jump_table_;
  LevelStack level_stack_;
  Registers registers_;
  RegisterID dst_;
  std::unordered_map<Symbol, uint32_t> symbol_to_index_map_;
  JSStringToIndexMap jsstring_to_index_map_;
  JSDoubleToIndexMap double_to_index_map_;
  uint16_t dynamic_env_level_;
  uint32_t call_stack_depth_;
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
