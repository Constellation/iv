#ifndef IV_LV5_RAILGUN_COMPILER_STATEMENT_H_
#define IV_LV5_RAILGUN_COMPILER_STATEMENT_H_
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Compiler::BreakTarget : private core::Noncopyable<> {
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
    for (Jump::Targets::const_iterator it = breaks_.begin(),
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
  Jump::Targets breaks_;
};

inline void Compiler::Visit(const Block* block) {
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

inline void Compiler::Visit(const FunctionStatement* stmt) {
  const FunctionLiteral* func = stmt->function();
  assert(func->name());  // FunctionStatement must have name
  const Symbol name = func->name().Address()->symbol();
  const uint32_t index = SymbolToNameIndex(name);
  if (IsUsedReference(index)) {
    if (RegisterID local = GetLocal(name)) {
      EmitExpressionToDest(func, local);
    } else {
      RegisterID tmp = EmitExpression(func);
      EmitStore(name, tmp);
    }
  }
}

inline void Compiler::Visit(const FunctionDeclaration* func) { }

inline void Compiler::Visit(const Declaration* decl) {
  if (const core::Maybe<const Expression> maybe = decl->expr()) {
    const Symbol sym = decl->name()->symbol();
    const uint32_t index = SymbolToNameIndex(sym);
    const Expression* expr = maybe.Address();
    if (IsUsedReference(index) || !Condition::NoSideEffect(expr)) {
      if (RegisterID local = GetLocal(sym)) {
        EmitExpressionToDest(expr, local);
      } else {
        RegisterID tmp = EmitExpression(expr);
        EmitStore(sym, tmp);
      }
    }
  }
}

inline void Compiler::Visit(const VariableStatement* var) {
  const Declarations& decls = var->decls();
  for (Declarations::const_iterator it = decls.begin(),
       last = decls.end(); it != last; ++it) {
    Visit(*it);
  }
}

inline void Compiler::Visit(const EmptyStatement* stmt) {
  // do nothing
}

inline void Compiler::Visit(const IfStatement* stmt) {
  const Condition::Type cond = Condition::Analyze(stmt->cond());

  switch (cond) {
    case Condition::COND_INDETERMINATE: {
      const JumpSite then_jump = EmitConditional(OP::IF_FALSE, stmt->cond());
      // then statement block
      EmitStatement(stmt->then_statement());

      // else statement check
      if (!stmt->else_statement()) {
        if (continuation_status_.IsDeadStatement()) {
          // recover if this IfStatement is not dead code
          continuation_status_.Next();
        }
        then_jump.JumpTo(this, CurrentSize());
        break;
      }

      if (continuation_status_.IsDeadStatement()) {
        continuation_status_.Next();
      } else {
        continuation_status_.Insert(stmt);
      }

      const JumpSite second_jump(CurrentSize());
      Emit<OP::JUMP_BY>(Instruction::Jump(0));
      then_jump.JumpTo(this, CurrentSize());

      EmitStatement(stmt->else_statement().Address());
      if (continuation_status_.Has(stmt)) {
        continuation_status_.Erase(stmt);
        if (continuation_status_.IsDeadStatement()) {
          continuation_status_.Next();
        }
      }

      second_jump.JumpTo(this, CurrentSize());
      break;
    }

    case Condition::COND_FALSE: {
      if (!stmt->else_statement()) {
        return;
      }
      EmitStatement(stmt->else_statement().Address());
      if (continuation_status_.IsDeadStatement()) {
        // recover if this IfStatement is not dead code
        continuation_status_.Next();
      }
      break;
    }

    case Condition::COND_TRUE: {
      // then statement block
      EmitStatement(stmt->then_statement());
      if (continuation_status_.IsDeadStatement()) {
        // recover if this IfStatement is not dead code
        continuation_status_.Next();
      }
      break;
    }
  }
}

class Compiler::ContinueTarget : protected BreakTarget {
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
    for (Jump::Targets::const_iterator it = continues_.begin(),
         last = continues_.end(); it != last; ++it) {
      compiler_->EmitJump(continue_target, *it);
    }
  }

 private:
  Jump::Targets continues_;
};

inline void Compiler::Visit(const DoWhileStatement* stmt) {
  ContinueTarget continue_target(this, stmt);
  const std::size_t start_index = CurrentSize();

  EmitStatement(stmt->body());

  const std::size_t cond_index = CurrentSize();

  const JumpSite jump = EmitConditional(OP::IF_TRUE, stmt->cond());
  jump.JumpTo(this, start_index);

  continue_target.EmitJumps(CurrentSize(), cond_index);

  continuation_status_.ResolveJump(stmt);
  if (continuation_status_.IsDeadStatement()) {
    continuation_status_.Next();
  }
}

inline void Compiler::Visit(const WhileStatement* stmt) {
  ContinueTarget continue_target(this, stmt);
  const Condition::Type cond = Condition::Analyze(stmt->cond());
  switch (cond) {
    case Condition::COND_INDETERMINATE: {
      if (current_variable_scope_->UseExpressionReturn() ||
          stmt->body()->IsEffectiveStatement()) {
        // We do loop-inversion optimization on this.
        const std::size_t start_index = CurrentSize();
        const JumpSite jump = EmitConditional(OP::IF_FALSE, stmt->cond());

        const std::size_t body_start = CurrentSize();
        EmitStatement(stmt->body());

        const JumpSite jump2 = EmitConditional(OP::IF_TRUE, stmt->cond());

        jump2.JumpTo(this, body_start);
        jump.JumpTo(this, CurrentSize());

        continue_target.EmitJumps(CurrentSize(), start_index);
        continuation_status_.ResolveJump(stmt);
        if (continuation_status_.IsDeadStatement()) {
          continuation_status_.Next();
        }
      } else {
        const std::size_t start_index = CurrentSize();
        const JumpSite jump = EmitConditional(OP::IF_TRUE, stmt->cond());
        jump.JumpTo(this, start_index);
      }
      break;
    }

    case Condition::COND_FALSE: {
      // like:
      //  while (false) {
      //  }
      break;
    }

    case Condition::COND_TRUE: {
      const std::size_t start_index = CurrentSize();
      if (current_variable_scope_->UseExpressionReturn() ||
          stmt->body()->IsEffectiveStatement()) {
        EmitStatement(stmt->body());
        Emit<OP::JUMP_BY>(Instruction::Jump(start_index - CurrentSize()));
        continue_target.EmitJumps(CurrentSize(), start_index);
        continuation_status_.ResolveJump(stmt);
        if (continuation_status_.IsDeadStatement()) {
          continuation_status_.Next();
        }
      } else {
        Emit<OP::JUMP_BY>(Instruction::Jump(start_index - CurrentSize()));
      }
      break;
    }
  }
}

inline void Compiler::Visit(const ForStatement* stmt) {
  ContinueTarget continue_target(this, stmt);

  if (const core::Maybe<const Statement> maybe = stmt->init()) {
    const Statement* init = maybe.Address();
    if (init->AsVariableStatement()) {
      EmitStatement(init);
    } else {
      assert(init->AsExpressionStatement());
      // not evaluate as ExpressionStatement
      // because ExpressionStatement returns statement value
      EmitExpressionIgnoreResult(init->AsExpressionStatement()->expr());
    }
  }

  const std::size_t start_index = CurrentSize();
  const core::Maybe<const Expression> cond = stmt->cond();
  JumpSite jump;

  if (cond) {
    jump = EmitConditional(OP::IF_FALSE, cond.Address());
  }

  const std::size_t body_start = CurrentSize();
  EmitStatement(stmt->body());
  const std::size_t body_end = CurrentSize();

  if (const core::Maybe<const Expression> next = stmt->next()) {
    EmitExpressionIgnoreResult(next.Address());
  }

  if (cond) {
    // We do loop-inversion optimization on this.
    const JumpSite jump2 = EmitConditional(OP::IF_TRUE, cond.Address());
    jump.JumpTo(this, CurrentSize());
    jump2.JumpTo(this, body_start);
  } else {
    Emit<OP::JUMP_BY>(Instruction::Jump(start_index - CurrentSize()));
  }

  continue_target.EmitJumps(CurrentSize(), body_end);

  continuation_status_.ResolveJump(stmt);
  if (continuation_status_.IsDeadStatement()) {
    continuation_status_.Next();
  }
}

inline void Compiler::Visit(const ForInStatement* stmt) {
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
    RegisterID iterator = Temporary();
    std::size_t for_in_setup_jump;
    {
      RegisterID enumerable = EmitExpression(stmt->enumerable());
      for_in_setup_jump = CurrentSize();
      Emit<OP::FORIN_SETUP>(Instruction::Jump(0, iterator, enumerable));
      PushLevelIterator(iterator);
    }

    const std::size_t start_index = CurrentSize();
    {
      if (!lhs || lhs->AsIdentifier()) {
        // Identifier
        if (lhs) {
          for_decl = lhs->AsIdentifier()->symbol();
        }
        if (RegisterID local = GetLocal(for_decl)) {
          const LookupInfo info = Lookup(for_decl);
          if (info.immutable()) {
            local = Temporary();
          }
          thunkpool_.Spill(local);
          Emit<OP::FORIN_ENUMERATE>(Instruction::Jump(0, local, iterator));
          if (code_->strict() && info.immutable()) {
            Emit<OP::RAISE_IMMUTABLE>(SymbolToNameIndex(for_decl));
          }
        } else {
          RegisterID tmp = Temporary();
          Emit<OP::FORIN_ENUMERATE>(Instruction::Jump(0, tmp, iterator));
          EmitStore(for_decl, tmp);
        }
      } else {
        RegisterID tmp = Temporary();
        Emit<OP::FORIN_ENUMERATE>(Instruction::Jump(0, tmp, iterator));
        if (lhs->AsPropertyAccess()) {
          // PropertyAccess
          if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
            // IdentifierAccess
            RegisterID base = EmitExpression(ac->target());
            const uint32_t index = SymbolToNameIndex(ac->key());
            Emit<OP::STORE_PROP>(Instruction::SSW(base, tmp, index), 0, 0);
          } else {
            // IndexAccess
            const IndexAccess* idx = lhs->AsIndexAccess();
            const Expression* key = idx->key();
            const Symbol sym = PropertyName(key);
            if (sym != symbol::kDummySymbol) {
              RegisterID base = EmitExpression(idx->target());
              const uint32_t index = SymbolToNameIndex(sym);
              Emit<OP::STORE_PROP>(Instruction::SSW(base, tmp, index), 0, 0);
            } else {
              Thunk base(&thunkpool_, EmitExpression(idx->target()));
              RegisterID element = EmitExpression(idx->key());
              Emit<OP::STORE_ELEMENT>(
                  Instruction::Reg3(base.Release(), element, tmp));
            }
          }
        } else {
          // FunctionCall
          // ConstructorCall
          tmp.reset();
          EmitExpressionIgnoreResult(lhs);
          Emit<OP::RAISE_REFERENCE>();
        }
      }
    }

    EmitStatement(stmt->body());
    if (!continuation_status_.IsDeadStatement()) {
      Emit<OP::JUMP_BY>(
          Instruction::Jump(start_index - CurrentSize()));  // loop jump
    }
    PopLevel();

    EmitJump(CurrentSize(), start_index);  // enumeration end jump
    Emit<OP::FORIN_LEAVE>(iterator);
    code_->RegisterHandler(
        Handler(
            Handler::ITERATOR,
            start_index,
            CurrentSize(),
            0,
            iterator->register_offset(),
            0));
    const std::size_t end_index = CurrentSize();
    jump.EmitJumps(end_index, start_index);
    EmitJump(end_index, for_in_setup_jump);
  }
  continuation_status_.ResolveJump(stmt);
  if (continuation_status_.IsDeadStatement()) {
    continuation_status_.Next();
  }
}

