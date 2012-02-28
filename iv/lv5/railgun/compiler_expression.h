#ifndef IV_LV5_RAILGUN_COMPILER_EXPRESSION_H_
#define IV_LV5_RAILGUN_COMPILER_EXPRESSION_H_
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Compiler::DestGuard {
 public:
  explicit DestGuard(Compiler* compiler)
    : compiler_(compiler) {
  }
  ~DestGuard() {
    assert(compiler_->ignore_result() || compiler_->dst());
  }
 private:
  Compiler* compiler_;
};

inline RegisterID Compiler::EmitOptimizedLookup(OP::Type op,
                                                uint32_t index,
                                                RegisterID dst) {
  dst = Dest(dst);
  const LookupInfo info = Lookup(code_->names_[index]);
  assert(info.type() != LookupInfo::STACK);
  switch (info.type()) {
    case LookupInfo::HEAP: {
      thunkpool_.Spill(dst);
      EmitUnsafe(
          OP::ToHeap(op),
          Instruction::SW(dst, index),
          Instruction::UInt32(
              info.heap_location(),
              current_variable_scope_->scope_nest_count() - info.scope()));
      return dst;
    }
    case LookupInfo::GLOBAL: {
      // last 2 zeros are placeholders for PIC
      thunkpool_.Spill(dst);
      EmitUnsafe(OP::ToGlobal(op), Instruction::SW(dst, index), 0u, 0u);
      return dst;
    }
    case LookupInfo::LOOKUP: {
      thunkpool_.Spill(dst);
      EmitUnsafe(op, Instruction::SW(dst, index));
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

inline void Compiler::EmitIdentifierAccessAssign(const Assignment* assign,
                                                 const Expression* target,
                                                 Symbol sym) {
    Thunk base(&thunkpool_, EmitExpression(target));
    const uint32_t index = SymbolToNameIndex(sym);
    if (Compiler::NotOrdered(dst_)) {
      dst_ = EmitExpressionToDest(assign->right(), dst_);
      Emit<OP::STORE_PROP>(Instruction::SSW(base.Release(), dst_, index), 0, 0);
    } else {
      RegisterID ret = EmitExpression(assign->right());
      Emit<OP::STORE_PROP>(Instruction::SSW(base.Release(), ret, index), 0, 0);
      dst_ = EmitMV(dst_, ret);
    }
}

inline void Compiler::EmitIdentifierAccessBinaryAssign(const Assignment* assign,
                                                       const Expression* target,
                                                       Symbol sym) {
    Thunk base(&thunkpool_, EmitExpression(target));
    const uint32_t index = SymbolToNameIndex(sym);
    RegisterID prop = Temporary();
    Emit<OP::LOAD_PROP>(Instruction::SSW(prop, base.reg(), index), 0, 0, 0);
    RegisterID val = EmitExpression(assign->right());
    if (Compiler::NotOrdered(dst_)) {
      dst_ = Dest(dst_, val, prop);
      thunkpool_.Spill(dst_);
      EmitUnsafe(OP::BinaryOP(assign->op()),
                 Instruction::Reg3(dst_, prop, val));
      Emit<OP::STORE_PROP>(Instruction::SSW(base.Release(), dst_, index), 0, 0);
    } else {
      RegisterID ret = Temporary(val, prop);
      EmitUnsafe(OP::BinaryOP(assign->op()), Instruction::Reg3(ret, prop, val));
      Emit<OP::STORE_PROP>(Instruction::SSW(base.Release(), ret, index), 0, 0);
      dst_ = EmitMV(dst_, ret);
    }
}

inline void Compiler::Visit(const Assignment* assign) {
  const DestGuard dest_guard(this);
  const Token::Type token = assign->op();

  const Expression* lhs = assign->left();
  const Expression* rhs = assign->right();
  
  if (!lhs->IsValidLeftHandSide()) {
    // not valid LHS
    if (token == Token::TK_ASSIGN) {
      EmitExpressionIgnoreResult(lhs);
      EmitExpressionIgnoreResult(rhs);
      dst_ = Dest(dst_);
      Emit<OP::RAISE_REFERENCE>();
    } else {
      {
        Thunk src(&thunkpool_, EmitExpression(lhs));
        RegisterID val = EmitExpression(rhs);
        RegisterID ret = Temporary(val, src.Release());
        EmitUnsafe(OP::BinaryOP(token),
                   Instruction::Reg3(ret, src.Release(), val));
      }
      dst_ = Dest(dst_);
      Emit<OP::RAISE_REFERENCE>();
    }
    return;
  }

  assert(lhs->IsValidLeftHandSide());
  if (token == Token::TK_ASSIGN) {
    if (const Identifier* ident = lhs->AsIdentifier()) {
      // Identifier
      if (RegisterID local = GetLocal(ident->symbol())) {
        const LookupInfo info = Lookup(ident->symbol());
        if (info.immutable()) {
          if (code_->strict()) {
            EmitExpressionIgnoreResult(rhs);
            dst_ = Dest(dst_);
            Emit<OP::RAISE_IMMUTABLE>(SymbolToNameIndex(ident->symbol()));
          } else {
            dst_ = EmitExpressionToDest(rhs, Dest(dst_));
          }
        } else {
          dst_ = EmitMV(dst_, EmitExpressionToDest(rhs, local));
        }
      } else {
        if (Compiler::NotOrdered(dst_)) {
          dst_ = EmitExpressionToDest(rhs, dst_);
          EmitStore(ident->symbol(), dst_);
        } else {
          RegisterID r = EmitExpression(rhs);
          EmitStore(ident->symbol(), r);
          dst_ = EmitMV(dst_, r);
        }
      }
    } else {
      assert(lhs->AsPropertyAccess());
      // PropertyAccess
      if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
        // IdentifierAccess
        EmitIdentifierAccessAssign(assign, ac->target(), ac->key());
      } else {
        // IndexAccess
        const IndexAccess* idx = lhs->AsIndexAccess();
        const Symbol sym = PropertyName(idx->key());
        if (sym != symbol::kDummySymbol) {
          EmitIdentifierAccessAssign(assign, idx->target(), sym);
        } else {
          Thunk base(&thunkpool_, EmitExpression(idx->target()));
          Thunk element(&thunkpool_, EmitExpression(idx->key()));
          if (Compiler::NotOrdered(dst_)) {
            dst_ = EmitExpressionToDest(rhs, dst_);
            Emit<OP::STORE_ELEMENT>(
                Instruction::Reg3(base.Release(), element.Release(), dst_));
          } else {
            RegisterID ret = EmitExpression(rhs);
            Emit<OP::STORE_ELEMENT>(
                Instruction::Reg3(base.Release(), element.Release(), ret));
            dst_ = EmitMV(dst_, ret);
          }
        }
      }
    }
  } else {
    if (const Identifier* ident = lhs->AsIdentifier()) {
      // Identifier
      const LookupInfo info = Lookup(ident->symbol());
      if (RegisterID local = GetLocal(ident->symbol())) {
        Thunk lv(&thunkpool_, EmitExpression(lhs));
        RegisterID rv = EmitExpression(rhs);
        lv.Release();
        if (info.immutable()) {
          dst_ = Dest(dst_);
          if (code_->strict()) {
            EmitUnsafe(OP::BinaryOP(token),
                       Instruction::Reg3(Temporary(), lv.reg(), rv));
            Emit<OP::RAISE_IMMUTABLE>(SymbolToNameIndex(ident->symbol()));
          } else {
            thunkpool_.Spill(dst_);
            EmitUnsafe(OP::BinaryOP(token),
                       Instruction::Reg3(dst_, lv.reg(), rv));
          }
        } else {
          thunkpool_.Spill(local);
          EmitUnsafe(OP::BinaryOP(token),
                     Instruction::Reg3(local, lv.reg(), rv));
          dst_ = EmitMV(dst_, local);
        }
      } else {
        Thunk lv(&thunkpool_, EmitExpression(lhs));
        RegisterID rv = EmitExpression(rhs);
        lv.Release();
        if (Compiler::NotOrdered(dst_)) {
          dst_ = Dest(dst_, lv.reg(), rv);
          thunkpool_.Spill(dst_);
          EmitUnsafe(OP::BinaryOP(token),
                     Instruction::Reg3(dst_, lv.reg(), rv));
          EmitStore(ident->symbol(), dst_);
        } else {
          RegisterID ret = Temporary(lv.reg(), rv);
          EmitUnsafe(OP::BinaryOP(token), Instruction::Reg3(ret, lv.reg(), rv));
          EmitStore(ident->symbol(), ret);
          dst_ = EmitMV(dst_, ret);
        }
      }
    } else {
      assert(lhs->AsPropertyAccess());
      // PropertyAccess
      if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
        // IdentifierAccess
        EmitIdentifierAccessBinaryAssign(assign, ac->target(), ac->key());
      } else {
        // IndexAccess
        const IndexAccess* idx = lhs->AsIndexAccess();
        const Expression* key = idx->key();
        const Symbol sym = PropertyName(key);
        if (sym != symbol::kDummySymbol) {
          EmitIdentifierAccessBinaryAssign(assign, idx->target(), sym);
        } else {
          Thunk base(&thunkpool_, EmitExpression(idx->target()));
          Thunk element(&thunkpool_, EmitExpression(key));
          RegisterID prop = Temporary();
          Emit<OP::LOAD_ELEMENT>(
              Instruction::Reg3(prop, base.reg(), element.reg()));
          RegisterID val = EmitExpression(rhs);
          if (Compiler::NotOrdered(dst_)) {
            dst_ = Dest(dst_, val, prop);
            thunkpool_.Spill(dst_);
            EmitUnsafe(OP::BinaryOP(token), Instruction::Reg3(dst_, prop, val));
            Emit<OP::STORE_ELEMENT>(
                Instruction::Reg3(base.Release(), element.Release(), dst_));
          } else {
            RegisterID ret = Temporary(val, prop);
            EmitUnsafe(OP::BinaryOP(token), Instruction::Reg3(ret, prop, val));
            Emit<OP::STORE_ELEMENT>(
                Instruction::Reg3(base.Release(), element.Release(), ret));
            dst_ = EmitMV(dst_, ret);
          }
        }
      }
    }
  }
}

template<core::Token::Type token>
void Compiler::EmitLogicalPath(const BinaryOperation* binary) {
  IV_STATIC_ASSERT(token == Token::TK_LOGICAL_AND ||
                   token == Token::TK_LOGICAL_OR);
  const OP::Type kOp = token == Token::TK_LOGICAL_AND ? OP::IF_FALSE : OP::IF_TRUE;
  if (ignore_result()) {
    // like
    // opt || (opt = { });
    std::size_t label;
    {
      RegisterID cond = EmitExpression(binary->left());
      label = CurrentSize();
      Emit<kOp>(Instruction::Jump(0, cond));
    }
    EmitExpressionIgnoreResult(binary->right());
    EmitJump(CurrentSize(), label);
  } else {
    std::size_t label;
    dst_ = Dest(dst_);
    {
      dst_ = EmitExpressionToDest(binary->left(), dst_);
      label = CurrentSize();
      Emit<kOp>(Instruction::Jump(0, dst_));
    }
    dst_ = EmitExpressionToDest(binary->right(), dst_);
    EmitJump(CurrentSize(), label);
  }
}

inline void Compiler::Visit(const BinaryOperation* binary) {
  const DestGuard dest_guard(this);
  const Token::Type token = binary->op();
  switch (token) {
    case Token::TK_LOGICAL_AND: {  // &&
      EmitLogicalPath<Token::TK_LOGICAL_AND>(binary);
      break;
    }

    case Token::TK_LOGICAL_OR: {  // ||
      EmitLogicalPath<Token::TK_LOGICAL_OR>(binary);
      break;
    }

    case Token::TK_COMMA: {  // ,
      EmitExpressionIgnoreResult(binary->left());
      dst_ = EmitExpressionToDest(binary->right(), dst_);
      break;
    }

    default: {
      Thunk lv(&thunkpool_, EmitExpression(binary->left()));
      RegisterID rv = EmitExpression(binary->right());
      dst_ = Dest(dst_, lv.Release(), rv);
      thunkpool_.Spill(dst_);
      EmitUnsafe(OP::BinaryOP(token), Instruction::Reg3(dst_, lv.reg(), rv));
      break;
    }
  }
}

inline void Compiler::Visit(const ConditionalExpression* cond) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // like
    // (cond) ? then_exec() : else_exec();
    std::size_t first;
    {
      RegisterID ret = EmitExpression(cond->cond());
      first = CurrentSize();
      Emit<OP::IF_FALSE>(Instruction::Jump(0, ret));
    }
    EmitExpressionIgnoreResult(cond->left());
    const std::size_t second = CurrentSize();
    Emit<OP::JUMP_BY>(Instruction::Jump(0));
    EmitJump(CurrentSize(), first);
    EmitExpressionIgnoreResult(cond->right());
    EmitJump(CurrentSize(), second);
  } else {
    std::size_t first;
    {
      RegisterID ret = EmitExpression(cond->cond());
      first = CurrentSize();
      Emit<OP::IF_FALSE>(Instruction::Jump(0, ret));
    }
    dst_ = Dest(dst_);
    thunkpool_.Spill(dst_);
    dst_ = EmitExpressionToDest(cond->left(), dst_);
    const std::size_t second = CurrentSize();
    Emit<OP::JUMP_BY>(Instruction::Jump(0));
    EmitJump(CurrentSize(), first);
    dst_ = EmitExpressionToDest(cond->right(), dst_);
    EmitJump(CurrentSize(), second);
  }
}

