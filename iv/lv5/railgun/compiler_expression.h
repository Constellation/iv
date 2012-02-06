#ifndef IV_LV5_RAILGUN_COMPILER_EXPRESSION_H_
#define IV_LV5_RAILGUN_COMPILER_EXPRESSION_H_
#include <iv/lv5/railgun/compiler.h>
namespace iv {
namespace lv5 {
namespace railgun {

inline void Compiler::Visit(const Assignment* assign) {
  using core::Token;
  const DestGuard dest_guard(this);
  const Token::Type token = assign->op();
  if (token == Token::TK_ASSIGN) {
    const Expression* lhs = assign->left();
    const Expression* rhs = assign->right();
    if (!lhs->IsValidLeftHandSide()) {
      EmitExpression(lhs);
      dst_ = EmitExpression(rhs, dst_);
      Emit<OP::RAISE_REFERENCE>();
      return;
    }
    assert(lhs->IsValidLeftHandSide());
    if (const Identifier* ident = lhs->AsIdentifier()) {
      // Identifier
      if (RegisterID local = GetLocal(ident->symbol())) {
        const LookupInfo info = Lookup(ident->symbol());
        if (info.immutable()) {
          local = dst_ ? dst_ : registers_.Acquire();
        }
        dst_ = EmitMV(dst_, EmitExpression(rhs, local));
        if (code_->strict() && info.immutable()) {
          Emit<OP::RAISE_IMMUTABLE>(SymbolToNameIndex(ident->symbol()));
        }
      } else {
        dst_ = EmitExpression(rhs, dst_);
        EmitStore(ident->symbol(), dst_);
      }
      return;
    } else if (lhs->AsPropertyAccess()) {
      // PropertyAccess
      if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
        // IdentifierAccess
        Thunk base(&thunklist_, EmitExpression(ac->target()));
        dst_ = EmitExpression(rhs, dst_);
        const uint32_t index = SymbolToNameIndex(ac->key());
        Emit<OP::STORE_PROP>(base.Release(), index, dst_, 0, 0, 0, 0);
        return;
      } else {
        // IndexAccess
        const IndexAccess* idx = lhs->AsIndexAccess();
        const Expression* key = idx->key();
        if (const StringLiteral* str = key->AsStringLiteral()) {
          Thunk base(&thunklist_, EmitExpression(idx->target()));
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, str->value()));
          dst_ = EmitExpression(rhs, dst_);
          Emit<OP::STORE_PROP>(base.Release(), index, dst_, 0, 0, 0, 0);
          return;
        } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
          Thunk base(&thunklist_, EmitExpression(idx->target()));
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, num->value()));
          dst_ = EmitExpression(rhs, dst_);
          Emit<OP::STORE_PROP>(base.Release(), index, dst_, 0, 0, 0, 0);
          return;
        } else {
          Thunk base(&thunklist_, EmitExpression(idx->target()));
          Thunk element(&thunklist_, EmitExpression(key));
          dst_ = EmitExpression(rhs, dst_);
          Emit<OP::STORE_ELEMENT>(base.Release(), element.Release(), dst_);
          return;
        }
      }
    } else {
      // FunctionCall
      // ConstructorCall
      EmitExpression(lhs);
      dst_ = EmitExpression(rhs, dst_);
      Emit<OP::RAISE_REFERENCE>();
      return;
    }
  } else {
    const Expression* lhs = assign->left();
    const Expression* rhs = assign->right();
    if (!lhs->IsValidLeftHandSide()) {
      EmitExpression(lhs);
      dst_ = EmitExpression(rhs, dst_);
      Emit<OP::RAISE_REFERENCE>();
      return;
    }
    assert(lhs->IsValidLeftHandSide());
    if (const Identifier* ident = lhs->AsIdentifier()) {
      // Identifier
      const LookupInfo info = Lookup(ident->symbol());
      if (RegisterID local = GetLocal(ident->symbol())) {
        Thunk lv(&thunklist_, EmitExpression(lhs));
        RegisterID rv = EmitExpression(rhs);
        if (info.immutable()) {
          local = dst_ ? dst_ : registers_.Acquire();
        }
        lv.Release();
        thunklist_.Spill(local);
        Emit(OP::BinaryOP(token), local, lv.reg(), rv);
        dst_ = EmitMV(dst_, local);
        if (code_->strict() && info.immutable()) {
          Emit<OP::RAISE_IMMUTABLE>(SymbolToNameIndex(ident->symbol()));
        }
      } else {
        Thunk lv(&thunklist_, EmitExpression(lhs));
        RegisterID rv = EmitExpression(rhs);
        lv.Release();
        dst_ = Dest(dst_, lv.reg(), rv);
        thunklist_.Spill(dst_);
        Emit(OP::BinaryOP(token), dst_, lv.reg(), rv);
        EmitStore(ident->symbol(), dst_);
      }
    } else if (lhs->AsPropertyAccess()) {
      // PropertyAccess
      if (const IdentifierAccess* ac = lhs->AsIdentifierAccess()) {
        // IdentifierAccess
        Thunk base(&thunklist_, EmitExpression(ac->target()));
        const uint32_t index = SymbolToNameIndex(ac->key());
        {
          RegisterID prop = registers_.Acquire();
          Emit<OP::LOAD_PROP>(prop, base.reg(), index, 0, 0, 0, 0);
          RegisterID tmp = EmitExpression(rhs);
          dst_ = Dest(dst_, tmp, prop);
          thunklist_.Spill(dst_);
          Emit(OP::BinaryOP(token), dst_, prop, tmp);
        }
        Emit<OP::STORE_PROP>(base.Release(), index, dst_, 0, 0, 0, 0);
        return;
      } else {
        // IndexAccess
        const IndexAccess* idx = lhs->AsIndexAccess();
        const Expression* key = idx->key();
        if (const StringLiteral* str = key->AsStringLiteral()) {
          Thunk base(&thunklist_, EmitExpression(idx->target()));
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, str->value()));
          {
            RegisterID prop = registers_.Acquire();
            Emit<OP::LOAD_PROP>(prop, base.reg(), index, 0, 0, 0, 0);
            RegisterID tmp = EmitExpression(rhs);
            dst_ = Dest(dst_, tmp, prop);
            thunklist_.Spill(dst_);
            Emit(OP::BinaryOP(token), dst_, prop, tmp);
          }
          Emit<OP::STORE_PROP>(base.Release(), index, dst_, 0, 0, 0, 0);
          return;
        } else if (const NumberLiteral* num = key->AsNumberLiteral()) {
          Thunk base(&thunklist_, EmitExpression(idx->target()));
          const uint32_t index =
              SymbolToNameIndex(context::Intern(ctx_, num->value()));
          {
            RegisterID prop = registers_.Acquire();
            Emit<OP::LOAD_PROP>(prop, base.reg(), index, 0, 0, 0, 0);
            RegisterID tmp = EmitExpression(rhs);
            dst_ = Dest(dst_, tmp, prop);
            thunklist_.Spill(dst_);
            Emit(OP::BinaryOP(token), dst_, prop, tmp);
          }
          Emit<OP::STORE_PROP>(base.Release(), index, dst_, 0, 0, 0, 0);
          return;
        } else {
          Thunk base(&thunklist_, EmitExpression(idx->target()));
          Thunk element(&thunklist_, EmitExpression(key));
          {
            RegisterID prop = registers_.Acquire();
            Emit<OP::LOAD_ELEMENT>(prop, base.reg(), element.reg());
            RegisterID tmp = EmitExpression(rhs);
            dst_ = Dest(dst_, tmp, prop);
            thunklist_.Spill(dst_);
            Emit(OP::BinaryOP(token), dst_, prop, tmp);
          }
          Emit<OP::STORE_ELEMENT>(base.Release(), element.Release(), dst_);
        }
      }
    } else {
      // FunctionCall
      // ConstructorCall
      Thunk src(&thunklist_, EmitExpression(lhs));
      {
        RegisterID tmp = EmitExpression(rhs);
        dst_ = Dest(dst_, src.Release(), tmp);
        thunklist_.Spill(dst_);
        Emit(OP::BinaryOP(token), dst_, src.reg(), tmp);
      }
      Emit<OP::RAISE_REFERENCE>();
    }
  }
}

