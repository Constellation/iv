#ifndef _IV_LV5_RAILGUN_COMPILER_H_
#define _IV_LV5_RAILGUN_COMPILER_H_
#include <algorithm>
#include <tr1/tuple>
#include <tr1/unordered_map>
#include "ast_visitor.h"
#include "noncopyable.h"
#include "static_assert.h"
#include "enable_if.h"
#include "lv5/specialized_ast.h"
#include "lv5/jsval.h"
#include "lv5/jsstring.h"
#include "lv5/jsregexp.h"
#include "lv5/gc_template.h"
#include "lv5/context_utils.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/op.h"
#include "lv5/railgun/code.h"

namespace iv {
namespace lv5 {
namespace railgun {

class Compiler
    : private core::Noncopyable<Compiler>,
      public AstVisitor {
 public:
  typedef std::tr1::tuple<uint16_t,
                          std::vector<std::size_t>*,
                          std::vector<std::size_t>*> JumpEntry;
  typedef std::tr1::unordered_map<
              const BreakableStatement*,
              JumpEntry> JumpTable;
  typedef std::vector<std::vector<std::size_t> > FinallyStack;

  explicit Compiler(Context* ctx)
    : ctx_(ctx),
      code_(NULL),
      script_(NULL),
      jump_table_(NULL),
      finally_stack_(),
      dynamic_env_level_(0) {
  }

  Code* Compile(const FunctionLiteral& global, JSScript* script) {
    script_ = script;
    Code* code = new Code(script_, global);
    {
      CodeContext code_context(this, code);
      EmitFunctionCode(global);
    }
    return code;
  }

  void set_code(Code* code) {
    code_ = code;
  }

  Code* code() const {
    return code_;
  }

  void set_jump_table(JumpTable* jump_table) {
    jump_table_ = jump_table;
  }

  JumpTable* jump_table() const {
    return jump_table_;
  }

  // try - catch - finally nest level
  // use for break / continue exile by executing finally block
  std::size_t CurrentLevel() const {
    return finally_stack_.size();
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
    jump_table_->insert(
        std::make_pair(
            stmt,
            std::tr1::make_tuple(
                CurrentLevel(),
                breaks,
                static_cast<std::vector<std::size_t>*>(NULL))));
  }

  void RegisterJumpTarget(const IterationStatement* stmt,
                          std::vector<std::size_t>* breaks,
                          std::vector<std::size_t>* continues) {
    jump_table_->insert(
        std::make_pair(
            stmt,
            std::tr1::make_tuple(
                CurrentLevel(),
                breaks,
                continues)));
  }

  void UnRegisterJumpTarget(const BreakableStatement* stmt) {
    jump_table_->erase(stmt);
  }

  void PushFinallyStack() {
    finally_stack_.push_back(FinallyStack::value_type());
  }

  void PopFinallyStack() {
    finally_stack_.pop_back();
  }

  const FinallyStack& finally_stack() const {
    return finally_stack_;
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
        has_finally_(has_finally) {
      if (has_finally_) {
        compiler_->PushFinallyStack();
      }
    }

    void EmitJumps(std::size_t finally_target) {
      assert(has_finally_);
      const FinallyStack& stack = compiler_->finally_stack();
      const std::vector<std::size_t>& vec = stack[stack.size() - 1];
      for (std::vector<std::size_t>::const_iterator it = vec.begin(),
           last = vec.end(); it != last; ++it) {
        compiler_->EmitArgAt(finally_target, *it);
      }
      compiler_->PopFinallyStack();
    }

   private:
    Compiler* compiler_;
    bool has_finally_;
  };

  class DynamicEnvLevelCounter : private core::Noncopyable<> {
   public:
    DynamicEnvLevelCounter(Compiler* compiler)
      : compiler_(compiler) {
      compiler_->DynamicEnvLevelUp();
    }
    ~DynamicEnvLevelCounter() {
      compiler_->DynamicEnvLevelDown();
    }
   private:
    Compiler* compiler_;
  };

  void Visit(const Block* block) {
    BreakTarget jump(this, block);
    const Statements& stmts = block->body();
    for (Statements::const_iterator it = stmts.begin(),
         last = stmts.end(); it != last; ++it) {
      (*it)->Accept(this);
    }
    jump.EmitJumps(CurrentSize());
  }

  void Visit(const FunctionStatement* stmt) {
    const FunctionLiteral& func = *stmt->function();
    assert(func.name());  // FunctionStatement must have name
    const uint16_t index = SymbolToNameIndex(func.name().Address()->symbol());
    Visit(&func);
    Emit<OP::STORE_NAME>(index);
    Emit<OP::POP_TOP>();
  }

  void Visit(const FunctionDeclaration* func) {
  }

  void Visit(const VariableStatement* var) {
    const Declarations& decls = var->decls();
    for (Declarations::const_iterator it = decls.begin(),
         last = decls.end(); it != last; ++it) {
      Visit(*it);
    }
  }

  void Visit(const EmptyStatement* stmt) {
  }

  void Visit(const IfStatement* stmt) {
    // TODO(Constellation)
    // if code like
    //    if (true) {
    //      print("OK");
    //    }
    // then, remove checker
    stmt->cond()->Accept(this);
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
    stmt->then_statement()->Accept(this);  // STMT
    if (const core::Maybe<const Statement> else_stmt = stmt->else_statement()) {
      const std::size_t second_label_index = CurrentSize();
      Emit<OP::JUMP_FORWARD>(0);  // dummy index
      EmitArgAt(CurrentSize(), arg_index);
      else_stmt.Address()->Accept(this);  // STMT
      EmitArgAt(CurrentSize() - second_label_index, second_label_index + 1);
    } else {
      EmitArgAt(CurrentSize(), arg_index);
    }
  }

  void Visit(const DoWhileStatement* stmt) {
    ContinueTarget jump(this, stmt);
    const std::size_t start_index = CurrentSize();
    stmt->body()->Accept(this);  // STMT
    const std::size_t cond_index = CurrentSize();
    stmt->cond()->Accept(this);
    Emit<OP::POP_JUMP_IF_TRUE>(start_index);
    jump.EmitJumps(CurrentSize(), cond_index);
  }

  void Visit(const WhileStatement* stmt) {
    ContinueTarget jump(this, stmt);
    const std::size_t start_index = CurrentSize();
    stmt->cond()->Accept(this);
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
    stmt->body()->Accept(this);  // STMT
    Emit<OP::JUMP_ABSOLUTE>(start_index);
    EmitArgAt(CurrentSize(), arg_index);
    jump.EmitJumps(CurrentSize(), start_index);
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
      }
    }
    const std::size_t start_index = CurrentSize();
    const core::Maybe<const Expression> cond = stmt->cond();
    std::size_t arg_index = 0;
    if (cond) {
      cond.Address()->Accept(this);
      arg_index = CurrentSize() + 1;
      Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
    }
    stmt->body()->Accept(this);
    if (const core::Maybe<const Expression> next = stmt->next()) {
      next.Address()->Accept(this);
      Emit<OP::POP_TOP>();
    }
    Emit<OP::JUMP_ABSOLUTE>(start_index);
    if (cond) {
      EmitArgAt(CurrentSize(), arg_index);
    }
    jump.EmitJumps(CurrentSize(), start_index);
  }