template<OP::Type PropOP, OP::Type ElementOP>
inline RegisterID Compiler::EmitElement(const IndexAccess* prop,
                                        RegisterID dst) {
  Thunk base(&thunkpool_, EmitExpression(prop->target()));
  const Expression* key = prop->key();
  const Symbol sym = PropertyName(key);
  if (sym != symbol::kDummySymbol) {
    const uint32_t index = SymbolToNameIndex(sym);
    dst = Dest(dst, base.Release());
    thunkpool_.Spill(dst);
    Emit<PropOP>(Instruction::SSW(dst, base.reg(), index), 0, 0, 0);
  } else {
    RegisterID element = EmitExpression(key);
    dst = Dest(dst, base.Release(), element);
    thunkpool_.Spill(dst);
    Emit<ElementOP>(Instruction::Reg3(dst, base.reg(), element));
  }
  return dst;
}

inline void Compiler::Visit(const UnaryOperation* unary) {
  const DestGuard dest_guard(this);
  const Token::Type token = unary->op();
  const Expression* expr = unary->expr();
  switch (token) {
    case Token::TK_DELETE: {
      if (const Identifier* ident = expr->AsIdentifier()) {
        // DELETE_NAME_STRICT is already rejected in parser
        assert(!code_->strict());
        if (RegisterID local = GetLocal(ident->symbol())) {
          dst_ = Dest(dst_);
          thunkpool_.Spill(dst_);
          Emit<OP::LOAD_FALSE>(dst_);
        } else {
          dst_ = EmitOptimizedLookup(
              OP::DELETE_NAME,
              SymbolToNameIndex(ident->symbol()), dst_);
        }
      } else if (expr->AsPropertyAccess()) {
        if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
          // IdentifierAccess
          RegisterID base = EmitExpression(ac->target());
          const uint32_t index = SymbolToNameIndex(ac->key());
          dst_ = Dest(dst_);
          thunkpool_.Spill(dst_);
          Emit<OP::DELETE_PROP>(Instruction::SSW(dst_, base, index), 0, 0, 0);
        } else {
          // IndexAccess
          dst_ = EmitElement<
               OP::DELETE_PROP,
               OP::DELETE_ELEMENT>(expr->AsIndexAccess(), dst_);
        }
      } else {
        // other case is no effect
        // but accept expr
        EmitExpressionIgnoreResult(expr);
        dst_ = Dest(dst_);
        thunkpool_.Spill(dst_);
        Emit<OP::LOAD_TRUE>(dst_);
      }
      return;
    }

    case Token::TK_VOID: {
      EmitExpressionIgnoreResult(expr);
      dst_ = Dest(dst_);
      thunkpool_.Spill(dst_);
      Emit<OP::LOAD_UNDEFINED>(dst_);
      return;
    }

    case Token::TK_TYPEOF: {
      if (const Identifier* ident = expr->AsIdentifier()) {
        // maybe Global Reference
        if (RegisterID local = GetLocal(ident->symbol())) {
          dst_ = Dest(dst_);
          thunkpool_.Spill(dst_);
          Emit<OP::TYPEOF>(Instruction::Reg2(dst_, local));
        } else {
          const uint32_t index = SymbolToNameIndex(ident->symbol());
          dst_ = EmitOptimizedLookup(OP::TYPEOF_NAME, index, dst_);
        }
      } else {
        RegisterID src = EmitExpression(expr);
        dst_ = Dest(dst_, src);
        thunkpool_.Spill(dst_);
        Emit<OP::TYPEOF>(Instruction::Reg2(dst_, src));
      }
      return;
    }

    case Token::TK_INC:
    case Token::TK_DEC: {
      if (!expr->IsValidLeftHandSide()) {
        RegisterID tmp = EmitExpression(expr);
        dst_ = Dest(dst_, tmp);
        Emit<OP::TO_NUMBER>(tmp);
        Emit<OP::RAISE_REFERENCE>();
        return;
      }
      assert(expr->IsValidLeftHandSide());
      if (const Identifier* ident = expr->AsIdentifier()) {
        const uint32_t index = SymbolToNameIndex(ident->symbol());
        if (RegisterID local = GetLocal(ident->symbol())) {
          const LookupInfo info = Lookup(ident->symbol());
          if (ignore_result()) {
            if (info.immutable()) {
              Emit<OP::TO_NUMBER>(local);
              if (code_->strict()) {
                Emit<OP::RAISE_IMMUTABLE>(index);
              }
            } else {
              thunkpool_.Spill(local);
              EmitUnsafe((token == Token::TK_INC) ?
                         OP::INCREMENT : OP::DECREMENT, local);
            }
          } else {
            if (info.immutable()) {
              if (code_->strict()) {
                dst_ = Dest(dst_);
                Emit<OP::TO_NUMBER>(local);
                Emit<OP::RAISE_IMMUTABLE>(index);
              } else {
                RegisterID tmp = Temporary(dst_);
                EmitMV(tmp, local);
                EmitUnsafe((token == Token::TK_INC) ?
                           OP::INCREMENT : OP::DECREMENT, tmp);
                dst_ = EmitMV(dst_, tmp);
              }
            } else {
              thunkpool_.Spill(local);
              EmitUnsafe((token == Token::TK_INC) ?
                         OP::INCREMENT : OP::DECREMENT, local);
              dst_ = EmitMV(dst_, local);
            }
          }
        } else {
          dst_ = EmitOptimizedLookup(
              (token == Token::TK_INC) ?
              OP::INCREMENT_NAME : OP::DECREMENT_NAME, index, dst_);
        }
      } else {
        assert(expr->AsPropertyAccess());
        if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
          // IdentifierAccess
          RegisterID base = EmitExpression(ac->target());
          const uint32_t index = SymbolToNameIndex(ac->key());
          dst_ = Dest(dst_);
          thunkpool_.Spill(dst_);
          if (token == Token::TK_INC) {
            Emit<OP::INCREMENT_PROP>(
                Instruction::SSW(dst_, base, index), 0, 0, 0);
          } else {
            Emit<OP::DECREMENT_PROP>(
                Instruction::SSW(dst_, base, index), 0, 0, 0);
          }
        } else {
          // IndexAccess
          const IndexAccess* idxac = expr->AsIndexAccess();
          if (token == Token::TK_INC) {
            dst_ = EmitElement<
                 OP::INCREMENT_PROP,
                 OP::INCREMENT_ELEMENT>(idxac, dst_);
          } else {
            dst_ = EmitElement<
                OP::DECREMENT_PROP,
                OP::DECREMENT_ELEMENT>(idxac, dst_);
          }
        }
      }
      return;
    }

    default: {
      RegisterID src = EmitExpression(expr);
      dst_ = Dest(dst_, src);
      thunkpool_.Spill(dst_);
      EmitUnsafe(OP::UnaryOP(token), Instruction::Reg2(dst_, src));
      return;
    }
  }
}