inline void Compiler::Visit(const BinaryOperation* binary) {
  using core::Token;
  const DestGuard dest_guard(this);
  const Token::Type token = binary->op();
  const Expression* lhs = binary->left();
  const Expression* rhs = binary->right();
  switch (token) {
    case Token::TK_LOGICAL_AND: {  // &&
      std::size_t label;
      dst_ = Dest(dst_);
      thunklist_.Spill(dst_);
      {
        dst_ = EmitExpression(lhs, dst_);
        label = CurrentSize();
        Emit<OP::IF_FALSE>(0, dst_);
      }
      dst_ = EmitExpression(rhs, dst_);
      EmitJump(CurrentSize(), label);
      break;
    }

    case Token::TK_LOGICAL_OR: {  // ||
      std::size_t label;
      dst_ = Dest(dst_);
      thunklist_.Spill(dst_);
      {
        dst_ = EmitExpression(lhs, dst_);
        label = CurrentSize();
        Emit<OP::IF_TRUE>(0, dst_);
      }
      dst_ = EmitExpression(rhs, dst_);
      EmitJump(CurrentSize(), label);
      break;
    }

    case Token::TK_COMMA: {  // ,
      EmitExpression(lhs);
      dst_ = EmitExpression(rhs, dst_);
      break;
    }

    default: {
      Thunk lv(&thunklist_, EmitExpression(lhs));
      RegisterID rv = EmitExpression(rhs);
      dst_ = Dest(dst_, lv.Release(), rv);
      thunklist_.Spill(dst_);
      Emit(OP::BinaryOP(token), dst_, lv.reg(), rv);
    }
  }
}