inline RegisterID Compiler::EmitUnrollingLevel(uint16_t from,
                                               uint16_t to,
                                               bool cont,
                                               RegisterID dst) {
  for (; from > to; --from) {
    const Level& entry = level_stack_[from - 1];
    if (entry.type() == Level::FINALLY) {
      if (dst) {
        dst = EmitMV(entry.ret(), dst);
      }
      const std::size_t finally_jump = CurrentSize();
      Emit<OP::JUMP_SUBROUTINE>(
          Instruction::Jump(0, entry.jmp(), entry.flag()));
      entry.holes()->push_back(finally_jump);
    } else if (entry.type() == Level::ENV) {
      Emit<OP::POP_ENV>();
    } else if (entry.type() == Level::ITERATOR) {
      // jump is continue and target for-in
      // for (var i in []) {
      //   continue;
      // }
      if (!(cont && from == (to + 1))) {
        Emit<OP::FORIN_LEAVE>(entry.ret());
      }
    }
  }
  return dst;
}

inline void Compiler::Visit(const ContinueStatement* stmt) {
  const Jump::Table::const_iterator it = jump_table_.find(stmt->target());
  assert(it != jump_table_.end());
  const Jump& entry = it->second;
  EmitUnrollingLevel(CurrentLevel(), entry.level(), true);
  const std::size_t arg_index = CurrentSize();
  Emit<OP::JUMP_BY>(Instruction::Jump(0));
  entry.continues()->push_back(arg_index);
  continuation_status_.JumpTo(stmt->target());
}