inline void Compiler::Visit(const PostfixExpression* postfix) {
  const DestGuard dest_guard(this);
  const Expression* expr = postfix->expr();
  const Token::Type token = postfix->op();
  if (!expr->IsValidLeftHandSide()) {
    RegisterID tmp = EmitExpression(expr);
    dst_ = Dest(dst_, tmp);
    Emit<OP::TO_NUMBER>(tmp);
    Emit<OP::RAISE_REFERENCE>();
    return;
  }
  assert(expr->IsValidLeftHandSide());
  if (const Identifier* ident = expr->AsIdentifier()) {
    const uint32_t index = SymbolToNameIndex(ident->symbol());
    if (RegisterID local = GetLocal(ident->symbol())) {
      const LookupInfo info = Lookup(ident->symbol());
      if (ignore_result()) {
        // ignore result path. not return dst
        if (info.immutable()) {
          Emit<OP::TO_NUMBER>(local);
          if (code_->strict()) {
            Emit<OP::RAISE_IMMUTABLE>(index);
          }
        } else {
          thunkpool_.Spill(local);
          EmitUnsafe((token == Token::TK_INC) ?
                     OP::INCREMENT : OP::DECREMENT, local);
        }
      } else {
        if (info.immutable()) {
          if (code_->strict()) {
            dst_ = Dest(dst_);
            Emit<OP::TO_NUMBER>(local);
            Emit<OP::RAISE_IMMUTABLE>(index);
          } else {
            RegisterID tmp = Temporary(dst_);
            EmitMV(tmp, local);
            Emit<OP::TO_NUMBER>(tmp);
            dst_ = EmitMV(dst_, tmp);
          }
        } else {
          dst_ = Dest(dst_);
          thunkpool_.Spill(dst_);
          thunkpool_.Spill(local);
          EmitUnsafe((token == Token::TK_INC) ?
                     OP::POSTFIX_INCREMENT : OP::POSTFIX_DECREMENT,
                     Instruction::Reg2(dst_, local));
        }
      }
    } else {
      dst_ = EmitOptimizedLookup(
          (token == Token::TK_INC) ?
          OP::POSTFIX_INCREMENT_NAME : OP::POSTFIX_DECREMENT_NAME, index, dst_);
    }
  } else {
    assert(expr->AsPropertyAccess());
    if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
      // IdentifierAccess
      RegisterID base = EmitExpression(ac->target());
      const uint32_t index = SymbolToNameIndex(ac->key());
      dst_ = Dest(dst_);
      thunkpool_.Spill(dst_);
      if (token == Token::TK_INC) {
        Emit<OP::POSTFIX_INCREMENT_PROP>(
            Instruction::SSW(dst_, base, index), 0, 0, 0);
      } else {
        Emit<OP::POSTFIX_DECREMENT_PROP>(
            Instruction::SSW(dst_, base, index), 0, 0, 0);
      }
    } else {
      // IndexAccess
      const IndexAccess* idxac = expr->AsIndexAccess();
      if (token == Token::TK_INC) {
        dst_ = EmitElement<
            OP::POSTFIX_INCREMENT_PROP,
            OP::POSTFIX_INCREMENT_ELEMENT>(idxac, dst_);
      } else {
        dst_ = EmitElement<
            OP::POSTFIX_DECREMENT_PROP,
            OP::POSTFIX_DECREMENT_ELEMENT>(idxac, dst_);
      }
    }
  }
}