inline void Compiler::Visit(const ConditionalExpression* cond) {
  const DestGuard dest_guard(this);
  std::size_t first;
  {
    RegisterID ret = EmitExpression(cond->cond());
    first = CurrentSize();
    Emit<OP::IF_FALSE>(0, ret);
  }
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  dst_ = EmitExpression(cond->left(), dst_);
  const std::size_t second = CurrentSize();
  Emit<OP::JUMP_BY>(0);
  EmitJump(CurrentSize(), first);
  dst_ = EmitExpression(cond->right(), dst_);
  EmitJump(CurrentSize(), second);
}

inline void Compiler::Visit(const UnaryOperation* unary) {
  using core::Token;
  const DestGuard dest_guard(this);
  const Token::Type token = unary->op();
  const Expression* expr = unary->expr();
  switch (token) {
    case Token::TK_DELETE: {
      if (expr->IsValidLeftHandSide()) {
        // Identifier
        // PropertyAccess
        // FunctionCall
        // ConstructorCall
        if (const Identifier* ident = expr->AsIdentifier()) {
          // DELETE_NAME_STRICT is already rejected in parser
          assert(!code_->strict());
          if (RegisterID local = GetLocal(ident->symbol())) {
            dst_ = Dest(dst_);
            thunklist_.Spill(dst_);
            Emit<OP::LOAD_FALSE>(dst_);
          } else {
            dst_ = EmitOptimizedLookup(OP::DELETE_NAME,
                                       SymbolToNameIndex(ident->symbol()), dst_);
          }
        } else if (expr->AsPropertyAccess()) {
          if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
            // IdentifierAccess
            RegisterID base = EmitExpression(ac->target());
            const uint32_t index = SymbolToNameIndex(ac->key());
            dst_ = Dest(dst_);
            thunklist_.Spill(dst_);
            Emit<OP::DELETE_PROP>(dst_, base, index, 0, 0, 0, 0);
          } else {
            // IndexAccess
            dst_ = EmitElement<
                 OP::DELETE_PROP,
                 OP::DELETE_ELEMENT>(expr->AsIndexAccess(), dst_);
          }
        } else {
          EmitExpression(expr);
          dst_ = Dest(dst_);
          thunklist_.Spill(dst_);
          Emit<OP::LOAD_TRUE>(dst_);
        }
      } else {
        // other case is no effect
        // but accept expr
        EmitExpression(expr);
        dst_ = Dest(dst_);
        thunklist_.Spill(dst_);
        Emit<OP::LOAD_TRUE>(dst_);
      }
      break;
    }

    case Token::TK_VOID: {
      EmitExpression(expr);
      dst_ = Dest(dst_);
      thunklist_.Spill(dst_);
      Emit<OP::LOAD_UNDEFINED>(dst_);
      break;
    }

    case Token::TK_TYPEOF: {
      if (const Identifier* ident = expr->AsIdentifier()) {
        // maybe Global Reference
        if (RegisterID local = GetLocal(ident->symbol())) {
          dst_ = Dest(dst_);
          thunklist_.Spill(dst_);
          Emit<OP::TYPEOF>(dst_, local);
        } else {
          const uint32_t index = SymbolToNameIndex(ident->symbol());
          dst_ = EmitOptimizedLookup(OP::TYPEOF_NAME, index, dst_);
        }
      } else {
        RegisterID src = EmitExpression(expr);
        dst_ = Dest(dst_, src);
        thunklist_.Spill(dst_);
        Emit<OP::TYPEOF>(dst_, src);
      }
      break;
    }

    case Token::TK_INC:
    case Token::TK_DEC: {
      if (!expr->IsValidLeftHandSide()) {
        dst_ = EmitExpression(expr, dst_);
        Emit<OP::TO_NUMBER_AND_RAISE_REFERENCE>(dst_->reg());
        return;
      }
      assert(expr->IsValidLeftHandSide());
      if (const Identifier* ident = expr->AsIdentifier()) {
        const uint32_t index = SymbolToNameIndex(ident->symbol());
        if (RegisterID local = GetLocal(ident->symbol())) {
          const LookupInfo info = Lookup(ident->symbol());
          if (info.immutable()) {
            local = EmitMV(dst_ ? dst_ : registers_.Acquire(), local);
          }
          thunklist_.Spill(local);
          Emit((token == Token::TK_INC) ?
               OP::INCREMENT : OP::DECREMENT, local);
          dst_ = EmitMV(dst_, local);
          if (code_->strict() && info.immutable()) {
            Emit<OP::RAISE_IMMUTABLE>(index);
          }
        } else {
          dst_ = EmitOptimizedLookup(
              (token == Token::TK_INC) ?
              OP::INCREMENT_NAME : OP::DECREMENT_NAME, index, dst_);
        }
        return;
      } else if (expr->AsPropertyAccess()) {
        if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
          // IdentifierAccess
          RegisterID base = EmitExpression(ac->target());
          const uint32_t index = SymbolToNameIndex(ac->key());
          dst_ = Dest(dst_);
          thunklist_.Spill(dst_);
          if (token == Token::TK_INC) {
            Emit<OP::INCREMENT_PROP>(dst_, base, index, 0, 0, 0, 0);
          } else {
            Emit<OP::DECREMENT_PROP>(dst_, base, index, 0, 0, 0, 0);
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
      } else {
        dst_ = EmitExpression(expr, dst_);
        Emit<OP::TO_NUMBER_AND_RAISE_REFERENCE>(dst_);
      }
      break;
    }

    default: {
      RegisterID src = EmitExpression(expr);
      dst_ = Dest(dst_, src);
      thunklist_.Spill(dst_);
      Emit(OP::UnaryOP(token), dst_, src);
    }
  }
}

