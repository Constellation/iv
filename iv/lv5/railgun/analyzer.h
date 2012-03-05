#ifndef IV_LV5_RAILGUN_ANALYZER_H_
#define IV_LV5_RAILGUN_ANALYZER_H_
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/railgun/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Analyzer {
 public:
  // Assignment
  class AssignmentVisitor : public ExpressionVisitor {
   public:
    AssignmentVisitor() : found_(false) { }

    ~AssignmentVisitor() { }

    void Visit(const Assignment* assign) {
      if (assign->left()->AsIdentifier()) {
        found_ = true;
        return;
      }
      assign->left()->AcceptExpressionVisitor(this);
      if (found()) {
        return;
      }
      assign->right()->AcceptExpressionVisitor(this);
    }

    void Visit(const BinaryOperation* binary) {
      binary->left()->AcceptExpressionVisitor(this);
      if (found()) {
        return;
      }
      binary->right()->AcceptExpressionVisitor(this);
    }

    void Visit(const ConditionalExpression* cond) {
      cond->cond()->AcceptExpressionVisitor(this);
      if (found()) {
        return;
      }
      cond->left()->AcceptExpressionVisitor(this);
      if (found()) {
        return;
      }
      cond->right()->AcceptExpressionVisitor(this);
    }

    void Visit(const UnaryOperation* unary) {
      unary->expr()->AcceptExpressionVisitor(this);
    }

    void Visit(const PostfixExpression* postfix) {
      if (postfix->expr()->AsIdentifier()) {
        found_ = true;
        return;
      }
      postfix->expr()->AcceptExpressionVisitor(this);
    }

    void Visit(const Assigned* assigned) { }
    void Visit(const StringLiteral* literal) { }
    void Visit(const NumberLiteral* literal) { }
    void Visit(const Identifier* literal) { }
    void Visit(const ThisLiteral* literal) { }
    void Visit(const NullLiteral* literal) { }
    void Visit(const TrueLiteral* literal) { }
    void Visit(const FalseLiteral* literal) { }
    void Visit(const RegExpLiteral* literal) { }

    void Visit(const ArrayLiteral* literal) {
      for (MaybeExpressions::const_iterator it = literal->items().begin(),
           last = literal->items().end(); it != last; ++it) {
        if (*it) {
          (*it).Address()->AcceptExpressionVisitor(this);
          if (found()) {
            return;
          }
        }
      }
    }

    void Visit(const ObjectLiteral* literal) {
      for (ObjectLiteral::Properties::const_iterator
           it = literal->properties().begin(),
           last = literal->properties().end();
           it != last; ++it) {
        std::get<2>(*it)->AcceptExpressionVisitor(this);
        if (found()) {
          return;
        }
      }
    }

    void Visit(const FunctionLiteral* literal) { }

    void Visit(const IdentifierAccess* prop) {
      prop->target()->AcceptExpressionVisitor(this);
    }

    void Visit(const IndexAccess* prop) {
      prop->target()->AcceptExpressionVisitor(this);
      if (found()) {
        return;
      }
      prop->key()->AcceptExpressionVisitor(this);
    }

    void Visit(const FunctionCall* call) {
      call->target()->AcceptExpressionVisitor(this);
      if (found()) {
        return;
      }
      for (Expressions::const_iterator it = call->args().begin(),
           last = call->args().end(); it != last; ++it) {
        (*it)->AcceptExpressionVisitor(this);
        if (found()) {
          return;
        }
      }
    }

    void Visit(const ConstructorCall* call) {
      call->target()->AcceptExpressionVisitor(this);
      if (found()) {
        return;
      }
      for (Expressions::const_iterator it = call->args().begin(),
           last = call->args().end(); it != last; ++it) {
        (*it)->AcceptExpressionVisitor(this);
        if (found()) {
          return;
        }
      }
    }

    bool found() const { return found_; }
   private:
    bool found_;
  };

  static bool ExpressionHasAssignment(const Expression* expr) {
    AssignmentVisitor visitor;
    expr->AcceptExpressionVisitor(&visitor);
    return visitor.found();
  }

  // TODO(Constellation) calculate it in parser phase
  // IsString
  class IsStringVisitor : public ExpressionVisitor {
   public:
    typedef core::Token Token;
    enum Type {
      TYPE_STRING,
      TYPE_NUMBER,
      TYPE_NULL,
      TYPE_UNDEFINED,
      TYPE_BOOL,
      TYPE_OTHER
    };

    Type Evaluate(const Expression* expr) {
      expr->AcceptExpressionVisitor(this);
      return type_;
    }

    IsStringVisitor() : type_() { }

    void Visit(const Assignment* assign) {
      // LHS type
      if (assign->op() == Token::TK_ASSIGN) {
        type_ = Evaluate(assign->right());
      } else {
        type_ = BinaryEvaluate(assign);
      }
    }

    template<typename T>
    Type BinaryEvaluate(const T* binary) {
      switch (binary->op()) {
        case Token::TK_ASSIGN_ADD:
        case Token::TK_ADD: {
          const Type type1 = Evaluate(binary->left());
          const Type type2 = Evaluate(binary->left());
          if (type1 == TYPE_STRING || type2 == TYPE_STRING) {
            type_ = TYPE_STRING;
          } else if (type1 == TYPE_NUMBER || type2 == TYPE_NUMBER) {
            type_ = TYPE_NUMBER;
          } else {
            type_ = TYPE_OTHER;  // String or Number anyway
          }
          break;
        }

        case Token::TK_ASSIGN_SUB:
        case Token::TK_SUB:
        case Token::TK_ASSIGN_SHR:
        case Token::TK_SHR:
        case Token::TK_ASSIGN_SAR:
        case Token::TK_SAR:
        case Token::TK_ASSIGN_SHL:
        case Token::TK_SHL:
        case Token::TK_ASSIGN_MUL:
        case Token::TK_MUL:
        case Token::TK_ASSIGN_DIV:
        case Token::TK_DIV:
        case Token::TK_ASSIGN_MOD:
        case Token::TK_MOD:
        case Token::TK_ASSIGN_BIT_AND:
        case Token::TK_BIT_AND:
        case Token::TK_ASSIGN_BIT_XOR:
        case Token::TK_BIT_XOR:
        case Token::TK_ASSIGN_BIT_OR:
        case Token::TK_BIT_OR:
          type_ = TYPE_NUMBER;
          break;

        case Token::TK_LT:
        case Token::TK_LTE:
        case Token::TK_GT:
        case Token::TK_GTE:
        case Token::TK_INSTANCEOF:
        case Token::TK_IN:
        case Token::TK_EQ:
        case Token::TK_NE:
        case Token::TK_EQ_STRICT:
        case Token::TK_NE_STRICT:
          type_ = TYPE_BOOL;
          break;

        case Token::TK_COMMA:
          type_ = Evaluate(binary->right());
          break;

        case Token::TK_LOGICAL_AND:
        case Token::TK_LOGICAL_OR: {
          const Type type1 = Evaluate(binary->left());
          const Type type2 = Evaluate(binary->right());
          if (type1 == type2) {
            type_ = type1;
          } else {
            // TODO(Constellation) we can calculate falsy or truly
            type_ = TYPE_OTHER;
          }
          break;
        }

        default:
        UNREACHABLE();
      }
      return type_;
    }

    void Visit(const BinaryOperation* binary) {
      type_ = BinaryEvaluate(binary);
    }

    void Visit(const ConditionalExpression* cond) {
      const Type type1 = Evaluate(cond->left());
      const Type type2 = Evaluate(cond->right());
      if (type1 == type2) {
        type_ = type1;
      } else {
        type_ = TYPE_OTHER;
      }
    }

    void Visit(const UnaryOperation* unary) {
      switch (unary->op()) {
        case Token::TK_ADD:
        case Token::TK_SUB:
        case Token::TK_INC:
        case Token::TK_DEC:
        case Token::TK_BIT_NOT:
          type_ = TYPE_NUMBER;
          break;
        case Token::TK_NOT:
        case Token::TK_DELETE:
          type_ = TYPE_BOOL;
          break;
        case Token::TK_VOID:
          type_ = TYPE_UNDEFINED;
          break;
        case Token::TK_TYPEOF:
          type_ = TYPE_STRING;
          break;
        default:
          UNREACHABLE();
      }
    }

    void Visit(const PostfixExpression* postfix) {
      type_ = TYPE_NUMBER;
    }

    void Visit(const Assigned* assigned) {
      UNREACHABLE();
    }

    void Visit(const StringLiteral* literal) {
      type_ = TYPE_STRING;
    }

    void Visit(const NumberLiteral* literal) {
      type_ = TYPE_NUMBER;
    }

    void Visit(const Identifier* literal) {
      type_ = TYPE_OTHER;
    }

    void Visit(const ThisLiteral* literal) {
      type_ = TYPE_OTHER;
    }

    void Visit(const NullLiteral* literal) {
      type_ = TYPE_NULL;
    }

    void Visit(const TrueLiteral* literal) {
      type_ = TYPE_BOOL;
    }

    void Visit(const FalseLiteral* literal) {
      type_ = TYPE_BOOL;
    }

    void Visit(const RegExpLiteral* literal) {
      type_ = TYPE_OTHER;
    }

    void Visit(const ArrayLiteral* literal) {
      type_ = TYPE_OTHER;
    }

    void Visit(const ObjectLiteral* literal) {
      type_ = TYPE_OTHER;
    }

    void Visit(const FunctionLiteral* literal) {
      type_ = TYPE_OTHER;
    }

    void Visit(const IdentifierAccess* prop) {
      type_ = TYPE_OTHER;
    }

    void Visit(const IndexAccess* prop) {
      type_ = TYPE_OTHER;
    }

    void Visit(const FunctionCall* call) {
      type_ = TYPE_OTHER;
    }

    void Visit(const ConstructorCall* call) {
      type_ = TYPE_OTHER;
    }

    bool IsString() const { return type_ == TYPE_STRING; }

   private:
    Type type_;
  };

  static bool IsString(const Expression* expr) {
    IsStringVisitor visitor;
    expr->AcceptExpressionVisitor(&visitor);
    return visitor.IsString();
  }
};


} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_ANALYZER_H_
