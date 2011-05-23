// railgun analyzer
//   analyze variable (global / local / heap) and eval, arguments, with
#ifndef _IV_LV5_RAILGUN_ANALYZIER_H_
#define _IV_LV5_RAILGUN_ANALYZIER_H_
#include <utility>
#include <tr1/unordered_map>
#include <tr1/memory>
#include "ast.h"
#include "ast_visitor.h"
#include "lv5/specialized_ast.h"
#include "lv5/symbol.h"
#include "lv5/railgun_fwd.h"
namespace iv {
namespace lv5 {
namespace railgun {

class Analyzer
    : private core::Noncopyable<Analyzer>,
      public AstVisitor {
 public:
  struct Variable {
    enum Type {
      STACK,
      HEAP,
      GLOBAL
    };
  };

  typedef std::tr1::unordered_map<Symbol, Variable::Type> Map;
  struct Entry : public std::pair<Map, Entry*> { };
  typedef std::tr1::unordered_map<const FunctionLiteral*, std::tr1::shared_ptr<Entry> > Table;
  typedef std::pair<std::tr1::shared_ptr<Table>, std::tr1::shared_ptr<Entry> > Root;

  static Entry* MakeEntry(std::tr1::shared_ptr<Table> table,
                          const FunctionLiteral& func,
                          Entry* upper) {
    std::tr1::shared_ptr<Entry> entry(new Entry);
    entry->second = upper;
    table->insert(std::make_pair(&func, entry));
    return entry.get();
  }

  Analyzer() : table_(), current_() { }

  Root Analyze(const FunctionLiteral& global) {
    table_ = std::tr1::shared_ptr<Table>(new Table());
    current_ = MakeEntry(table_, global, NULL);
    VisitFunction(global, Variable::GLOBAL);
    return std::make_pair(table_, current_);
  }

 private:
  void VisitFunction(const FunctionLiteral& func, Variable::Type stack_type) {
    const Scope& scope = func.scope();
    {
      typedef Scope::FunctionLiterals Functions;
      const Functions& functions = scope.function_declarations();
      for (Functions::const_iterator it = functions.begin(),
           last = functions.end(); it != last; ++it) {
        if (core::Maybe<const Identifier> name = (*it)->name()) {
          current_->first.insert(std::make_pair(name.Address()->symbol(), stack_type));
        }
      }
    }
    {
      // variables
      typedef Scope::Variables Variables;
      const Variables& vars = scope.variables();
      for (Variables::const_iterator it = vars.begin(),
           last = vars.end(); it != last; ++it) {
        current_->first.insert(std::make_pair(it->first->symbol(), stack_type));
      }
    }
    {
      // main
      const Statements& stmts = func.body();
      for (Statements::const_iterator it = stmts.begin(),
           last = stmts.end(); it != last; ++it) {
        (*it)->Accept(this);
      }
    }
  }

  void Visit(const Block* block) {
    const Statements& stmts = block->body();
    for (Statements::const_iterator it = stmts.begin(),
         last = stmts.end(); it != last; ++it) {
      (*it)->Accept(this);
    }
  }

  void Visit(const FunctionStatement* func) { }
  void Visit(const FunctionDeclaration* func) { }
  void Visit(const VariableStatement* var) { }
  void Visit(const EmptyStatement* stmt) { }
  void Visit(const IfStatement* stmt) { }
  void Visit(const DoWhileStatement* stmt) { }
  void Visit(const WhileStatement* stmt) { }
  void Visit(const ForStatement* stmt) { }
  void Visit(const ForInStatement* stmt) { }
  void Visit(const ContinueStatement* stmt) { }
  void Visit(const BreakStatement* stmt) { }
  void Visit(const ReturnStatement* stmt) { }
  void Visit(const WithStatement* stmt) { }
  void Visit(const LabelledStatement* stmt) { }
  void Visit(const SwitchStatement* stmt) { }
  void Visit(const ThrowStatement* stmt) { }
  void Visit(const TryStatement* stmt) { }
  void Visit(const DebuggerStatement* stmt) { }
  void Visit(const ExpressionStatement* stmt) { }
  void Visit(const Assignment* assign) { }
  void Visit(const BinaryOperation* binary) { }
  void Visit(const ConditionalExpression* cond) { }
  void Visit(const UnaryOperation* unary) { }
  void Visit(const PostfixExpression* postfix) { }
  void Visit(const StringLiteral* literal) { }
  void Visit(const NumberLiteral* literal) { }
  void Visit(const Identifier* literal) { }
  void Visit(const ThisLiteral* literal) { }
  void Visit(const NullLiteral* lit) { }
  void Visit(const TrueLiteral* lit) { }
  void Visit(const FalseLiteral* lit) { }
  void Visit(const RegExpLiteral* literal) { }
  void Visit(const ArrayLiteral* literal) { }
  void Visit(const ObjectLiteral* literal) { }

  class EntrySwitcher : private core::Noncopyable<> {
   public:
    explicit EntrySwitcher(Analyzer* analyzer, const FunctionLiteral& func)
      : analyzer_(analyzer) {
      analyzer_->PushEntry(func);
    };
    ~EntrySwitcher() {
      analyzer_->PopEntry();
    }
   private:
    Analyzer* analyzer_;
  };

  void Visit(const FunctionLiteral* literal) {
    const EntrySwitcher switcher(this, *literal);
    VisitFunction(*literal, Variable::STACK);
  }

  void Visit(const IdentifierAccess* prop) { }
  void Visit(const IndexAccess* prop) { }
  void Visit(const FunctionCall* call) { }
  void Visit(const ConstructorCall* call) { }

  void Visit(const Declaration* dummy) { }
  void Visit(const CaseClause* dummy) { }

  void PushEntry(const FunctionLiteral& func) {
    current_ = MakeEntry(table_, func, current_);
  }

  void PopEntry() {
    current_ = current_->second;
  }

  std::tr1::shared_ptr<Table> table_;
  Entry* current_;
};


} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_ANALYZIER_H_