inline void Compiler::Visit(const PostfixExpression* postfix) {
  using core::Token;
  const DestGuard dest_guard(this);
  const Expression* expr = postfix->expr();
  const Token::Type token = postfix->op();
  if (!expr->IsValidLeftHandSide()) {
    dst_ = EmitExpression(expr, dst_);
    Emit<OP::TO_NUMBER_AND_RAISE_REFERENCE>(dst_->reg());
    return;
  }
  assert(expr->IsValidLeftHandSide());
  if (const Identifier* ident = expr->AsIdentifier()) {
    const uint32_t index = SymbolToNameIndex(ident->symbol());
    if (RegisterID local = GetLocal(ident->symbol())) {
      const LookupInfo info = Lookup(ident->symbol());
      dst_ = Dest(dst_);
      thunklist_.Spill(dst_);
      if (info.immutable() || dst_ == local) {
        local = EmitMV(registers_.Acquire(), local);
      }
      Emit((token == Token::TK_INC) ?
           OP::POSTFIX_INCREMENT : OP::POSTFIX_DECREMENT, dst_, local);
      if (code_->strict() && info.immutable()) {
        Emit<OP::RAISE_IMMUTABLE>(index);
      }
    } else {
      dst_ = EmitOptimizedLookup(
          (token == Token::TK_INC) ?
          OP::POSTFIX_INCREMENT_NAME : OP::POSTFIX_DECREMENT_NAME, index, dst_);
    }
  } else if (expr->AsPropertyAccess()) {
    if (const IdentifierAccess* ac = expr->AsIdentifierAccess()) {
      // IdentifierAccess
      RegisterID base = EmitExpression(ac->target());
      const uint32_t index = SymbolToNameIndex(ac->key());
      dst_ = Dest(dst_);
      thunklist_.Spill(dst_);
      if (token == Token::TK_INC) {
        Emit<OP::POSTFIX_INCREMENT_PROP>(dst_, base, index, 0, 0, 0, 0);
      } else {
        Emit<OP::POSTFIX_DECREMENT_PROP>(dst_, base, index, 0, 0, 0, 0);
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
  } else {
    dst_ = EmitExpression(expr, dst_);
    Emit<OP::TO_NUMBER_AND_RAISE_REFERENCE>(dst_);
  }
}

inline void Compiler::Visit(const StringLiteral* lit) {
  const DestGuard dest_guard(this);
  const core::UString s = core::ToUString(lit->value());
  const JSStringToIndexMap::const_iterator it = jsstring_to_index_map_.find(s);
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  if (it != jsstring_to_index_map_.end()) {
    // duplicate constant
    Emit<OP::LOAD_CONST>(dst_, it->second);
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
  Emit<OP::LOAD_CONST>(dst_, index);
}

inline void Compiler::Visit(const NumberLiteral* lit) {
  const DestGuard dest_guard(this);
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  const double val = lit->value();
  const int32_t i32 = static_cast<int32_t>(val);
  if (val == i32 && (i32 || !core::math::Signbit(val))) {
    // boxing int32_t
    Instruction inst(0u);
    inst.i32 = i32;
    Emit<OP::LOAD_INT32>(dst_, inst);
    return;
  }

  const uint32_t ui32 = static_cast<uint32_t>(val);
  if (val == ui32 && (ui32 || !core::math::Signbit(val))) {
    // boxing uint32_t
    Emit<OP::LOAD_UINT32>(dst_, ui32);
    return;
  }

  const JSDoubleToIndexMap::const_iterator it =
      double_to_index_map_.find(val);
  if (it != double_to_index_map_.end()) {
    // duplicate constant pool
    Emit<OP::LOAD_CONST>(dst_, it->second);
    return;
  }

  // new constant value
  const uint32_t index = code_->constants_.size();
  code_->constants_.push_back(val);
  double_to_index_map_.insert(std::make_pair(val, index));
  Emit<OP::LOAD_CONST>(dst_, index);
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
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  Emit<OP::LOAD_THIS>(dst_);
}

inline void Compiler::Visit(const NullLiteral* lit) {
  const DestGuard dest_guard(this);
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  Emit<OP::LOAD_NULL>(dst_);
}

inline void Compiler::Visit(const TrueLiteral* lit) {
  const DestGuard dest_guard(this);
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  Emit<OP::LOAD_TRUE>(dst_);
}

inline void Compiler::Visit(const FalseLiteral* lit) {
  const DestGuard dest_guard(this);
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  Emit<OP::LOAD_FALSE>(dst_);
}

inline void Compiler::Visit(const RegExpLiteral* lit) {
  const DestGuard dest_guard(this);
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  Emit<OP::LOAD_REGEXP>(dst_, code_->constants_.size());
  code_->constants_.push_back(
      JSRegExp::New(ctx_, lit->value(), lit->regexp()));
}

inline void Compiler::Visit(const ArrayLiteral* lit) {
  typedef ArrayLiteral::MaybeExpressions Items;
  const DestGuard dest_guard(this);
  const Items& items = lit->items();
  RegisterID ary = registers_.Acquire();
  Emit<OP::LOAD_ARRAY>(ary, items.size());
  uint32_t current = 0;
  for (Items::const_iterator it = items.begin(),
       last = items.end(); it != last; ++it, ++current) {
    const core::Maybe<const Expression>& expr = *it;
    if (expr) {
      RegisterID item = EmitExpression(expr.Address());
      if (JSArray::kMaxVectorSize > current) {
        Emit<OP::INIT_VECTOR_ARRAY_ELEMENT>(ary, item, current);
      } else {
        Emit<OP::INIT_SPARSE_ARRAY_ELEMENT>(ary, item, current);
      }
    }
  }
  dst_ = EmitMV(dst_, ary);
}

inline void Compiler::Visit(const ObjectLiteral* lit) {
  using std::get;
  typedef ObjectLiteral::Properties Properties;
  const DestGuard dest_guard(this);
  const std::size_t arg_index = CurrentSize() + 2;
  RegisterID obj = registers_.Acquire();
  Emit<OP::LOAD_OBJECT>(obj, 0u);
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
      Emit<OP::STORE_OBJECT_DATA>(obj, item, position, merged);
    } else if (type == ObjectLiteral::GET) {
      Emit<OP::STORE_OBJECT_GET>(obj, item, position, merged);
    } else {
      Emit<OP::STORE_OBJECT_SET>(obj, item, position, merged);
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
  thunklist_.Spill(dst_);
  Emit<OP::LOAD_FUNCTION>(dst_, index);
}

inline void Compiler::Visit(const IdentifierAccess* prop) {
  const DestGuard dest_guard(this);
  RegisterID base = EmitExpression(prop->target());
  const uint32_t index = SymbolToNameIndex(prop->key());
  dst_ = Dest(dst_);
  thunklist_.Spill(dst_);
  Emit<OP::LOAD_PROP>(dst_, base, index, 0, 0, 0, 0);
}

inline void Compiler::Visit(const IndexAccess* prop) {
  const DestGuard dest_guard(this);
  dst_ = EmitElement<OP::LOAD_PROP, OP::LOAD_ELEMENT>(prop, dst_);
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