inline void Compiler::Visit(const StringLiteral* lit) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // this value is not used and StringLiteral doesn't have side effect
    return;
  }
  const core::UString s = core::ToUString(lit->value());
  const JSStringToIndexMap::const_iterator it = jsstring_to_index_map_.find(s);
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  if (it != jsstring_to_index_map_.end()) {
    // duplicate constant
    Emit<OP::LOAD_CONST>(Instruction::SW(dst_, it->second));
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
  jsstring_to_index_map_.insert(std::make_pair(s, index));
  Emit<OP::LOAD_CONST>(Instruction::SW(dst_, index));
}

inline void Compiler::Visit(const NumberLiteral* lit) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // this value is not used and NumberLiteral doesn't have side effect
    return;
  }
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  const double val = lit->value();
  const JSDoubleToIndexMap::const_iterator it =
      double_to_index_map_.find(val);
  if (it != double_to_index_map_.end()) {
    // duplicate constant pool
    Emit<OP::LOAD_CONST>(Instruction::SW(dst_, it->second));
    return;
  }

  // new constant value
  const uint32_t index = code_->constants_.size();
  code_->constants_.push_back(val);
  double_to_index_map_.insert(std::make_pair(val, index));
  Emit<OP::LOAD_CONST>(Instruction::SW(dst_, index));
}