  void EmitAssign(const Expression& lhs, const Expression& rhs) {
    assert(lhs.IsValidLeftHandSide());
    if (const Identifier* ident = lhs.AsIdentifier()) {
      // Identifier
      const uint16_t index = SymbolToNameIndex(ident->symbol());
      rhs.Accept(this);
      Emit<OP::STORE_NAME>(index);
    } else if (lhs.AsPropertyAccess()) {
      // PropertyAccess
      if (const IdentifierAccess* ac = lhs.AsIdentifierAccess()) {
        // IdentifierAccess
        ac->target()->Accept(this);
        rhs.Accept(this);
        const uint16_t index = SymbolToNameIndex(ac->key()->symbol());
        Emit<OP::STORE_PROP>(index);
      } else {
        // IndexAccess
        const IndexAccess& idx = *lhs.AsIndexAccess();
        idx.target()->Accept(this);
        const Expression& key = *idx.key();
        if (const StringLiteral* str = key.AsStringLiteral()) {
          const uint16_t index =
              SymbolToNameIndex(context::Intern(ctx_, str->value()));
          rhs.Accept(this);
          Emit<OP::STORE_PROP>(index);
        } else if (const NumberLiteral* num = key.AsNumberLiteral()) {
          const uint16_t index =
              SymbolToNameIndex(context::Intern(ctx_, num->value()));
          rhs.Accept(this);
          Emit<OP::STORE_PROP>(index);
        } else {
          idx.key()->Accept(this);
          rhs.Accept(this);
          Emit<OP::STORE_ELEMENT>();
        }
      }
    } else {
      // FunctionCall
      // ConstructorCall
      lhs.Accept(this);
      rhs.Accept(this);
      Emit<OP::STORE_CALL_RESULT>();
    }
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
    stmt->enumerable()->Accept(this);
    Emit<OP::FORIN_SETUP>();
    const std::size_t start_index = CurrentSize();
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::FORIN_ENUMERATE>(0);  // dummy index
    lhs->Accept(this);
    stmt->body()->Accept(this);
    Emit<OP::JUMP_ABSOLUTE>(start_index);
    const std::size_t end_index = CurrentSize();
    EmitArgAt(end_index, arg_index);
    jump.EmitJumps(end_index, start_index);
  }

  void Visit(const ContinueStatement* stmt) {
    const JumpEntry& entry = (*jump_table_)[stmt->target()];
    for (uint16_t level = CurrentLevel(),
         last = std::tr1::get<0>(entry); level > last; --level) {
      const std::size_t finally_jump_index = CurrentSize() + 1;
      Emit<OP::JUMP_SUBROUTINE>(0);
      finally_stack_[level - 1].push_back(finally_jump_index);
    }
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::JUMP_ABSOLUTE>(0);  // dummy
    std::tr1::get<2>(entry)->push_back(arg_index);
  }