inline void Compiler::Visit(const BreakStatement* stmt) {
  if (!stmt->target() && !stmt->label().IsDummy()) {
    // through
  } else {
    const Jump::Table::const_iterator it = jump_table_.find(stmt->target());
    assert(it != jump_table_.end());
    const Jump& entry = it->second;
    EmitUnrollingLevel(CurrentLevel(), entry.level(), false);
    const std::size_t arg_index = CurrentSize();
    Emit<OP::JUMP_BY>(Instruction::Jump(0));  // dummy
    entry.breaks()->push_back(arg_index);
  }
  continuation_status_.JumpTo(stmt->target());
}

inline void Compiler::Visit(const ReturnStatement* stmt) {
  RegisterID dst;
  if (const core::Maybe<const Expression> expr = stmt->expr()) {
    dst = EmitExpression(expr.Address());
  } else {
    dst = Temporary();
    Emit<OP::LOAD_UNDEFINED>(dst);
  }

  if (CurrentLevel() == 0) {
    Emit<OP::RETURN>(dst);
  } else {
    // nested finally has found
    // set finally jump targets
    dst = EmitUnrollingLevel(CurrentLevel(), 0, false, dst);
    Emit<OP::RETURN>(dst);
  }
  continuation_status_.Kill();
}

inline void Compiler::Visit(const WithStatement* stmt) {
  {
    RegisterID dst = EmitExpression(stmt->context());
    Emit<OP::WITH_SETUP>(dst);
  }
  {
    PushLevelEnv();
    current_variable_scope_ =
        std::shared_ptr<VariableScope>(
            new WithScope(current_variable_scope_));
    const std::size_t with_start = CurrentSize();
    EmitStatement(stmt->body());
    current_variable_scope_ = current_variable_scope_->upper();
    code_->RegisterHandler(
        Handler(
            Handler::ENV,
            with_start,
            CurrentSize(),
            0,
            0,
            0));
    PopLevel();
  }
  Emit<OP::POP_ENV>();
}