inline void Compiler::Visit(const Assigned* lit) {
  UNREACHABLE();
}

inline void Compiler::Visit(const Identifier* lit) {
  const DestGuard dest_guard(this);
  if (RegisterID local = GetLocal(lit->symbol())) {
    dst_ = EmitMV(dst_, local);
  } else {
    dst_ = EmitOptimizedLookup(
        OP::LOAD_NAME, SymbolToNameIndex(lit->symbol()), dst_);
  }
}

inline void Compiler::Visit(const ThisLiteral* lit) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // this value is not used and ThisLiteral doesn't have side effect
    return;
  }
  assert(registers_.This()->IsConstant());
  dst_ = EmitMV(dst_, registers_.This());
}

inline void Compiler::Visit(const NullLiteral* lit) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // this value is not used and NullLiteral doesn't have side effect
    return;
  }
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  Emit<OP::LOAD_NULL>(dst_);
}

inline void Compiler::Visit(const TrueLiteral* lit) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // this value is not used and TrueLiteral doesn't have side effect
    return;
  }
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  Emit<OP::LOAD_TRUE>(dst_);
}

inline void Compiler::Visit(const FalseLiteral* lit) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // this value is not used and FalseLiteral doesn't have side effect
    return;
  }
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  Emit<OP::LOAD_FALSE>(dst_);
}