  void Visit(const BreakStatement* stmt) {
    const JumpEntry& entry = (*jump_table_)[stmt->target()];
    for (uint16_t level = CurrentLevel(),
         last = std::tr1::get<0>(entry); level > last; --level) {
      const std::size_t finally_jump_index = CurrentSize() + 1;
      Emit<OP::JUMP_SUBROUTINE>(0);
      finally_stack_[level - 1].push_back(finally_jump_index);
    }
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::JUMP_ABSOLUTE>(0);  // dummy
    std::tr1::get<1>(entry)->push_back(arg_index);
  }

  void Visit(const ReturnStatement* stmt) {
    if (const core::Maybe<const Expression> expr = stmt->expr()) {
      expr.Address()->Accept(this);
    } else {
      Emit<OP::PUSH_UNDEFINED>();
    }
    if (CurrentLevel() == 0) {
      Emit<OP::RETURN>();
    } else {
      Emit<OP::SET_RET_VALUE>();
      // nested finally has found
      // set finally jump targets
      for (uint16_t level = CurrentLevel(); level > 0; --level) {
        const std::size_t finally_jump_index = CurrentSize() + 1;
        Emit<OP::JUMP_SUBROUTINE>(0);
        finally_stack_[level - 1].push_back(finally_jump_index);
      }
      Emit<OP::RETURN_RET_VALUE>();
    }
  }

  void Visit(const WithStatement* stmt) {
    stmt->context()->Accept(this);
    Emit<OP::WITH_SETUP>();
    {
      DynamicEnvLevelCounter counter(this);
      stmt->body()->Accept(this);
    }
    Emit<OP::POP_ENV>();
  }

  void Visit(const LabelledStatement* stmt) {
    stmt->body()->Accept(this);
  }

