#ifndef IV_LV5_RAILGUN_ANALYZER_H_
#define IV_LV5_RAILGUN_ANALYZER_H_
#include <iv/lv5/specialized_ast.h>
#include <iv/lv5/railgun/fwd.h>
namespace iv {
namespace lv5 {
namespace railgun {

class Analyzer {
 public:
  class AssignmentVisitor : public ExpressionVisitor {
   public:
    AssignmentVisitor() : found_(false) { }
    ~AssignmentVisitor() { };

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
      for (ObjectLiteral::Properties::const_iterator it = literal->properties().begin(),
           last = literal->properties().end(); it != last; ++it) {
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
};


} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_ANALYZER_H_