inline void Compiler::Visit(const RegExpLiteral* lit) {
  const DestGuard dest_guard(this);
  if (ignore_result()) {
    // this value is not used and RegExpLiteral doesn't have side effect
    return;
  }
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  Emit<OP::LOAD_REGEXP>(Instruction::SW(dst_, code_->constants_.size()));
  code_->constants_.push_back(
      JSRegExp::New(ctx_, lit->value(), lit->regexp()));
}

class Compiler::ArraySite {
 public:
  static const int kOnce = 50;
  explicit ArraySite(const ArrayLiteral* literal,
                     Compiler* compiler,
                     ThunkPool* thunkpool,
                     Registers* registers)
    : literal_(literal),
      ary_(compiler->Temporary(compiler->dst())),
      elements_(),
      compiler_(compiler),
      registers_(registers) {
    if (literal->SideEffect()) {
      thunkpool->ForceSpill();
    }
  }

  RegisterID ary() const { return ary_; }

  RegisterID Place(int32_t register_start, uint32_t i) {
    if (!elements_[i]) {
      elements_[i] = registers_->Acquire(register_start + i);
    }
    return elements_[i];
  }

  void Emit() {
    typedef ArrayLiteral::MaybeExpressions Items;
    compiler_->Emit<OP::LOAD_ARRAY>(
        Instruction::SW(ary_, literal_->items().size()));
    Items::const_iterator it = literal_->items().begin();
    const Items::const_iterator last = literal_->items().end();
    const uint32_t size = literal_->items().size();
    uint32_t rest = size;
    uint32_t idx = 0;

    while (it != last) {
      const int32_t dis =
          static_cast<int32_t>(std::min<uint32_t>(rest, ArraySite::kOnce));
      int32_t register_start = registers_->AcquireCallBase(dis);
      elements_.resize(dis, RegisterID());
      uint32_t i = 0;
      for (Items::const_iterator c = it + dis; it != c; ++it, ++i) {
        const core::Maybe<const Expression>& expr = *it;
        if (expr) {
          compiler_->EmitExpressionToDest(
              expr.Address(),
              Place(register_start, i));
        } else {
          compiler_->Emit<OP::LOAD_EMPTY>(Place(register_start, i));
        }
      }
      EmitElement(idx, dis);
      rest -= dis;
      idx += dis;
      elements_.clear();
    }
  }