inline void Compiler::Visit(const LabelledStatement* stmt) {
  EmitStatement(stmt->body());
}

inline void Compiler::Visit(const SwitchStatement* stmt) {
  BreakTarget jump(this, stmt);
  typedef SwitchStatement::CaseClauses CaseClauses;
  const CaseClauses& clauses = stmt->clauses();
  bool has_default_clause = false;
  std::size_t label = 0;
  std::vector<std::size_t> indexes(clauses.size());
  {
    RegisterID cond = EmitExpressionToDest(stmt->expr(), Temporary());
    std::vector<std::size_t>::iterator idx = indexes.begin();
    std::vector<std::size_t>::iterator default_it = indexes.end();
    for (CaseClauses::const_iterator it = clauses.begin(),
         last = clauses.end(); it != last; ++it, ++idx) {
      if (const core::Maybe<const Expression> expr = (*it)->expr()) {
        // case
        RegisterID tmp = EmitExpression(expr.Address());
        RegisterID ret = Temporary(tmp);
        Emit<OP::BINARY_STRICT_EQ>(Instruction::Reg3(ret, cond, tmp));
        *idx = CurrentSize();
        Emit<OP::IF_TRUE>(Instruction::Jump(0, ret));
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
    Emit<OP::JUMP_BY>(Instruction::Jump(0));
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
          continuation_status_.Next();
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
    continuation_status_.Next();
  }
}

inline void Compiler::Visit(const ThrowStatement* stmt) {
  RegisterID dst = EmitExpression(stmt->expr());
  Emit<OP::THROW>(dst);
  continuation_status_.Kill();
}

class Compiler::TryTarget : private core::Noncopyable<> {
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
    const Level::Stack& stack = compiler_->level_stack();
    const Level& entry = stack.back();
    assert(entry.type() == Level::FINALLY);
    assert(entry.holes() == &vec_);
    const Jump::Targets* vec = entry.holes();
    for (Jump::Targets::const_iterator it = vec->begin(),
         last = vec->end(); it != last; ++it) {
      compiler_->EmitJump(finally_target, *it);
    }
    compiler_->PopLevel();
  }

 private:
  Compiler* compiler_;
  bool has_finally_;
  Jump::Targets vec_;
};

inline void Compiler::Visit(const TryStatement* stmt) {
  const std::size_t try_start = CurrentSize();
  const bool has_catch = stmt->catch_block();
  const bool has_finally = stmt->finally_block();
  TryTarget target(this, has_finally);
  RegisterID jmp;
  RegisterID ret;
  RegisterID flag;
  if (has_finally) {
    jmp = Temporary();
    ret = Temporary();
    flag = Temporary();
    Level& level = level_stack_.back();
    level.set_jmp(jmp);
    level.set_ret(ret);
    level.set_flag(flag);
  }
  EmitStatement(stmt->body());
  if (has_finally && !continuation_status_.IsDeadStatement()) {
    const std::size_t finally_jump = CurrentSize();
    Emit<OP::JUMP_SUBROUTINE>(Instruction::Jump(0, jmp, flag));
    level_stack_.back().holes()->push_back(finally_jump);
  }
  const std::size_t label = CurrentSize();
  Emit<OP::JUMP_BY>(Instruction::Jump(0));

  std::size_t catch_return_label_index = 0;
  if (const core::Maybe<const Block> block = stmt->catch_block()) {
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Next();
    } else {
      continuation_status_.Insert(stmt);
    }
    const Symbol catch_symbol = stmt->catch_name().Address()->symbol();
    {
      RegisterID error = Temporary();
      code_->RegisterHandler(
          Handler(
              Handler::CATCH,
              try_start,
              CurrentSize(),
              0,
              error->register_offset(),
              0));
      Emit<OP::TRY_CATCH_SETUP>(
          Instruction::SW(error, SymbolToNameIndex(catch_symbol)));
    }

    {
      PushLevelEnv();
      current_variable_scope_ =
          std::shared_ptr<VariableScope>(
              new CatchScope(current_variable_scope_, catch_symbol));
      std::size_t catch_start = CurrentSize();
      EmitStatement(block.Address());
      current_variable_scope_ = current_variable_scope_->upper();
      PopLevel();
      code_->RegisterHandler(
          Handler(
              Handler::ENV,
              catch_start,
              CurrentSize(),
              0,
              0,
              0));
      Emit<OP::POP_ENV>();
    }

    if (has_finally) {
      const std::size_t finally_jump = CurrentSize();
      Emit<OP::JUMP_SUBROUTINE>(Instruction::Jump(0, jmp, flag));
      level_stack_.back().holes()->push_back(finally_jump);
    }
    catch_return_label_index = CurrentSize();
    Emit<OP::JUMP_BY>(Instruction::Jump(0));

    if (continuation_status_.Has(stmt)) {
      continuation_status_.Erase(stmt);
      if (continuation_status_.IsDeadStatement()) {
        continuation_status_.Next();
      }
    }
  }

  if (const core::Maybe<const Block> block = stmt->finally_block()) {
    const std::size_t finally_start = CurrentSize();
    if (continuation_status_.IsDeadStatement()) {
      continuation_status_.Next();
    } else {
      continuation_status_.Insert(stmt);
    }
    code_->RegisterHandler(
        Handler(
            Handler::FINALLY,
            try_start,
            finally_start,
            jmp->register_offset(),
            ret->register_offset(),
            flag->register_offset()));
    target.EmitJumps(finally_start);

    EmitStatement(block.Address());
    Emit<OP::RETURN_SUBROUTINE>(Instruction::Reg2(jmp, flag));

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

inline void Compiler::Visit(const DebuggerStatement* stmt) {
  Emit<OP::DEBUGGER>();
}

inline void Compiler::Visit(const ExpressionStatement* stmt) {
  if (current_variable_scope_->UseExpressionReturn()) {
    eval_result_ = EmitExpressionToDest(stmt->expr(), eval_result_);
  } else {
    EmitExpressionIgnoreResult(stmt->expr());
  }
}

inline void Compiler::Visit(const CaseClause* dummy) { }

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_COMPILER_STATEMENT_H_
