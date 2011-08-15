#ifndef _IV_LV5_RAILGUN_COMPILER_H_
#define _IV_LV5_RAILGUN_COMPILER_H_
#include <algorithm>
#include <gc/gc.h>
#include "detail/tuple.h"
#include "detail/unordered_map.h"
#include "detail/memory.h"
#include "utils.h"
#include "ast_visitor.h"
#include "noncopyable.h"
#include "static_assert.h"
#include "enable_if.h"
#include "unicode.h"
#include "lv5/specialized_ast.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/jsregexp.h"
#include "lv5/gc_template.h"
#include "lv5/context_utils.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/instruction_fwd.h"
#include "lv5/railgun/core_data.h"
#include "lv5/railgun/code.h"
#include "lv5/railgun/scope.h"
#include "lv5/railgun/condition.h"
#include "lv5/railgun/direct_threading.h"

namespace iv {
namespace lv5 {
namespace railgun {
namespace detail {

const Statement* kNextStatement = NULL;

}  // namespace detail

class StackDepth : private core::Noncopyable<> {
 public:
  StackDepth()
    : current_depth_(0),
      max_depth_(0),
      base_(0) {
  }

  void Up(std::size_t i = 1) {
    current_depth_ += i;
    Update();
  }

  void Down(std::size_t i = 1) {
    assert(current_depth_ >= i);
    current_depth_ -= i;
  }

  void BaseUp(std::size_t i) {
    base_ += i;
  }

  void BaseDown(std::size_t i) {
    base_ -= i;
  }

  uint32_t GetMaxDepth() const {
    return max_depth_;
  }

  uint32_t GetCurrent() const {
    return current_depth_;
  }

  uint32_t GetStackBase() const {
    return base_;
  }

  bool IsBaseLine() const {
    return current_depth_ == base_;
  }

  void Clear() {
    current_depth_ = 0;
    max_depth_ = 0;
    base_ = 0;
  }

 private:
  void Update() {
    if (current_depth_ > max_depth_) {
      max_depth_ = current_depth_;
    }
  }

  uint32_t current_depth_;
  uint32_t max_depth_;
  uint32_t base_;
};

class DepthPoint : private core::Noncopyable<> {
 public:
  explicit DepthPoint(StackDepth* depth)
#ifdef DEBUG
    : depth_(depth),
      current_(depth->GetCurrent())
#endif  // DEBUG
  { }

  void LevelCheck(std::size_t i) {
    assert(depth_->GetCurrent() == (current_ + i));
  }

 private:
#ifdef DEBUG
  StackDepth* depth_;
  uint32_t current_;
#endif  // DEBUG
};

// save continuation status and find dead code
class ContinuationStatus : private iv::core::Noncopyable<ContinuationStatus> {
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
  typedef std::pair<LevelType, std::vector<std::size_t>*> LevelEntry;
  typedef std::vector<LevelEntry> LevelStack;

  explicit Compiler(Context* ctx)
    : ctx_(ctx),
      code_(NULL),
      core_(NULL),
      data_(NULL),
      script_(NULL),
      code_info_stack_(),
      jump_table_(),
      level_stack_(),
      stack_depth_(),
      symbol_to_index_map_(),
      dynamic_env_level_(0),
      continuation_status_(),
      current_variable_scope_() {
  }