 private:
  void EmitElement(uint32_t idx, uint32_t size) const {
    assert(!elements_.empty());
    if ((idx + size) > JSArray::kMaxVectorSize) {
      compiler_->Emit<OP::INIT_SPARSE_ARRAY_ELEMENT>(
          Instruction::Reg2(ary_, elements_.front()),
          Instruction::UInt32(idx, size));
    } else {
      compiler_->Emit<OP::INIT_VECTOR_ARRAY_ELEMENT>(
          Instruction::Reg2(ary_, elements_.front()),
          Instruction::UInt32(idx, size));
    }
  }
  const ArrayLiteral* literal_;
  RegisterID ary_;
  std::vector<RegisterID> elements_;
  Compiler* compiler_;
  Registers* registers_;
};

inline void Compiler::Visit(const ArrayLiteral* lit) {
  const DestGuard dest_guard(this);
  ArraySite site(lit, this, &thunkpool_, &registers_);
  site.Emit();
  dst_ = EmitMV(dst_, site.ary());
}

inline void Compiler::Visit(const ObjectLiteral* lit) {
  typedef ObjectLiteral::Properties Properties;
  const DestGuard dest_guard(this);
  const std::size_t arg_index = CurrentSize() + 2;
  RegisterID obj = Temporary(dst_);
  Emit<OP::LOAD_OBJECT>(obj, 0u);
  std::unordered_map<Symbol, std::size_t> slots;
  const Properties& properties = lit->properties();
  for (Properties::const_iterator it = properties.begin(),
       last = properties.end(); it != last; ++it) {
    const ObjectLiteral::Property& prop = *it;
    const ObjectLiteral::PropertyDescriptorType type(std::get<0>(prop));
    const Symbol name = std::get<1>(prop);

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

    RegisterID item = EmitExpression(std::get<2>(prop));
    if (type == ObjectLiteral::DATA) {
      Emit<OP::STORE_OBJECT_DATA>(
          Instruction::Reg2(obj, item), Instruction::UInt32(position, merged));
    } else if (type == ObjectLiteral::GET) {
      Emit<OP::STORE_OBJECT_GET>(
          Instruction::Reg2(obj, item), Instruction::UInt32(position, merged));
    } else {
      Emit<OP::STORE_OBJECT_SET>(
          Instruction::Reg2(obj, item), Instruction::UInt32(position, merged));
    }
  }
  Map* map = Map::NewObjectLiteralMap(ctx_, slots.begin(), slots.end());
  temporary_.push_back(map);
  Instruction inst(0u);
  inst.map = map;
  EmitArgAt(inst, arg_index);
  dst_ = EmitMV(dst_, obj);
}

inline void Compiler::Visit(const FunctionLiteral* lit) {
  const DestGuard dest_guard(this);
  Code* const code = new Code(ctx_, script_, *lit, core_, Code::FUNCTION);
  const uint32_t index = code_->codes_.size();
  code_->codes_.push_back(code);
  code_info_stack_.push_back(
      std::make_tuple(code, lit, current_variable_scope_));
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  Emit<OP::LOAD_FUNCTION>(Instruction::SW(dst_, index));
}

inline void Compiler::Visit(const IdentifierAccess* prop) {
  const DestGuard dest_guard(this);
  RegisterID base = EmitExpression(prop->target());
  const uint32_t index = SymbolToNameIndex(prop->key());
  dst_ = Dest(dst_);
  thunkpool_.Spill(dst_);
  Emit<OP::LOAD_PROP>(Instruction::SSW(dst_, base, index), 0, 0, 0);
}