  void Visit(const SwitchStatement* stmt) {
    BreakTarget jump(this, stmt);
    stmt->expr()->Accept(this);
    Emit<OP::SWITCH_SETUP>();
    typedef SwitchStatement::CaseClauses CaseClauses;
    const CaseClauses& clauses = stmt->clauses();
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
        } else {
          // default
          default_it = idx;
        }
      }
      if (default_it != indexes.end()) {
        *default_it = CurrentSize() + 1;
        Emit<OP::SWITCH_DEFAULT>(0);  // dummy index
      }
    }
    {
      std::vector<std::size_t>::const_iterator idx = indexes.begin();
      for (CaseClauses::const_iterator it = clauses.begin(),
           last = clauses.end(); it != last; ++it, ++idx) {
        EmitArgAt(CurrentSize(), *idx);
        const Statements& stmts = (*it)->body();
        for (Statements::const_iterator stmt_it = stmts.begin(),
             stmt_last = stmts.end(); stmt_it != stmt_last; ++stmt_it) {
          (*stmt_it)->Accept(this);
        }
      }
    }
    jump.EmitJumps(CurrentSize());
  }

  void Visit(const ThrowStatement* stmt) {
    stmt->expr()->Accept(this);
    Emit<OP::THROW>();
  }

  void Visit(const TryStatement* stmt) {
    const std::size_t try_start = CurrentSize();
    const bool has_catch = stmt->catch_block();
    const bool has_finally = stmt->finally_block();
    TryTarget target(this, has_finally);
    stmt->body()->Accept(this);
    if (has_finally) {
      const std::size_t finally_jump_index = CurrentSize() + 1;
      Emit<OP::JUMP_SUBROUTINE>(0);  // dummy index
      finally_stack_[CurrentLevel() - 1].push_back(finally_jump_index);
    }
    const std::size_t label_index = CurrentSize();
    Emit<OP::JUMP_FORWARD>(0);  // dummy index

    std::size_t catch_return_label_index = 0;
    if (const core::Maybe<const Block> block = stmt->catch_block()) {
      code_->RegisterHandler<Handler::CATCH>(try_start, CurrentSize(), CurrentLevel(), dynamic_env_level());
      Emit<OP::TRY_CATCH_SETUP>(
          SymbolToNameIndex(stmt->catch_name().Address()->symbol()));
      {
        DynamicEnvLevelCounter counter(this);
        block.Address()->Accept(this);
      }
      Emit<OP::POP_ENV>();
      if (has_finally) {
        const std::size_t finally_jump_index = CurrentSize() + 1;
        Emit<OP::JUMP_SUBROUTINE>(0);  // dummy index
        finally_stack_[CurrentLevel() - 1].push_back(finally_jump_index);
      }
      catch_return_label_index = CurrentSize();
      Emit<OP::JUMP_FORWARD>(0);  // dummy index
    }

    if (const core::Maybe<const Block> block = stmt->finally_block()) {
      const std::size_t finally_start = CurrentSize();
      code_->RegisterHandler<Handler::FINALLY>(try_start, finally_start, CurrentLevel(), dynamic_env_level());
      target.EmitJumps(finally_start);
      block.Address()->Accept(this);
      Emit<OP::RETURN_SUBROUTINE>();
    }
    // try last
    EmitArgAt(CurrentSize() - label_index, label_index + 1);
    // catch last
    if (has_catch) {
      EmitArgAt(CurrentSize() - catch_return_label_index,
                catch_return_label_index + 1);
    }
  }

  void Visit(const DebuggerStatement* stmt) {
  }

  void Visit(const ExpressionStatement* stmt) {
    stmt->expr()->Accept(this);
    Emit<OP::POP_TOP_AND_RET>();
  }

  void Visit(const Assignment* assign) {
    using core::Token;
    const Token::Type token = assign->op();
    if (token == Token::ASSIGN) {
      EmitAssign(*assign->left(), *assign->right());
    } else {
      const Expression& lhs = *assign->left();
      const Expression& rhs = *assign->right();
      assert(lhs.IsValidLeftHandSide());
      if (const Identifier* ident = lhs.AsIdentifier()) {
        // Identifier
        const uint16_t index = SymbolToNameIndex(ident->symbol());
        if (ident->symbol() == context::arguments_symbol(ctx_)) {
          code_->set_code_has_arguments();
          Emit<OP::PUSH_ARGUMENTS>();
        } else {
          Emit<OP::LOAD_NAME>(index);
        }
        rhs.Accept(this);
        EmitAssignedBinaryOperation(token);
        Emit<OP::STORE_NAME>(index);
      } else if (lhs.AsPropertyAccess()) {
        // PropertyAccess
        if (const IdentifierAccess* ac = lhs.AsIdentifierAccess()) {
          // IdentifierAccess
          ac->target()->Accept(this);
          Emit<OP::DUP_TOP>();
          const uint16_t index = SymbolToNameIndex(ac->key()->symbol());
          Emit<OP::LOAD_PROP>(index);
          rhs.Accept(this);
          EmitAssignedBinaryOperation(token);
          Emit<OP::STORE_PROP>(index);
        } else {
          // IndexAccess
          const IndexAccess& idx = *lhs.AsIndexAccess();
          idx.target()->Accept(this);
          const Expression& key = *idx.key();
          if (const StringLiteral* str = key.AsStringLiteral()) {
            Emit<OP::DUP_TOP>();
            const uint16_t index =
                SymbolToNameIndex(context::Intern(ctx_, str->value()));
            Emit<OP::LOAD_PROP>(index);
            rhs.Accept(this);
            EmitAssignedBinaryOperation(token);
            Emit<OP::STORE_PROP>(index);
          } else if (const NumberLiteral* num = key.AsNumberLiteral()) {
            Emit<OP::DUP_TOP>();
            const uint16_t index =
                SymbolToNameIndex(context::Intern(ctx_, num->value()));
            Emit<OP::LOAD_PROP>(index);
            rhs.Accept(this);
            EmitAssignedBinaryOperation(token);
            Emit<OP::STORE_PROP>(index);
          } else {
            key.Accept(this);
            Emit<OP::DUP_TWO>();
            Emit<OP::LOAD_ELEMENT>();
            rhs.Accept(this);
            EmitAssignedBinaryOperation(token);
            Emit<OP::STORE_ELEMENT>();
          }
        }
      } else {
        // FunctionCall
        // ConstructorCall
        lhs.Accept(this);
        Emit<OP::DUP_TOP>();
        rhs.Accept(this);
        EmitAssignedBinaryOperation(token);
        Emit<OP::STORE_CALL_RESULT>();
      }
    }
  }

  void EmitAssignedBinaryOperation(core::Token::Type token) {
    using core::Token;
    switch (token) {
      case Token::ASSIGN_ADD: {  // +=
        Emit<OP::BINARY_ADD>();
        break;
      }

      case Token::ASSIGN_SUB: {  // -=
        Emit<OP::BINARY_SUBTRACT>();
        break;
      }

      case Token::ASSIGN_MUL: {  // *=
        Emit<OP::BINARY_MULTIPLY>();
        break;
      }

      case Token::ASSIGN_MOD: {  // %=
        Emit<OP::BINARY_MODULO>();
        break;
      }

      case Token::ASSIGN_DIV: {  // /=
        Emit<OP::BINARY_DIVIDE>();
        break;
      }

      case Token::ASSIGN_SAR: {  // >>=
        Emit<OP::BINARY_RSHIFT>();
        break;
      }

      case Token::ASSIGN_SHR: {  // >>>=
        Emit<OP::BINARY_RSHIFT_LOGICAL>();
        break;
      }

      case Token::ASSIGN_SHL: {  // <<=
        Emit<OP::BINARY_LSHIFT>();
        break;
      }

      case Token::ASSIGN_BIT_AND: {  // &=
        Emit<OP::BINARY_BIT_AND>();
        break;
      }

      case Token::ASSIGN_BIT_OR: {  // |=
        Emit<OP::BINARY_BIT_OR>();
        break;
      }

      case Token::ASSIGN_BIT_XOR: {  // ^=
        Emit<OP::BINARY_BIT_XOR>();
        break;
      }

      default:
        UNREACHABLE();
    }
  }

  void Visit(const BinaryOperation* binary) {
    using core::Token;
    const Token::Type token = binary->op();
    switch (token) {
      case Token::LOGICAL_AND: {  // &&
        binary->left()->Accept(this);
        const std::size_t arg_index = CurrentSize() + 1;
        Emit<OP::JUMP_IF_FALSE_OR_POP>(0);  // dummy index
        binary->right()->Accept(this);
        EmitArgAt(CurrentSize(), arg_index);
        return;
      }

      case Token::LOGICAL_OR: {  // ||
        binary->left()->Accept(this);
        const std::size_t arg_index = CurrentSize() + 1;
        Emit<OP::JUMP_IF_TRUE_OR_POP>(0);  // dummy index
        binary->right()->Accept(this);
        EmitArgAt(CurrentSize(), arg_index);
        return;
      }

      case Token::ADD: {  // +
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_ADD>();
        return;
      }

      case Token::SUB: {  // -
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_SUBTRACT>();
        return;
      }

      case Token::SHR: {  // >>>
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_RSHIFT_LOGICAL>();
        return;
      }

      case Token::SAR: {  // >>
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_RSHIFT>();
        return;
      }

      case Token::SHL: {  // <<
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_LSHIFT>();
        return;
      }

      case Token::MUL: {  // *
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_MULTIPLY>();
        return;
      }

      case Token::DIV: {  // /
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_DIVIDE>();
        return;
      }

      case Token::MOD: {  // %
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_MODULO>();
        return;
      }

      case Token::LT: {  // <
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_LT>();
        return;
      }

      case Token::GT: {  // >
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_GT>();
        return;
      }

      case Token::LTE: {  // <=
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_LTE>();
        return;
      }

      case Token::GTE: {  // >=
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_GTE>();
        return;
      }

      case Token::INSTANCEOF: {  // instanceof
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_INSTANCEOF>();
        return;
      }

      case Token::IN: {  // in
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_IN>();
        return;
      }

      case Token::EQ: {  // ==
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_EQ>();
        return;
      }

      case Token::NE: {  // !=
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_NE>();
        return;
      }

      case Token::EQ_STRICT: {  // ===
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_STRICT_EQ>();
        return;
      }

      case Token::NE_STRICT: {  // !==
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_STRICT_NE>();
        return;
      }

      case Token::BIT_AND: {  // &
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_BIT_AND>();
        return;
      }

      case Token::BIT_XOR: {  // ^
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_BIT_XOR>();
        return;
      }

      case Token::BIT_OR: {  // |
        binary->left()->Accept(this);
        binary->right()->Accept(this);
        Emit<OP::BINARY_BIT_OR>();
        return;
      }

      case Token::COMMA: {  // ,
        binary->left()->Accept(this);
        Emit<OP::POP_TOP>();
        binary->right()->Accept(this);
        return;
      }

      default:
        UNREACHABLE();
    }
  }

  void Visit(const ConditionalExpression* cond) {
    cond->cond()->Accept(this);
    const std::size_t arg_index = CurrentSize() + 1;
    Emit<OP::POP_JUMP_IF_FALSE>(0);  // dummy index
    cond->left()->Accept(this);
    const std::size_t second_label_index = CurrentSize();
    Emit<OP::JUMP_FORWARD>(0);  // dummy index
    EmitArgAt(CurrentSize(), arg_index);
    cond->right()->Accept(this);  // STMT
    EmitArgAt(CurrentSize() - second_label_index, second_label_index + 1);
  }

  void Visit(const UnaryOperation* unary) {
    using core::Token;
    const Token::Type token = unary->op();
    switch (token) {
      case Token::DELETE: {
        const Expression& expr = *unary->expr();
        if (expr.IsValidLeftHandSide()) {
          // Identifier
          // PropertyAccess
          // FunctionCall
          // ConstructorCall
          if (const Identifier* ident = expr.AsIdentifier()) {
            // DELETE_NAME_STRICT is already rejected in parser
            assert(!code_->strict());
            Emit<OP::DELETE_NAME>(SymbolToNameIndex(ident->symbol()));
          } else if (expr.AsPropertyAccess()) {
            if (const IdentifierAccess* ac = expr.AsIdentifierAccess()) {
              // IdentifierAccess
              ac->target()->Accept(this);
              const uint16_t index = SymbolToNameIndex(ac->key()->symbol());
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
          Emit<OP::PUSH_TRUE>();
        }
        return;
      }

      case Token::VOID: {
        unary->expr()->Accept(this);
        Emit<OP::POP_TOP>();
        Emit<OP::PUSH_UNDEFINED>();
        return;
      }

      case Token::TYPEOF: {
        const Expression& expr = *unary->expr();
        if (const Identifier* ident = expr.AsIdentifier()) {
          // maybe Global Reference
          Emit<OP::TYPEOF_NAME>(SymbolToNameIndex(ident->symbol()));
        } else {
          unary->expr()->Accept(this);
          Emit<OP::TYPEOF>();
        }
        return;
      }

      case Token::INC:
      case Token::DEC: {
        const Expression& expr = *unary->expr();
        assert(expr.IsValidLeftHandSide());
        if (const Identifier* ident = expr.AsIdentifier()) {
          const uint16_t index = SymbolToNameIndex(ident->symbol());
          if (token == Token::INC) {
            Emit<OP::INCREMENT_NAME>(index);
          } else {
            Emit<OP::DECREMENT_NAME>(index);
          }
        } else if (expr.AsPropertyAccess()) {
          if (const IdentifierAccess* ac = expr.AsIdentifierAccess()) {
            // IdentifierAccess
            const uint16_t index = SymbolToNameIndex(ac->key()->symbol());
            if (token == Token::INC) {
              Emit<OP::INCREMENT_PROP>(index);
            } else {
              Emit<OP::DECREMENT_PROP>(index);
            }
          } else {
            // IndexAccess
            const IndexAccess& idxac = *expr.AsIndexAccess();
            if (token == Token::INC) {
              EmitElement<OP::INCREMENT_PROP,
                          OP::INCREMENT_ELEMENT>(idxac);
            } else {
              EmitElement<OP::DECREMENT_PROP,
                          OP::DECREMENT_ELEMENT>(idxac);
            }
          }
        } else {
          expr.Accept(this);
          if (token == Token::INC) {
            Emit<OP::INCREMENT_CALL_RESULT>();
          } else {
            Emit<OP::DECREMENT_CALL_RESULT>();
          }
        }
        return;
      }

      case Token::ADD: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_POSITIVE>();
        return;
      }

      case Token::SUB: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_NEGATIVE>();
        return;
      }

      case Token::BIT_NOT: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_BIT_NOT>();
        return;
      }

      case Token::NOT: {
        unary->expr()->Accept(this);
        Emit<OP::UNARY_NOT>();
        return;
      }

      default:
        UNREACHABLE();
    }
  }

  void Visit(const PostfixExpression* postfix) {
    using core::Token;
    const Expression& expr = *postfix->expr();
    const Token::Type token = postfix->op();
    assert(expr.IsValidLeftHandSide());
    if (const Identifier* ident = expr.AsIdentifier()) {
      const uint16_t index = SymbolToNameIndex(ident->symbol());
      if (token == Token::INC) {
        Emit<OP::POSTFIX_INCREMENT_NAME>(index);
      } else {
        Emit<OP::POSTFIX_DECREMENT_NAME>(index);
      }
    } else if (expr.AsPropertyAccess()) {
      if (const IdentifierAccess* ac = expr.AsIdentifierAccess()) {
        // IdentifierAccess
        const uint16_t index = SymbolToNameIndex(ac->key()->symbol());
        if (token == Token::INC) {
          Emit<OP::POSTFIX_INCREMENT_PROP>(index);
        } else {
          Emit<OP::POSTFIX_DECREMENT_PROP>(index);
        }
      } else {
        // IndexAccess
        const IndexAccess& idxac = *expr.AsIndexAccess();
        if (token == Token::INC) {
          EmitElement<OP::POSTFIX_INCREMENT_PROP,
                      OP::POSTFIX_INCREMENT_ELEMENT>(idxac);
        } else {
          EmitElement<OP::POSTFIX_DECREMENT_PROP,
                      OP::POSTFIX_DECREMENT_ELEMENT>(idxac);
        }
      }
    } else {
      expr.Accept(this);
      if (token == Token::INC) {
        Emit<OP::POSTFIX_INCREMENT_CALL_RESULT>();
      } else {
        Emit<OP::POSTFIX_DECREMENT_CALL_RESULT>();
      }
    }
  }

  void Visit(const StringLiteral* lit) {
    uint16_t i = 0;
    for (JSVals::const_iterator it = code_->constants_.begin(),
         last = code_->constants_.end(); it != last; ++it, ++i) {
      if (it->IsString()) {
        const JSString& str = *it->string();
        if (str.compare(lit->value()) == 0) {
          // duplicate constant pool
          Emit<OP::LOAD_CONST>(i);
          return; }
      }
    }
    // new constant value
    Emit<OP::LOAD_CONST>(code_->constants_.size());
    code_->constants_.push_back(JSString::New(ctx_, lit->value()));
  }

  void Visit(const NumberLiteral* lit) {
    uint16_t i = 0;
    for (JSVals::const_iterator it = code_->constants_.begin(),
         last = code_->constants_.end(); it != last; ++it, ++i) {
      if (it->IsNumber() && it->number() == lit->value()) {
        // duplicate constant pool
        Emit<OP::LOAD_CONST>(i);
        return;
      }
    }
    // new constant value
    Emit<OP::LOAD_CONST>(code_->constants_.size());
    code_->constants_.push_back(lit->value());
  }

  void Visit(const Identifier* lit) {
    // directlly extract value and set to top version
    const Symbol name = lit->symbol();
    if (name == context::arguments_symbol(ctx_)) {
      code_->set_code_has_arguments();
      Emit<OP::PUSH_ARGUMENTS>();
      return;
    }
    const uint16_t index = SymbolToNameIndex(name);
    Emit<OP::LOAD_NAME>(index);
  }

  void Visit(const ThisLiteral* lit) {
    Emit<OP::PUSH_THIS>();
  }

  void Visit(const NullLiteral* lit) {
    Emit<OP::PUSH_NULL>();
  }

  void Visit(const TrueLiteral* lit) {
    Emit<OP::PUSH_TRUE>();
  }

  void Visit(const FalseLiteral* lit) {
    Emit<OP::PUSH_FALSE>();
  }

  void Visit(const RegExpLiteral* lit) {
    Emit<OP::LOAD_CONST>(code_->constants_.size());
    Emit<OP::BUILD_REGEXP>();
    code_->constants_.push_back(
        JSRegExp::New(ctx_, lit->value(), lit->regexp()));
  }

  void Visit(const ArrayLiteral* lit) {
    typedef ArrayLiteral::MaybeExpressions Items;
    const Items& items = lit->items();
    Emit<OP::BUILD_ARRAY>(items.size());
    uint16_t current = 0;
    for (Items::const_iterator it = items.begin(),
         last = items.end(); it != last; ++it, ++current) {
      const core::Maybe<const Expression>& expr = *it;
      if (expr) {
        expr.Address()->Accept(this);
        Emit<OP::INIT_ARRAY_ELEMENT>(current);
      }
    }
  }

  void Visit(const ObjectLiteral* lit) {
    using std::tr1::get;
    typedef ObjectLiteral::Properties Properties;
    Emit<OP::BUILD_OBJECT>();
    const Properties& properties = lit->properties();
    for (Properties::const_iterator it = properties.begin(),
         last = properties.end(); it != last; ++it) {
      const ObjectLiteral::Property& prop = *it;
      const ObjectLiteral::PropertyDescriptorType type(get<0>(prop));
      const Symbol name = get<1>(prop)->symbol();
      const uint16_t index = SymbolToNameIndex(name);
      get<2>(prop)->Accept(this);
      if (type == ObjectLiteral::DATA) {
        Emit<OP::STORE_OBJECT_DATA>(index);
      } else if (type == ObjectLiteral::GET) {
        Emit<OP::STORE_OBJECT_GET>(index);
      } else {
        Emit<OP::STORE_OBJECT_SET>(index);
      }
    }
  }

  class CodeContext : private core::Noncopyable<> {
   public:
    CodeContext(Compiler* compiler, Code* code)
      : compiler_(compiler),
        jump_table_(),
        prev_code_(compiler_->code()),
        prev_jump_table_(compiler_->jump_table()),
        prev_dynamic_env_level_(compiler_->dynamic_env_level()) {
      compiler_->set_code(code);
      compiler_->set_jump_table(&jump_table_);
      compiler_->set_dynamic_env_level(0);
    }

    ~CodeContext() {
      compiler_->set_code(prev_code_);
      compiler_->set_jump_table(prev_jump_table_);
      compiler_->set_dynamic_env_level(prev_dynamic_env_level_);
    }
   private:
    Compiler* compiler_;
    JumpTable jump_table_;
    Code* prev_code_;
    JumpTable* prev_jump_table_;
    uint16_t prev_dynamic_env_level_;
  };

  void Visit(const FunctionLiteral* lit) {
    Code* const code = new Code(script_, *lit);
    const uint16_t index = code_->codes_.size();
    code_->codes_.push_back(code);
    {
      CodeContext code_context(this, code);
      EmitFunctionCode(*lit);
    }
    Emit<OP::MAKE_CLOSURE>(index);
  }

  void Visit(const IdentifierAccess* prop) {
    prop->target()->Accept(this);
    const uint16_t index = SymbolToNameIndex(prop->key()->symbol());
    Emit<OP::LOAD_PROP>(index);
  }

  template<OP::Type PropOP,
           OP::Type ElementOP>
  void EmitElement(const IndexAccess& prop) {
    prop.target()->Accept(this);
    const Expression& key = *prop.key();
    if (const StringLiteral* str = key.AsStringLiteral()) {
      const uint16_t index =
          SymbolToNameIndex(context::Intern(ctx_, str->value()));
      Emit<PropOP>(index);
    } else if (const NumberLiteral* num = key.AsNumberLiteral()) {
      const uint16_t index =
          SymbolToNameIndex(context::Intern(ctx_, num->value()));
      Emit<PropOP>(index);
    } else {
      prop.key()->Accept(this);
      Emit<ElementOP>();
    }
  }

  void Visit(const IndexAccess* prop) {
    EmitElement<OP::LOAD_PROP,
                OP::LOAD_ELEMENT>(*prop);
  }

  template<OP::Type op, typename Call>
  void EmitCall(const Call& call) {
    bool direct_call_to_eval = false;
    const Expression& target = *call.target();
    if (target.IsValidLeftHandSide()) {
      if (const Identifier* ident = target.AsIdentifier()) {
        const uint16_t index = SymbolToNameIndex(ident->symbol());
        Emit<OP::CALL_NAME>(index);
        if (op == OP::CALL && ident->symbol() == context::eval_symbol(ctx_)) {
          direct_call_to_eval = true;
        }
      } else if (const PropertyAccess* prop = target.AsPropertyAccess()) {
        if (const IdentifierAccess* ac = prop->AsIdentifierAccess()) {
          // IdentifierAccess
          prop->target()->Accept(this);
          const uint16_t index = SymbolToNameIndex(ac->key()->symbol());
          Emit<OP::CALL_PROP>(index);
        } else {
          // IndexAccess
          EmitElement<OP::CALL_PROP,
                      OP::CALL_ELEMENT>(*prop->AsIndexAccess());
        }
      } else {
        target.Accept(this);
        Emit<OP::CALL_CALL_RESULT>();
      }
    } else {
      target.Accept(this);
      Emit<OP::PUSH_UNDEFINED>();
    }

    const Expressions& args = call.args();
    for (Expressions::const_iterator it = args.begin(),
         last = args.end(); it != last; ++it) {
      (*it)->Accept(this);
    }

    if (direct_call_to_eval) {
      Emit<OP::EVAL>(args.size());
      code_->set_code_has_eval();
    } else {
      Emit<op>(args.size());
    }
  }

  void Visit(const FunctionCall* call) {
    EmitCall<OP::CALL>(*call);
  }

  void Visit(const ConstructorCall* call) {
    EmitCall<OP::CONSTRUCT>(*call);
  }

  void Visit(const Declaration* decl) {
    const uint16_t index = SymbolToNameIndex(decl->name()->symbol());
    if (const core::Maybe<const Expression> expr = decl->expr()) {
      expr.Address()->Accept(this);
      Emit<OP::STORE_NAME>(index);
      Emit<OP::POP_TOP>();
    }
  }

  void Visit(const CaseClause* dummy) { }

  uint16_t SymbolToNameIndex(const Symbol& sym) {
    const Code::Names::const_iterator it =
        std::find(code_->names_.begin(), code_->names_.end(), sym);
    if (it != code_->names_.end()) {
      return std::distance<
          Code::Names::const_iterator>(code_->names_.begin(), it);
    }
    code_->names_.push_back(sym);
    return code_->names_.size() - 1;
  }

  void EmitFunctionCode(const FunctionLiteral& lit) {
    const Scope& scope = lit.scope();
    {
      // function declarations
      typedef Scope::FunctionLiterals Functions;
      const Functions& functions = scope.function_declarations();
      for (Functions::const_iterator it = functions.begin(),
           last = functions.end(); it != last; ++it) {
        const FunctionLiteral* const func = *it;
        Visit(func);
        const uint16_t index =
            SymbolToNameIndex(func->name().Address()->symbol());
        Emit<OP::STORE_NAME>(index);
        Emit<OP::POP_TOP>();
      }
    }
    {
      // variables
      typedef Scope::Variables Variables;
      const Variables& vars = scope.variables();
      for (Variables::const_iterator it = vars.begin(),
           last = vars.end(); it != last; ++it) {
        const Symbol name = it->first->symbol();
        SymbolToNameIndex(name);
        code_->varnames_.push_back(name);
      }
    }
    {
      // main
      const Statements& stmts = lit.body();
      for (Statements::const_iterator it = stmts.begin(),
           last = stmts.end(); it != last; ++it) {
        (*it)->Accept(this);
      }
    }
    Emit<OP::PUSH_UNDEFINED>();
    Emit<OP::RETURN>();
    Emit<OP::STOP_CODE>();
  }

  std::size_t CurrentSize() const {
    return code_->data_.size();
  }

  template<OP::Type op>
  void Emit() {
    IV_STATIC_ASSERT(op < OP::HAVE_ARGUMENT);
    code_->data_.push_back(op);
  }

  template<OP::Type op>
  void Emit(uint16_t arg,
            typename disable_if<OP::IsNameLookupOP<op> >::type* = 0) {
    IV_STATIC_ASSERT(OP::HAVE_ARGUMENT < op);
    code_->data_.push_back(op);
    code_->data_.push_back(arg & 0xff);
    code_->data_.push_back(arg >> 8);
  }

  template<OP::Type op>
  void Emit(uint16_t arg,
            typename enable_if<OP::IsNameLookupOP<op> >::type* = 0) {
    IV_STATIC_ASSERT(OP::HAVE_ARGUMENT < op);
    code_->data_.push_back(op);
    code_->data_.push_back(arg & 0xff);
    code_->data_.push_back(arg >> 8);
    if (code_->names()[arg] == context::arguments_symbol(ctx_)) {
      code_->set_code_has_arguments();
    }
  }

  void Emit(OP::Type op) {
    code_->data_.push_back(op);
  }

  void Emit(OP::Type op, uint16_t arg) {
    code_->data_.push_back(op);
    code_->data_.push_back(arg & 0xff);
    code_->data_.push_back(arg >> 8);
  }

  void EmitOPAt(OP::Type op, std::size_t index) {
    code_->data_[index] = op;
  }

  void EmitArgAt(uint16_t arg, std::size_t index) {
    code_->data_[index] = (arg & 0xff);
    code_->data_[index + 1] = (arg >> 8);
  }

  Context* ctx_;
  Code* code_;
  JSScript* script_;
  JumpTable* jump_table_;
  FinallyStack finally_stack_;
  uint16_t dynamic_env_level_;
};

inline Code* Compile(Context* ctx, const FunctionLiteral& global, JSScript* script) {
  Compiler compiler(ctx);
  return compiler.Compile(global, script);
}

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_COMPILER_H_