  Code* Compile(const FunctionLiteral& global, JSScript* script) {
    Code* code = NULL;
    {
      script_ = script;
      core_ = CoreData::New();
      data_ = core_->data();
      data_->reserve(4 * core::Size::KB);
      current_variable_scope_ = std::shared_ptr<VariableScope>();
      code = new Code(ctx_, script_, global, core_, Code::GLOBAL);
      EmitFunctionCode(global, code, current_variable_scope_);
      assert(!current_variable_scope_);
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
      // create dummy global scope
      current_variable_scope_ =
          std::shared_ptr<VariableScope>(
              new FunctionScope(std::shared_ptr<VariableScope>(), data_));
      std::shared_ptr<VariableScope> target = current_variable_scope_;
      code = new Code(ctx_, script_, function, core_, Code::FUNCTION);
      EmitFunctionCode(function, code, current_variable_scope_);
      current_variable_scope_ = current_variable_scope_->upper();
      assert(!current_variable_scope_);
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
      current_variable_scope_ = std::shared_ptr<VariableScope>();
      code = new Code(ctx_, script_, eval, core_, Code::EVAL);
      EmitFunctionCode(eval, code, current_variable_scope_);
      assert(!current_variable_scope_);
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
        compiler_->EmitArgAt(break_target, *it);
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
        compiler_->EmitArgAt(continue_target, *it);
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
      std::pair<LevelType, std::vector<std::size_t>*> pair = stack.back();
      assert(pair.first == FINALLY);
      assert(pair.second == &vec_);
      for (std::vector<std::size_t>::const_iterator it = pair.second->begin(),
           last = pair.second->end(); it != last; ++it) {
        compiler_->EmitArgAt(finally_target, *it);
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
  }

  void CodeContextPrologue(Code* code) {
    set_code(code);
    ClearJumpTable();
    ClearLevelStack();
    ClearStackDepth();
    set_dynamic_env_level(0);
    ClearContinuation();
    ClearSymbolToIndexMap();
    code->set_start(data_->size());
  }

  void CodeContextEpilogue(Code* code) {
    code->set_end(data_->size());
    assert(stack_depth_.GetCurrent() == 0);
    code->set_stack_depth(stack_depth_.GetMaxDepth());
  }

  void Visit(const Block* block) {
    BreakTarget jump(this, block);
    const Statements& stmts = block->body();
    for (Statements::const_iterator it = stmts.begin(),
         last = stmts.end(); it != last; ++it) {
      (*it)->Accept(this);
      if (continuation_status_.IsDeadStatement()) {
        break;
      }
    }
    jump.EmitJumps(CurrentSize());
    continuation_status_.ResolveJump(block);
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const FunctionStatement* stmt) {
    const FunctionLiteral& func = *stmt->function();
    assert(func.name());  // FunctionStatement must have name
    const uint32_t index = SymbolToNameIndex(func.name().Address()->symbol());
    Visit(&func);
    EmitStoreName(index);
    Emit<OP::POP_TOP>();
    stack_depth_.Down();
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const FunctionDeclaration* func) {
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const VariableStatement* var) {
    const Declarations& decls = var->decls();
    for (Declarations::const_iterator it = decls.begin(),
         last = decls.end(); it != last; ++it) {
      Visit(*it);
    }
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const EmptyStatement* stmt) {
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const IfStatement* stmt) {
    const Condition::Type cond = Condition::Analyze(stmt->cond());
    if (cond == Condition::COND_INDETERMINATE) {
      stmt->cond()->Accept(this);
    }
    const std::size_t arg_index = CurrentSize() + 1;

    if (cond == Condition::COND_INDETERMINATE) {
      Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
      stack_depth_.Down();
    }

    if (const core::Maybe<const Statement> else_stmt = stmt->else_statement()) {
      if (cond != Condition::COND_FALSE) {
        // then statement block
        stmt->then_statement()->Accept(this);  // STMT
      }

      if (continuation_status_.IsDeadStatement()) {
        continuation_status_.Insert(detail::kNextStatement);
      } else {
        continuation_status_.Insert(stmt);
      }
      assert(stack_depth_.IsBaseLine());

      const std::size_t second_label_index = CurrentSize();
      if (cond == Condition::COND_INDETERMINATE) {
        Emit<OP::JUMP_FORWARD>(0);  // dummy index
        EmitArgAt(CurrentSize(), arg_index);
      }

      if (cond != Condition::COND_TRUE) {
        else_stmt.Address()->Accept(this);  // STMT
      }
      if (continuation_status_.Has(stmt)) {
        continuation_status_.Erase(stmt);
        if (continuation_status_.IsDeadStatement()) {
          continuation_status_.Insert(detail::kNextStatement);
        }
      }
      assert(stack_depth_.IsBaseLine());

      if (cond == Condition::COND_INDETERMINATE) {
        EmitArgAt(CurrentSize() - second_label_index, second_label_index + 1);
      }
    } else {
      if (cond == Condition::COND_FALSE) {
        assert(stack_depth_.IsBaseLine());
        return;
      }
      // then statement block
      stmt->then_statement()->Accept(this);  // STMT
      if (continuation_status_.IsDeadStatement()) {
        // recover if this IfStatement is not dead code
        continuation_status_.Insert(detail::kNextStatement);
      }
      assert(stack_depth_.IsBaseLine());
      if (cond == Condition::COND_INDETERMINATE) {
        EmitArgAt(CurrentSize(), arg_index);
      }
    }
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const DoWhileStatement* stmt) {
    ContinueTarget jump(this, stmt);
    const std::size_t start_index = CurrentSize();

    stmt->body()->Accept(this);  // STMT
    assert(stack_depth_.IsBaseLine());

    const std::size_t cond_index = CurrentSize();

    stmt->cond()->Accept(this);

    Emit<OP::POP_JUMP_IF_TRUE>(start_index);
    stack_depth_.Down();

    jump.EmitJumps(CurrentSize(), cond_index);

    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
    assert(stack_depth_.IsBaseLine());
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

    if (cond == Condition::COND_INDETERMINATE) {
      stmt->cond()->Accept(this);
    }

    if (stmt->body()->IsEffectiveStatement()) {
      if (cond == Condition::COND_INDETERMINATE) {
        const std::size_t arg_index = CurrentSize() + 1;

        Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
        stack_depth_.Down();

        stmt->body()->Accept(this);  // STMT
        assert(stack_depth_.IsBaseLine());

        Emit<OP::JUMP_ABSOLUTE>(start_index);
        EmitArgAt(CurrentSize(), arg_index);
      } else {
        assert(cond == Condition::COND_TRUE);

        stmt->body()->Accept(this);  // STMT
        assert(stack_depth_.IsBaseLine());

        Emit<OP::JUMP_ABSOLUTE>(start_index);
      }
      jump.EmitJumps(CurrentSize(), start_index);
      continuation_status_.ResolveJump(stmt);
    } else {
      if (cond == Condition::COND_INDETERMINATE) {
        Emit<OP::POP_JUMP_IF_TRUE>(start_index);
        stack_depth_.Down();
      } else {
        assert(cond == Condition::COND_TRUE);
        Emit<OP::JUMP_ABSOLUTE>(start_index);
      }
    }

    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const ForStatement* stmt) {
    ContinueTarget jump(this, stmt);

    if (const core::Maybe<const Statement> maybe = stmt->init()) {
      const Statement& init = *(maybe.Address());
      if (init.AsVariableStatement()) {
        init.Accept(this);
      } else {
        assert(init.AsExpressionStatement());
        // not evaluate as ExpressionStatement
        // because ExpressionStatement returns statement value
        init.AsExpressionStatement()->expr()->Accept(this);
        Emit<OP::POP_TOP>();
        stack_depth_.Down();
      }
    }

    const std::size_t start_index = CurrentSize();
    const core::Maybe<const Expression> cond = stmt->cond();
    std::size_t arg_index = 0;

    if (cond) {
      cond.Address()->Accept(this);
      arg_index = CurrentSize() + 1;
      Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
      stack_depth_.Down();
    }

    stmt->body()->Accept(this);  // STMT
    assert(stack_depth_.IsBaseLine());

    const std::size_t prev_next = CurrentSize();
    if (const core::Maybe<const Expression> next = stmt->next()) {
      next.Address()->Accept(this);
      Emit<OP::POP_TOP>();
      stack_depth_.Down();
    }

    Emit<OP::JUMP_ABSOLUTE>(start_index);

    if (cond) {
      EmitArgAt(CurrentSize(), arg_index);
    }

    jump.EmitJumps(CurrentSize(), prev_next);

    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const ForInStatement* stmt) {
    ContinueTarget jump(this, stmt);

    const Expression* lhs;
    if (const VariableStatement* var = stmt->each()->AsVariableStatement()) {
      const Declaration* decl = var->decls().front();
      Visit(decl);
      // Identifier
      lhs = decl->name();
    } else {
      // LeftHandSideExpression
      assert(stmt->each()->AsExpressionStatement());
      lhs = stmt->each()->AsExpressionStatement()->expr();
    }

    assert(lhs->IsValidLeftHandSide());

    {
      stmt->enumerable()->Accept(this);
      stack_depth_.BaseUp(1);
      PushLevelForIn();
      const std::size_t for_in_setup_jump = CurrentSize() + 1;
      Emit<OP::FORIN_SETUP>(0);  // dummy index
      const std::size_t start_index = CurrentSize();
      const std::size_t arg_index = CurrentSize() + 1;

      Emit<OP::FORIN_ENUMERATE>(0);  // dummy index
      stack_depth_.Up();

      // TODO(Constellation) abstraction...
      if (const Identifier* ident = lhs->AsIdentifier()) {
        // Identifier
        const uint32_t index = SymbolToNameIndex(ident->symbol());
        EmitStoreName(index);
      } else if (lhs->AsPropertyAccess()) {
        // PropertyAccess
        if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
          // IdentifierAccess
          ac->target()->Accept(this);
          Emit<OP::ROT_TWO>();
          const uint32_t index = SymbolToNameIndex(ac->key()->symbol());
          Emit<OP::STORE_PROP>(index);
          stack_depth_.Down();
        } else {
          // IndexAccess
          const IndexAccess& idx = *lhs->AsIndexAccess();
          idx.target()->Accept(this);
          const Expression& key = *idx.key();
          if (const StringLiteral* str = key.AsStringLiteral()) {
            Emit<OP::ROT_TWO>();
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, str->value()));
            Emit<OP::STORE_PROP>(index);
            stack_depth_.Down();
          } else if (const NumberLiteral* num = key.AsNumberLiteral()) {
            Emit<OP::ROT_TWO>();
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, num->value()));
            Emit<OP::STORE_PROP>(index);
            stack_depth_.Down();
          } else {
            Emit<OP::ROT_TWO>();
            idx.key()->Accept(this);
            Emit<OP::ROT_TWO>();
            Emit<OP::STORE_ELEMENT>();
            stack_depth_.Down(2);
          }
        }
      } else {
        // FunctionCall
        // ConstructorCall
        lhs->Accept(this);
        Emit<OP::ROT_TWO>();
        Emit<OP::STORE_CALL_RESULT>();
        stack_depth_.Down();
      }
      Emit<OP::POP_TOP>();
      stack_depth_.Down();