inline void Compiler::Visit(const IndexAccess* prop) {
  const DestGuard dest_guard(this);
  dst_ = EmitElement<OP::LOAD_PROP, OP::LOAD_ELEMENT>(prop, dst_);
}

// Allocate Register if
//   f(1, 2, 3)
// is called, allocate like
//   r3[this]
//   r2[3]
//   r1[2]
//   r0[1]
template<typename Call>
class Compiler::CallSite {
 public:
  explicit CallSite(const Call& call,
                    Compiler* compiler,
                    ThunkPool* thunkpool,
                    Registers* registers)
    : call_(call),
      callee_(compiler->Temporary(compiler->dst())),
      args_(argc_with_this()),
      start_(),
      registers_(registers) {
    // spill heap thunks
    //   for example,
    //     function test() {
    //       var i = 10;
    //       function inner() {
    //         i = 20;
    //         return i;
    //       }
    //       return i + inner();
    //     }
    //
    // and if LHS has found, spill all thunks
    if (call.SideEffect()) {
      thunkpool->ForceSpill();
    }
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
        compiler->EmitExpressionToDest(*it, Arg(i));
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
inline RegisterID Compiler::EmitCall(const Call& call, RegisterID dst) {
  bool direct_call_to_eval = false;
  const Expression* target = call.target();
  CallSite<Call> site(call, this, &thunkpool_, &registers_);

  if (target->IsValidLeftHandSide()) {
    if (const Identifier* ident = target->AsIdentifier()) {
      if (RegisterID local = GetLocal(ident->symbol())) {
        EmitMV(site.callee(), local);
        Emit<OP::LOAD_UNDEFINED>(site.base());
      } else {
        // lookup dynamic call or not
        {
          const LookupInfo info = Lookup(ident->symbol());
          const uint32_t index = SymbolToNameIndex(ident->symbol());
          assert(info.type() != LookupInfo::STACK);
          if (info.type() == LookupInfo::LOOKUP) {
            Emit<OP::PREPARE_DYNAMIC_CALL>(
                Instruction::SSW(site.callee(), site.base(), index));
          } else {
            EmitOptimizedLookup(OP::LOAD_NAME, index, site.callee());
            Emit<OP::LOAD_UNDEFINED>(site.base());
          }
        }
      }
      if (op == OP::CALL && ident->symbol() == symbol::eval()) {
        direct_call_to_eval = true;
      }
    } else {
      assert(target->AsPropertyAccess());
      if (const IdentifierAccess* ac = target->AsIdentifierAccess()) {
        // IdentifierAccess
        EmitExpressionToDest(ac->target(), site.base());
        const uint32_t index = SymbolToNameIndex(ac->key());
        Emit<OP::LOAD_PROP>(
            Instruction::SSW(site.callee(), site.base(), index), 0, 0, 0);
      } else {
        // IndexAccess
        const IndexAccess* ai = target->AsIndexAccess();
        EmitExpressionToDest(ai->target(), site.base());
        const Symbol sym = PropertyName(ai->key());
        if (sym != symbol::kDummySymbol) {
          const uint32_t index = SymbolToNameIndex(sym);
          Emit<OP::LOAD_PROP>(
              Instruction::SSW(site.callee(), site.base(), index), 0, 0, 0);
        } else {
          EmitExpressionToDest(ai->key(), site.callee());
          Emit<OP::LOAD_ELEMENT>(
              Instruction::Reg3(site.callee(), site.base(), site.callee()));
        }
      }
    }
  } else {
    EmitExpressionToDest(target, site.callee());
    Emit<OP::LOAD_UNDEFINED>(site.base());
  }

  site.EmitArguments(this);

  dst = Dest(dst, site.callee());
  thunkpool_.Spill(dst);
  if (direct_call_to_eval) {
    Emit<OP::EVAL>(
        Instruction::Reg3(dst, site.callee(), site.GetFirstPosition()),
        Instruction::Int32(site.argc_with_this(), 0));
  } else {
    Emit<op>(
        Instruction::Reg3(dst, site.callee(), site.GetFirstPosition()),
        Instruction::Int32(site.argc_with_this(), 0));
  }
  assert(registers_.IsLiveTop(site.base()->register_offset()));
  return dst;
}


inline void Compiler::Visit(const FunctionCall* call) {
  const DestGuard dest_guard(this);
  dst_ = EmitCall<OP::CALL>(*call, dst_);
}

inline void Compiler::Visit(const ConstructorCall* call) {
  const DestGuard dest_guard(this);
  dst_ = EmitCall<OP::CONSTRUCT>(*call, dst_);
}

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_COMPILER_EXPRESSION_H_