      stmt->body()->Accept(this);  // STMT

      Emit<OP::JUMP_ABSOLUTE>(start_index);
      const std::size_t end_index = CurrentSize();
      EmitArgAt(end_index, arg_index);
      EmitArgAt(end_index, for_in_setup_jump);

      stack_depth_.BaseDown(1);
      stack_depth_.Down();
      jump.EmitJumps(end_index, start_index);
    }
    PopLevel();
    Emit<OP::POP_TOP>();  // FORIN_CLEANUP

    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Insert(detail::kNextStatement);
    }
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const ContinueStatement* stmt) {
    const JumpEntry& entry = jump_table_[stmt->target()];
    for (uint16_t level = CurrentLevel(),
         last = std::get<0>(entry); level > last; --level) {
      const LevelEntry& le = level_stack_[level - 1];
      if (le.first == FINALLY) {
        const std::size_t finally_jump_index = CurrentSize() + 1;
        Emit<OP::JUMP_SUBROUTINE>(0);
        le.second->push_back(finally_jump_index);
      } else if (le.first == WITH) {
        Emit<OP::POP_ENV>();
      } else if (le.first == SUB) {
        Emit<OP::POP_N>(3);
      } else {
        // continue target for in ?
        if (last + 1 != level) {
          Emit<OP::POP_TOP>();
        }
      }
    }
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::JUMP_ABSOLUTE>(0);  // dummy
    std::get<2>(entry)->push_back(arg_index);

    continuation_status_.JumpTo(stmt->target());
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const BreakStatement* stmt) {
    if (!stmt->target() && stmt->label()) {
      // through
    } else {
      const JumpEntry& entry = jump_table_[stmt->target()];
      for (uint16_t level = CurrentLevel(),
           last = std::get<0>(entry); level > last; --level) {
        const LevelEntry& le = level_stack_[level - 1];
        if (le.first == FINALLY) {
          const std::size_t finally_jump_index = CurrentSize() + 1;
          Emit<OP::JUMP_SUBROUTINE>(0);
          le.second->push_back(finally_jump_index);
        } else if (le.first == WITH) {
          Emit<OP::POP_ENV>();
        } else if (le.first == SUB) {
          Emit<OP::POP_N>(3);
        } else {
          // break target for in ?
          if (last + 1 != level) {
            Emit<OP::POP_TOP>();
          }
        }
      }
      const std::size_t arg_index = CurrentSize() + 1;
      Emit<OP::JUMP_ABSOLUTE>(0);  // dummy
      std::get<1>(entry)->push_back(arg_index);
    }

    continuation_status_.JumpTo(stmt->target());
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const ReturnStatement* stmt) {
    if (const core::Maybe<const Expression> expr = stmt->expr()) {
      expr.Address()->Accept(this);
    } else {
      Emit<OP::PUSH_UNDEFINED>();
      stack_depth_.Up();
    }

    if (CurrentLevel() == 0) {
      Emit<OP::RETURN>();
      stack_depth_.Down();
    } else {
      stack_depth_.BaseUp(1);  // set return value in base

      // nested finally has found
      // set finally jump targets
      for (uint16_t level = CurrentLevel(); level > 0; --level) {
        const LevelEntry& le = level_stack_[level - 1];
        if (le.first == FINALLY) {
          const std::size_t finally_jump_index = CurrentSize() + 1;
          Emit<OP::JUMP_RETURN_HOOKED_SUBROUTINE>(0);
          le.second->push_back(finally_jump_index);
        } else if (le.first == WITH) {
          Emit<OP::POP_ENV>();
        } else if (le.first == SUB) {
          Emit<OP::ROT_FOUR>();
          Emit<OP::POP_N>(3);
        } else {
          Emit<OP::ROT_TWO>();
          Emit<OP::POP_TOP>();
        }
      }

      Emit<OP::RETURN>();
      stack_depth_.BaseDown(1);
      stack_depth_.Down();
    }

    continuation_status_.Kill();
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const WithStatement* stmt) {
    stmt->context()->Accept(this);
    Emit<OP::WITH_SETUP>();
    PushLevelWith();
    stack_depth_.Down();
    {
      DynamicEnvLevelCounter counter(this);
      current_variable_scope_ =
          std::shared_ptr<VariableScope>(
              new WithScope(current_variable_scope_));
      stmt->body()->Accept(this);  // STMT
      current_variable_scope_ = current_variable_scope_->upper();
    }
    PopLevel();
    Emit<OP::POP_ENV>();

    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const LabelledStatement* stmt) {
    stmt->body()->Accept(this);  // STMT
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const SwitchStatement* stmt) {
    BreakTarget jump(this, stmt);
    stmt->expr()->Accept(this);
    typedef SwitchStatement::CaseClauses CaseClauses;
    const CaseClauses& clauses = stmt->clauses();
    bool has_default_clause = false;
    std::vector<std::size_t> indexes(clauses.size());
    {
      std::vector<std::size_t>::iterator idx = indexes.begin();
      std::vector<std::size_t>::iterator default_it = indexes.end();
      for (CaseClauses::const_iterator it = clauses.begin(),
           last = clauses.end(); it != last; ++it, ++idx) {
        if (const core::Maybe<const Expression> expr = (*it)->expr()) {
          // case
          expr.Address()->Accept(this);
          *idx = CurrentSize() + 1;
          Emit<OP::SWITCH_CASE>(0);  // dummy index
          stack_depth_.Down();
        } else {
          // default
          default_it = idx;
        }
      }
      if (default_it != indexes.end()) {
        *default_it = CurrentSize() + 1;
        has_default_clause = true;
        Emit<OP::SWITCH_DEFAULT>(0);  // dummy index
      }
    }
    stack_depth_.Down();
    {
      std::vector<std::size_t>::const_iterator idx = indexes.begin();
      for (CaseClauses::const_iterator it = clauses.begin(),
           last = clauses.end(); it != last; ++it, ++idx) {
        EmitArgAt(CurrentSize(), *idx);
        const Statements& stmts = (*it)->body();
        for (Statements::const_iterator stmt_it = stmts.begin(),
             stmt_last = stmts.end(); stmt_it != stmt_last; ++stmt_it) {
          (*stmt_it)->Accept(this);
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

    continuation_status_.ResolveJump(stmt);
    if (continuation_status_.IsDeadStatement() && !has_default_clause) {
      continuation_status_.Insert(detail::kNextStatement);
    }
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const ThrowStatement* stmt) {
    stmt->expr()->Accept(this);
    Emit<OP::THROW>();
    stack_depth_.Down();

    continuation_status_.Kill();
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const TryStatement* stmt) {
    const std::size_t try_start = CurrentSize();
    const bool has_catch = stmt->catch_block();
    const bool has_finally = stmt->finally_block();
    TryTarget target(this, has_finally);
    stmt->body()->Accept(this);  // STMT
    if (has_finally) {
      const std::size_t finally_jump_index = CurrentSize() + 1;
      Emit<OP::JUMP_SUBROUTINE>(0);  // dummy index
      level_stack_[CurrentLevel() - 1].second->push_back(finally_jump_index);
    }
    const std::size_t label_index = CurrentSize();
    Emit<OP::JUMP_FORWARD>(0);  // dummy index

    std::size_t catch_return_label_index = 0;
    if (const core::Maybe<const Block> block = stmt->catch_block()) {
      if (continuation_status_.IsDeadStatement()) {
        continuation_status_.Insert(detail::kNextStatement);
      } else {
        continuation_status_.Insert(stmt);
      }
      code_->RegisterHandler<Handler::CATCH>(
          try_start,
          CurrentSize(),
          stack_depth_.GetStackBase(),
          dynamic_env_level());
      stack_depth_.Up();  // exception handler
      const Symbol catch_symbol = stmt->catch_name().Address()->symbol();
      Emit<OP::TRY_CATCH_SETUP>(SymbolToNameIndex(catch_symbol));
      PushLevelWith();
      stack_depth_.Down();
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
        const std::size_t finally_jump_index = CurrentSize() + 1;
        Emit<OP::JUMP_SUBROUTINE>(0);  // dummy index
        level_stack_[CurrentLevel() - 1].second->push_back(finally_jump_index);
      }
      catch_return_label_index = CurrentSize();
      Emit<OP::JUMP_FORWARD>(0);  // dummy index

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
          stack_depth_.GetStackBase(),
          dynamic_env_level());
      stack_depth_.BaseUp(3);
      stack_depth_.Up(3);  // JUMP_SUBROUTINE or exception handler
      target.EmitJumps(finally_start);

      PushLevelSub();
      block.Address()->Accept(this);  // STMT

      Emit<OP::RETURN_SUBROUTINE>();
      PopLevel();
      stack_depth_.BaseDown(3);
      stack_depth_.Down(3);  // RETURN_SUBROUTINE

      if (continuation_status_.Has(stmt)) {
        continuation_status_.Erase(stmt);
      } else {
        if (!continuation_status_.IsDeadStatement()) {
          continuation_status_.Kill();
        }
      }
    }
    // try last
    EmitArgAt(CurrentSize() - label_index, label_index + 1);
    // catch last
    if (has_catch) {
      EmitArgAt(CurrentSize() - catch_return_label_index,
                catch_return_label_index + 1);
    }

    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const DebuggerStatement* stmt) {
    Emit<OP::DEBUGGER>();
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const ExpressionStatement* stmt) {
    stmt->expr()->Accept(this);
    Emit<OP::POP_TOP_AND_RET>();
    stack_depth_.Down();
    assert(stack_depth_.IsBaseLine());
  }

  void Visit(const Assignment* assign) {
    using core::Token;
    DepthPoint point(&stack_depth_);
    const Token::Type token = assign->op();
    if (token == Token::TK_ASSIGN) {
      EmitAssign(*assign->left(), *assign->right());
    } else {
      const Expression& lhs = *assign->left();
      const Expression& rhs = *assign->right();
      assert(lhs.IsValidLeftHandSide());
      if (const Identifier* ident = lhs.AsIdentifier()) {
        // Identifier
        const uint32_t index = SymbolToNameIndex(ident->symbol());
        if (ident->symbol() == symbol::arguments()) {
          code_->set_code_has_arguments();
          // Emit<OP::PUSH_ARGUMENTS>();
          EmitLoadName(index);
          stack_depth_.Up();
        } else {
          EmitLoadName(index);
          stack_depth_.Up();
        }
        rhs.Accept(this);
        EmitAssignedBinaryOperation(token);
        EmitStoreName(index);
      } else if (lhs.AsPropertyAccess()) {
        // PropertyAccess
        if (const IdentifierAccess* ac = lhs.AsIdentifierAccess()) {
          // IdentifierAccess
          ac->target()->Accept(this);

          Emit<OP::DUP_TOP>();
          stack_depth_.Up();

          const uint32_t index = SymbolToNameIndex(ac->key()->symbol());
          Emit<OP::LOAD_PROP>(index);
          rhs.Accept(this);
          EmitAssignedBinaryOperation(token);
          Emit<OP::STORE_PROP>(index);
          stack_depth_.Down();
        } else {
          // IndexAccess
          const IndexAccess& idx = *lhs.AsIndexAccess();
          idx.target()->Accept(this);
          const Expression& key = *idx.key();
          if (const StringLiteral* str = key.AsStringLiteral()) {
            Emit<OP::DUP_TOP>();
            stack_depth_.Up();
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, str->value()));
            Emit<OP::LOAD_PROP>(index);
            rhs.Accept(this);
            EmitAssignedBinaryOperation(token);
            Emit<OP::STORE_PROP>(index);
            stack_depth_.Down();
          } else if (const NumberLiteral* num = key.AsNumberLiteral()) {
            Emit<OP::DUP_TOP>();
            stack_depth_.Up();
            const uint32_t index =
                SymbolToNameIndex(context::Intern(ctx_, num->value()));
            Emit<OP::LOAD_PROP>(index);
            rhs.Accept(this);
            EmitAssignedBinaryOperation(token);
            Emit<OP::STORE_PROP>(index);
            stack_depth_.Down();
          } else {
            key.Accept(this);
            Emit<OP::DUP_TWO>();
            stack_depth_.Up(2);
            Emit<OP::LOAD_ELEMENT>();
            stack_depth_.Down();
            rhs.Accept(this);
            EmitAssignedBinaryOperation(token);
            Emit<OP::STORE_ELEMENT>();
            stack_depth_.Down(2);
          }
        }
      } else {
        // FunctionCall
        // ConstructorCall
        lhs.Accept(this);
        Emit<OP::DUP_TOP>();
        stack_depth_.Up();
        rhs.Accept(this);
        EmitAssignedBinaryOperation(token);
        Emit<OP::STORE_CALL_RESULT>();
        stack_depth_.Down();
      }
    }
    point.LevelCheck(1);
  }

  void Visit(const BinaryOperation* binary) {
    using core::Token;
    DepthPoint point(&stack_depth_);
    const Token::Type token = binary->op();
    switch (token) {
      case Token::TK_LOGICAL_AND: {  // &&
        binary->left()->Accept(this);
        const std::size_t arg_index = CurrentSize() + 1;
        Emit<OP::JUMP_IF_FALSE_OR_POP>(0);  // dummy index
        stack_depth_.Down();
        binary->right()->Accept(this);
        EmitArgAt(CurrentSize(), arg_index);
        break;
      }

      case Token::TK_LOGICAL_OR: {  // ||
        binary->left()->Accept(this);
        const std::size_t arg_index = CurrentSize() + 1;
        Emit<OP::JUMP_IF_TRUE_OR_POP>(0);  // dummy index
        stack_depth_.Down();
        binary->right()->Accept(this);
        EmitArgAt(CurrentSize(), arg_index);
        break;
      }

      case Token::TK_ADD: {  // +
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_ADD>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_SUB: {  // -
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_SUBTRACT>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_SHR: {  // >>>
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_RSHIFT_LOGICAL>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_SAR: {  // >>
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_RSHIFT>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_SHL: {  // <<
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_LSHIFT>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_MUL: {  // *
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_MULTIPLY>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_DIV: {  // /
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_DIVIDE>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_MOD: {  // %
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_MODULO>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_LT: {  // <
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_LT>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_GT: {  // >
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_GT>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_LTE: {  // <=
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_LTE>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_GTE: {  // >=
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_GTE>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_INSTANCEOF: {  // instanceof
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_INSTANCEOF>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_IN: {  // in
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_IN>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_EQ: {  // ==
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_EQ>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_NE: {  // !=
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_NE>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_EQ_STRICT: {  // ===
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_STRICT_EQ>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_NE_STRICT: {  // !==
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_STRICT_NE>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_BIT_AND: {  // &
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_BIT_AND>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_BIT_XOR: {  // ^
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_BIT_XOR>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_BIT_OR: {  // |
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_BIT_OR>();
        stack_depth_.Down();
        break;
      }

      case Token::TK_COMMA: {  // ,
        binary->left()->Accept(this);
        Emit<OP::POP_TOP>();
        stack_depth_.Down();
        binary->right()->Accept(this);
        break;
      }

      default:
        UNREACHABLE();
    }
    point.LevelCheck(1);
  }

  void Visit(const ConditionalExpression* cond) {
    DepthPoint point(&stack_depth_);
    cond->cond()->Accept(this);
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
    stack_depth_.Down();
    cond->left()->Accept(this);
    stack_depth_.Down();
    const std::size_t second_label_index = CurrentSize();
    Emit<OP::JUMP_FORWARD>(0);  // dummy index
    EmitArgAt(CurrentSize(), arg_index);
    cond->right()->Accept(this);  // STMT
    EmitArgAt(CurrentSize() - second_label_index, second_label_index + 1);
    point.LevelCheck(1);
  }

  void Visit(const UnaryOperation* unary) {
    using core::Token;
    const Token::Type token = unary->op();
    DepthPoint point(&stack_depth_);
    switch (token) {
      case Token::TK_DELETE: {
        const Expression& expr = *unary->expr();
        if (expr.IsValidLeftHandSide()) {
          // Identifier
          // PropertyAccess
          // FunctionCall
          // ConstructorCall
          if (const Identifier* ident = expr.AsIdentifier()) {
            // DELETE_NAME_STRICT is already rejected in parser
            assert(!code_->strict());
            EmitDeleteName(SymbolToNameIndex(ident->symbol()));
            stack_depth_.Up();
          } else if (expr.AsPropertyAccess()) {
            if (const IdentifierAccess* ac = expr.AsIdentifierAccess()) {
              // IdentifierAccess
              ac->target()->Accept(this);
              const uint32_t index = SymbolToNameIndex(ac->key()->symbol());
              Emit<OP::DELETE_PROP>(index);
            } else {
              // IndexAccess
              EmitElement<OP::DELETE_PROP,
                          OP::DELETE_ELEMENT>(*expr.AsIndexAccess());
            }
          } else {
            expr.Accept(this);
            Emit<OP::DELETE_CALL_RESULT>();
          }
        } else {
          // other case is no effect
          // but accept expr
          expr.Accept(this);
          Emit<OP::POP_TOP>();
          stack_depth_.Down();
          Emit<OP::PUSH_TRUE>();
          stack_depth_.Up();
        }
        break;
      }

      case Token::TK_VOID: {
        unary->expr()->Accept(this);
        Emit<OP::POP_TOP>();
        stack_depth_.Down();
        Emit<OP::PUSH_UNDEFINED>();
        stack_depth_.Up();
        break;
      }

      case Token::TK_TYPEOF: {
        const Expression& expr = *unary->expr();
        if (const Identifier* ident = expr.AsIdentifier()) {
          // maybe Global Reference
          EmitTypeofName(SymbolToNameIndex(ident->symbol()));
          stack_depth_.Up();
        } else {
          unary->expr()->Accept(this);
          Emit<OP::TYPEOF>();
        }
        break;
      }

      case Token::TK_INC:
      case Token::TK_DEC: {
        const Expression& expr = *unary->expr();
        assert(expr.IsValidLeftHandSide());
        if (const Identifier* ident = expr.AsIdentifier()) {
          const uint32_t index = SymbolToNameIndex(ident->symbol());
          if (token == Token::TK_INC) {
            EmitIncrementName(index);
            stack_depth_.Up();
          } else {
            EmitDecrementName(index);
            stack_depth_.Up();
          }
        } else if (expr.AsPropertyAccess()) {
          if (const IdentifierAccess* ac = expr.AsIdentifierAccess()) {
            // IdentifierAccess
            ac->target()->Accept(this);
            const uint32_t index = SymbolToNameIndex(ac->key()->symbol());
            if (token == Token::TK_INC) {
              Emit<OP::INCREMENT_PROP>(index);
            } else {
              Emit<OP::DECREMENT_PROP>(index);
            }
          } else {
            // IndexAccess
            const IndexAccess& idxac = *expr.AsIndexAccess();
            if (token == Token::TK_INC) {
              EmitElement<OP::INCREMENT_PROP,
                          OP::INCREMENT_ELEMENT>(idxac);
            } else {
              EmitElement<OP::DECREMENT_PROP,
                          OP::DECREMENT_ELEMENT>(idxac);
            }
          }
        } else {
          expr.Accept(this);
          if (token == Token::TK_INC) {
            Emit<OP::INCREMENT_CALL_RESULT>();
          } else {
            Emit<OP::DECREMENT_CALL_RESULT>();
          }
        }
        break;
      }

      case Token::TK_ADD: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_POSITIVE>();
        break;
      }

      case Token::TK_SUB: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_NEGATIVE>();
        break;
      }

      case Token::TK_BIT_NOT: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_BIT_NOT>();
        break;
      }

      case Token::TK_NOT: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_NOT>();
        break;
      }

      default:
        UNREACHABLE();
    }
    point.LevelCheck(1);
  }

  void Visit(const PostfixExpression* postfix) {
    using core::Token;
    DepthPoint point(&stack_depth_);
    const Expression& expr = *postfix->expr();
    const Token::Type token = postfix->op();
    assert(expr.IsValidLeftHandSide());
    if (const Identifier* ident = expr.AsIdentifier()) {
      const uint32_t index = SymbolToNameIndex(ident->symbol());
      if (token == Token::TK_INC) {
        EmitPostfixIncrementName(index);
        stack_depth_.Up();
      } else {
        EmitPostfixDecrementName(index);
        stack_depth_.Up();
      }
    } else if (expr.AsPropertyAccess()) {
      if (const IdentifierAccess* ac = expr.AsIdentifierAccess()) {
        // IdentifierAccess
        ac->target()->Accept(this);
        const uint32_t index = SymbolToNameIndex(ac->key()->symbol());
        if (token == Token::TK_INC) {
          Emit<OP::POSTFIX_INCREMENT_PROP>(index);
        } else {
          Emit<OP::POSTFIX_DECREMENT_PROP>(index);
        }
      } else {
        // IndexAccess
        const IndexAccess& idxac = *expr.AsIndexAccess();
        if (token == Token::TK_INC) {
          EmitElement<OP::POSTFIX_INCREMENT_PROP,
                      OP::POSTFIX_INCREMENT_ELEMENT>(idxac);
        } else {
          EmitElement<OP::POSTFIX_DECREMENT_PROP,
                      OP::POSTFIX_DECREMENT_ELEMENT>(idxac);
        }
      }
    } else {
      expr.Accept(this);
      if (token == Token::TK_INC) {
        Emit<OP::POSTFIX_INCREMENT_CALL_RESULT>();
      } else {
        Emit<OP::POSTFIX_DECREMENT_CALL_RESULT>();
      }
    }
    point.LevelCheck(1);
  }

  void Visit(const StringLiteral* lit) {
    DepthPoint point(&stack_depth_);
    uint32_t i = 0;
    for (JSVals::const_iterator it = code_->constants_.begin(),
         last = code_->constants_.end(); it != last; ++it, ++i) {
      if (it->IsString()) {
        const JSString& str = *it->string();
        core::UStringPiece temp(*str.Flatten());
        if (temp.compare(lit->value()) == 0) {
          // duplicate constant pool
          Emit<OP::LOAD_CONST>(i);
          stack_depth_.Up();
          point.LevelCheck(1);
          return;
        }
      }
    }
    // new constant value
    Emit<OP::LOAD_CONST>(code_->constants_.size());
    stack_depth_.Up();
    code_->constants_.push_back(JSString::New(ctx_, lit->value()));
    point.LevelCheck(1);
  }

  void Visit(const NumberLiteral* lit) {
    DepthPoint point(&stack_depth_);
    uint32_t i = 0;
    const double val = lit->value();

    const int32_t i32 = static_cast<int32_t>(val);
    if (val == i32 && (i32 || !core::Signbit(val))) {
      // boxing int32_t
      Instruction inst(0u);
      inst.i32 = i32;
      Emit<OP::PUSH_INT32>(inst);
      stack_depth_.Up();
      point.LevelCheck(1);
      return;
    }

    const uint32_t ui32 = static_cast<uint32_t>(val);
    if (val == ui32 && (ui32 || !core::Signbit(val))) {
      // boxing uint32_t
      Emit<OP::PUSH_UINT32>(ui32);
      stack_depth_.Up();
      point.LevelCheck(1);
      return;
    }

    // double fall back
    for (JSVals::const_iterator it = code_->constants_.begin(),
         last = code_->constants_.end(); it != last; ++it, ++i) {
      if (it->IsNumber() && it->number() == val &&
          (static_cast<bool>(core::Signbit(it->number())) ==
           static_cast<bool>(core::Signbit(val)))) {
        // duplicate constant pool
        Emit<OP::LOAD_CONST>(i);
        stack_depth_.Up();
        point.LevelCheck(1);
        return;
      }
    }

    // new constant value
    Emit<OP::LOAD_CONST>(code_->constants_.size());
    stack_depth_.Up();
    code_->constants_.push_back(val);
    point.LevelCheck(1);
  }

  void Visit(const Identifier* lit) {
    // directlly extract value and set to top version
    DepthPoint point(&stack_depth_);
    const Symbol name = lit->symbol();
    const uint32_t index = SymbolToNameIndex(name);
    if (name == symbol::arguments()) {
      code_->set_code_has_arguments();
      // Emit<OP::PUSH_ARGUMENTS>();
      EmitLoadName(index);
      stack_depth_.Up();
    } else {
      EmitLoadName(index);
      stack_depth_.Up();
    }
    point.LevelCheck(1);
  }

  void Visit(const ThisLiteral* lit) {
    DepthPoint point(&stack_depth_);
    Emit<OP::PUSH_THIS>();
    stack_depth_.Up();
    point.LevelCheck(1);
  }

  void Visit(const NullLiteral* lit) {
    DepthPoint point(&stack_depth_);
    Emit<OP::PUSH_NULL>();
    stack_depth_.Up();
    point.LevelCheck(1);
  }

  void Visit(const TrueLiteral* lit) {
    DepthPoint point(&stack_depth_);
    Emit<OP::PUSH_TRUE>();
    stack_depth_.Up();
    point.LevelCheck(1);
  }

  void Visit(const FalseLiteral* lit) {
    DepthPoint point(&stack_depth_);
    Emit<OP::PUSH_FALSE>();
    stack_depth_.Up();
    point.LevelCheck(1);
  }

  void Visit(const RegExpLiteral* lit) {
    DepthPoint point(&stack_depth_);
    Emit<OP::LOAD_CONST>(code_->constants_.size());
    stack_depth_.Up();
    Emit<OP::BUILD_REGEXP>();
    code_->constants_.push_back(
        JSRegExp::New(ctx_, lit->value(), lit->regexp()));
    point.LevelCheck(1);
  }

  void Visit(const ArrayLiteral* lit) {
    typedef ArrayLiteral::MaybeExpressions Items;
    DepthPoint point(&stack_depth_);
    const Items& items = lit->items();
    Emit<OP::BUILD_ARRAY>(items.size());
    stack_depth_.Up();
    uint32_t current = 0;
    for (Items::const_iterator it = items.begin(),
         last = items.end(); it != last; ++it, ++current) {
      const core::Maybe<const Expression>& expr = *it;
      if (expr) {
        expr.Address()->Accept(this);
        Emit<OP::INIT_ARRAY_ELEMENT>(current);
        stack_depth_.Down();
      }
    }
    point.LevelCheck(1);
  }

  void Visit(const ObjectLiteral* lit) {
    using std::get;
    typedef ObjectLiteral::Properties Properties;
    DepthPoint point(&stack_depth_);
    Emit<OP::BUILD_OBJECT>();
    stack_depth_.Up();
    const Properties& properties = lit->properties();
    for (Properties::const_iterator it = properties.begin(),
         last = properties.end(); it != last; ++it) {
      const ObjectLiteral::Property& prop = *it;
      const ObjectLiteral::PropertyDescriptorType type(get<0>(prop));
      const Symbol name = get<1>(prop)->symbol();
      const uint32_t index = SymbolToNameIndex(name);
      get<2>(prop)->Accept(this);
      if (type == ObjectLiteral::DATA) {
        Emit<OP::STORE_OBJECT_DATA>(index);
        stack_depth_.Down();
      } else if (type == ObjectLiteral::GET) {
        Emit<OP::STORE_OBJECT_GET>(index);
        stack_depth_.Down();
      } else {
        Emit<OP::STORE_OBJECT_SET>(index);
        stack_depth_.Down();
      }
    }
    point.LevelCheck(1);
  }

  void Visit(const FunctionLiteral* lit) {
    DepthPoint point(&stack_depth_);
    Code* const code = new Code(ctx_, script_, *lit, core_, Code::FUNCTION);
    const uint32_t index = code_->codes_.size();
    code_->codes_.push_back(code);
    code_info_stack_.push_back(
        std::make_tuple(code, lit, current_variable_scope_));
    Emit<OP::MAKE_CLOSURE>(index);
    stack_depth_.Up();
    point.LevelCheck(1);
  }

  void Visit(const IdentifierAccess* prop) {
    DepthPoint point(&stack_depth_);
    prop->target()->Accept(this);
    const uint32_t index = SymbolToNameIndex(prop->key()->symbol());
    Emit<OP::LOAD_PROP>(index);
    point.LevelCheck(1);
  }

  void Visit(const IndexAccess* prop) {
    DepthPoint point(&stack_depth_);
    EmitElement<OP::LOAD_PROP,
                OP::LOAD_ELEMENT>(*prop);
    point.LevelCheck(1);
  }

  void Visit(const FunctionCall* call) {
    DepthPoint point(&stack_depth_);
    EmitCall<OP::CALL>(*call);
    point.LevelCheck(1);
  }

  void Visit(const ConstructorCall* call) {
    DepthPoint point(&stack_depth_);
    EmitCall<OP::CONSTRUCT>(*call);
    point.LevelCheck(1);
  }

  void Visit(const Declaration* decl) {
    DepthPoint point(&stack_depth_);
    const uint32_t index = SymbolToNameIndex(decl->name()->symbol());
    if (const core::Maybe<const Expression> expr = decl->expr()) {
      expr.Address()->Accept(this);
      EmitStoreName(index);
      Emit<OP::POP_TOP>();
      stack_depth_.Down();
    }
    point.LevelCheck(0);
  }

  void Visit(const CaseClause* dummy) { }

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

  template<OP::Type PropOP,
           OP::Type ElementOP>
  void EmitElement(const IndexAccess& prop) {
    prop.target()->Accept(this);
    const Expression& key = *prop.key();
    if (const StringLiteral* str = key.AsStringLiteral()) {
      const uint32_t index =
          SymbolToNameIndex(context::Intern(ctx_, str->value()));
      Emit<PropOP>(index);
    } else if (const NumberLiteral* num = key.AsNumberLiteral()) {
      const uint32_t index =
          SymbolToNameIndex(context::Intern(ctx_, num->value()));
      Emit<PropOP>(index);
    } else {
      prop.key()->Accept(this);
      Emit<ElementOP>();
      stack_depth_.Down();
    }
  }

  void EmitAssign(const Expression& lhs, const Expression& rhs) {
    assert(lhs.IsValidLeftHandSide());
    if (const Identifier* ident = lhs.AsIdentifier()) {
      // Identifier
      const uint32_t index = SymbolToNameIndex(ident->symbol());
      rhs.Accept(this);
      EmitStoreName(index);
    } else if (lhs.AsPropertyAccess()) {
      // PropertyAccess
      if (const IdentifierAccess* ac = lhs.AsIdentifierAccess()) {
        // IdentifierAccess
        ac->target()->Accept(this);
        rhs.Accept(this);
        const uint32_t index = SymbolToNameIndex(ac->key()->symbol());
        Emit<OP::STORE_PROP>(index);
        stack_depth_.Down();
      } else {
        // IndexAccess
        const IndexAccess& idx = *lhs.AsIndexAccess();
        idx.target()->Accept(this);
        const Expression& key = *idx.key();
        if (const StringLiteral* str = key.AsStringLiteral()) {
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, str->value()));
          rhs.Accept(this);
          Emit<OP::STORE_PROP>(index);
          stack_depth_.Down();
        } else if (const NumberLiteral* num = key.AsNumberLiteral()) {
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, num->value()));
          rhs.Accept(this);
          Emit<OP::STORE_PROP>(index);
          stack_depth_.Down();
        } else {
          idx.key()->Accept(this);
          rhs.Accept(this);
          Emit<OP::STORE_ELEMENT>();
          stack_depth_.Down(2);
        }
      }
    } else {
      // FunctionCall
      // ConstructorCall
      lhs.Accept(this);
      rhs.Accept(this);
      Emit<OP::STORE_CALL_RESULT>();
      stack_depth_.Down();
    }
  }

  template<OP::Type op, typename Call>
  void EmitCall(const Call& call) {
    bool direct_call_to_eval = false;
    const Expression& target = *call.target();
    if (target.IsValidLeftHandSide()) {
      if (const Identifier* ident = target.AsIdentifier()) {
        const uint32_t index = SymbolToNameIndex(ident->symbol());
        EmitCallName(index);
        stack_depth_.Up(2);
        if (op == OP::CALL && ident->symbol() == symbol::eval()) {
          direct_call_to_eval = true;
        }
      } else if (const PropertyAccess* prop = target.AsPropertyAccess()) {
        if (const IdentifierAccess* ac = prop->AsIdentifierAccess()) {
          // IdentifierAccess
          prop->target()->Accept(this);
          const uint32_t index = SymbolToNameIndex(ac->key()->symbol());
          Emit<OP::CALL_PROP>(index);
          stack_depth_.Up();
        } else {
          // IndexAccess
          // TODO(Constellation) this is patching ->Up()
          EmitElement<OP::CALL_PROP,
                      OP::CALL_ELEMENT>(*prop->AsIndexAccess());
          stack_depth_.Up();
        }
      } else {
        target.Accept(this);
        Emit<OP::CALL_CALL_RESULT>();
        stack_depth_.Up();
      }
    } else {
      target.Accept(this);
      Emit<OP::PUSH_UNDEFINED>();
      stack_depth_.Up();
    }

    const Expressions& args = call.args();
    for (Expressions::const_iterator it = args.begin(),
         last = args.end(); it != last; ++it) {
      (*it)->Accept(this);
    }

    if (direct_call_to_eval) {
      Emit<OP::EVAL>(args.size());
      stack_depth_.Down(args.size() + 1);
      code_->set_code_has_eval();
      current_variable_scope_->RecordEval();
    } else {
      Emit<op>(args.size());
      stack_depth_.Down(args.size() + 1);
    }
  }

  void EmitFunctionCode(const FunctionLiteral& lit,
                        Code* code,
                        std::shared_ptr<VariableScope> upper) {
    // preserve function scope nest count
    CodeContextPrologue(code);
    const Scope& scope = lit.scope();
    current_variable_scope_ =
        std::shared_ptr<VariableScope>(
            new FunctionScope(upper,
                              ctx_,
                              code,
                              data_,
                              scope));
    const std::size_t code_info_stack_size = code_info_stack_.size();

    {
      // function declarations
      typedef Scope::FunctionLiterals Functions;
      const Functions& functions = scope.function_declarations();
      for (Functions::const_iterator it = functions.begin(),
           last = functions.end(); it != last; ++it) {
        const FunctionLiteral* const func = *it;
        Visit(func);
        const Symbol sym = func->name().Address()->symbol();
        if (sym == symbol::arguments()) {
          code_->set_code_hiding_arguments();
        }
        const uint32_t index = SymbolToNameIndex(sym);
        EmitStoreName(index);
        Emit<OP::POP_TOP>();
        stack_depth_.Down();
      }
    }
    {
      // variables
      typedef Scope::Variables Variables;
      const Variables& vars = scope.variables();
      for (Variables::const_iterator it = vars.begin(),
           last = vars.end(); it != last; ++it) {
        const Symbol name = it->first->symbol();
        if (std::find(
                code_->varnames().begin(),
                code_->varnames().end(), name) == code_->varnames().end()) {
          code_->varnames_.push_back(name);
        }
      }
    }
    {
      // main
      const Statements& stmts = lit.body();
      for (Statements::const_iterator it = stmts.begin(),
           last = stmts.end(); it != last; ++it) {
        (*it)->Accept(this);
        if (continuation_status_.IsDeadStatement()) {
          break;
        }
      }
    }

    // epilogue
    Emit<OP::STOP_CODE>();
    CodeContextEpilogue(code);
    std::shared_ptr<VariableScope> target = current_variable_scope_;
    {
      // lazy code compile
      std::size_t code_info_stack_index = code_info_stack_size;
      for (Code::Codes::const_iterator it = code_->codes().begin(),
           last = code_->codes().end();
           it != last; ++it, ++code_info_stack_index) {
        const CodeInfo info = code_info_stack_[code_info_stack_index];
        assert(std::get<0>(info) == *it);
        EmitFunctionCode(*std::get<1>(info), *it, std::get<2>(info));
      }
    }
    current_variable_scope_ = target->upper();
    // clear code info stack
    code_info_stack_.erase(
        code_info_stack_.begin() + code_info_stack_size,
        code_info_stack_.end());
  }

  void EmitAssignedBinaryOperation(core::Token::Type token) {
    using core::Token;
    switch (token) {
      case Token::TK_ASSIGN_ADD: {  // +=
        Emit<OP::BINARY_ADD>();
        break;
      }

      case Token::TK_ASSIGN_SUB: {  // -=
        Emit<OP::BINARY_SUBTRACT>();
        break;
      }

      case Token::TK_ASSIGN_MUL: {  // *=
        Emit<OP::BINARY_MULTIPLY>();
        break;
      }

      case Token::TK_ASSIGN_MOD: {  // %=
        Emit<OP::BINARY_MODULO>();
        break;
      }

      case Token::TK_ASSIGN_DIV: {  // /=
        Emit<OP::BINARY_DIVIDE>();
        break;
      }

      case Token::TK_ASSIGN_SAR: {  // >>=
        Emit<OP::BINARY_RSHIFT>();
        break;
      }

      case Token::TK_ASSIGN_SHR: {  // >>>=
        Emit<OP::BINARY_RSHIFT_LOGICAL>();
        break;
      }

      case Token::TK_ASSIGN_SHL: {  // <<=
        Emit<OP::BINARY_LSHIFT>();
        break;
      }

      case Token::TK_ASSIGN_BIT_AND: {  // &=
        Emit<OP::BINARY_BIT_AND>();
        break;
      }

      case Token::TK_ASSIGN_BIT_OR: {  // |=
        Emit<OP::BINARY_BIT_OR>();
        break;
      }

      case Token::TK_ASSIGN_BIT_XOR: {  // ^=
        Emit<OP::BINARY_BIT_XOR>();
        break;
      }

      default:
        UNREACHABLE();
    }
    stack_depth_.Down();
  }

  void EmitLoadName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::LOAD_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitStoreName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::STORE_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitCallName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::CALL_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitIncrementName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::INCREMENT_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitPostfixIncrementName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::POSTFIX_INCREMENT_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitDecrementName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::DECREMENT_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitPostfixDecrementName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::POSTFIX_DECREMENT_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitTypeofName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::TYPEOF_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  void EmitDeleteName(uint32_t index) {
    const std::size_t point = data_->size();
    Emit<OP::DELETE_NAME>(index, 0, 0);
    current_variable_scope_->Lookup(code_->names_[index], point, code_);
  }

  std::size_t CurrentSize() const {
    return data_->size() - code_->start();
  }

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
    if (OP::IsNameLookupOP<op>::value &&
        code_->names()[arg1.value] == symbol::arguments()) {
      if (op == OP::STORE_NAME) {
        code_->set_code_has_arguments_assign();
      } else {
        code_->set_code_has_arguments();
      }
    }
  }

  void Emit(OP::Type op) {
    data_->push_back(op);
  }

  void Emit(OP::Type op, Instruction arg) {
    data_->push_back(op);
    data_->push_back(arg);
  }

  void EmitOPAt(OP::Type op, std::size_t index) {
    (*data_)[code_->start() + index] = op;
  }

  void EmitArgAt(Instruction arg, std::size_t index) {
    (*data_)[code_->start() + index] = arg;
  }

  void set_code(Code* code) {
    code_ = code;
  }

  Code* code() const {
    return code_;
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
    level_stack_.push_back(std::make_pair(FINALLY, vec));
  }

  void PushLevelWith() {
    level_stack_.push_back(
        std::make_pair(WITH, static_cast<std::vector<std::size_t>*>(NULL)));
  }

  void PushLevelForIn() {
    level_stack_.push_back(
        std::make_pair(FORIN, static_cast<std::vector<std::size_t>*>(NULL)));
  }

  void PushLevelSub() {
    level_stack_.push_back(
        std::make_pair(SUB, static_cast<std::vector<std::size_t>*>(NULL)));
  }

  void PopLevel() {
    level_stack_.pop_back();
  }

  void ClearJumpTable() {
    jump_table_.clear();
  }

  void ClearLevelStack() {
    level_stack_.clear();
  }

  void ClearStackDepth() {
    stack_depth_.Clear();
  }

  void ClearSymbolToIndexMap() {
    symbol_to_index_map_.clear();
  }

  void ClearContinuation() {
    continuation_status_.Clear();
  }

  Context* ctx_;
  Code* code_;
  CoreData* core_;
  Code::Data* data_;
  JSScript* script_;
  CodeInfoStack code_info_stack_;
  JumpTable jump_table_;
  LevelStack level_stack_;
  StackDepth stack_depth_;
  std::unordered_map<Symbol, uint32_t> symbol_to_index_map_;
  uint16_t dynamic_env_level_;
  ContinuationStatus continuation_status_;
  std::shared_ptr<VariableScope> current_variable_scope_;
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

inline Code* CompileEval(Context* ctx,
                         const FunctionLiteral& eval,
                         JSScript* script, JSEnv* variable, JSEnv* lexical) {
  Compiler compiler(ctx);
  return compiler.CompileEval(eval, script);
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_COMPILER_H_
