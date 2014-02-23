#ifndef IV_PARSER_H_
#define IV_PARSER_H_
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <iv/detail/cstdint.h>
#include <iv/detail/cinttypes.h>
#include <iv/detail/unordered_map.h>
#include <iv/detail/unordered_set.h>
#include <iv/detail/array.h>
#include <iv/maybe.h>
#include <iv/ast.h>
#include <iv/ast_factory.h>
#include <iv/lexer.h>
#include <iv/dtoa.h>
#include <iv/noncopyable.h>
#include <iv/utils.h>
#include <iv/ustring.h>
#include <iv/none.h>
#include <iv/environment.h>
#include <iv/symbol_table.h>

#define IV_IS(token)\
  do {\
    if (token_ != token) {\
      *res = false;\
      ReportUnexpectedToken(token);\
      return nullptr;\
    }\
  } while (0)

#define IV_EXPECT(token)\
  do {\
    IV_IS(token);\
    Next();\
  } while (0)

#define IV_UNEXPECT_WITH(token, value)\
  do {\
    *res = false;\
    ReportUnexpectedToken(token);\
    return value;\
  } while (0)

#define IV_UNEXPECT(token) IV_UNEXPECT_WITH(token, nullptr)

#define IV_RAISE_IMPL(str, line, value)\
  do {\
    *res = false;\
    error_state_ |= kNotRecoverable;\
    SetErrorHeader(line);\
    error_.append(str);\
    return value;\
  } while (0)

#define IV_RAISE_WITH(str, v) IV_RAISE_IMPL(str, lexer_.line_number(), v)
#define IV_RAISE_NUMBER(str, line) IV_RAISE_IMPL(str, line, nullptr)
#define IV_RAISE(str) IV_RAISE_WITH(str, nullptr)

#define IV_RAISE_RECOVERABLE(str)\
  do {\
    *res = false;\
    SetErrorHeader(lexer_.line_number());\
    error_.append(str);\
    return nullptr;\
  } while (0)


#define IV_CHECK_WITH(value)  res);\
  if (!*res) {\
    return value;\
  }\
  ((void)0
#define IV_DUMMY )  // to make indentation work
#undef IV_DUMMY

#define IV_CHECK IV_CHECK_WITH(nullptr)

namespace iv {
namespace core {
namespace detail {

static const UString kUseStrict = ToUString("use strict");

}  // namespace iv::core::detail

template<typename Factory,
         typename Source,
         bool UseFunctionStatement = true,
         bool ReduceExpressions = true>
class Parser : private Noncopyable<> {
 public:
  typedef Parser<Factory, Source,
                 UseFunctionStatement, ReduceExpressions> this_type;
  typedef this_type parser_type;
  typedef Lexer<Source> lexer_type;
#define V(AST) typedef typename ast::AST<Factory> AST;
  IV_AST_NODE_LIST(V)
#undef V
#define V(XS) typedef typename ast::AstNode<Factory>::XS XS;
  IV_AST_LIST_LIST(V)
#undef V
#define V(S) typedef SpaceUString<Factory> S;
  IV_AST_STRING(V)
#undef V

  typedef std::unordered_set<Symbol> SymbolSet;

  enum ErrorState {
    kNotRecoverable = 1
  };

  class Target : private Noncopyable<> {
   public:
    enum Type {
      kNamedOnlyStatement = 0,  // (00)2
      kIterationStatement = 2,  // (10)2
      kSwitchStatement = 3      // (11)2
    };
    Target(parser_type* parser, Type type)
      : parser_(parser),
        prev_(parser->target()),
        labels_(parser->labels()),
        node_(nullptr),
        type_(type) {
      parser_->set_target(this);
      parser_->set_labels(nullptr);
    }
    ~Target() {
      parser_->set_target(prev_);
    }
    inline Target* previous() const {
      return prev_;
    }
    inline bool IsAnonymous() const {
      return type_ & 2;
    }
    inline bool IsIteration() const {
      return type_ == kIterationStatement;
    }
    inline bool IsSwitch() const {
      return type_ == kSwitchStatement;
    }
    inline bool IsNamedOnly() const {
      return !IsAnonymous();
    }
    inline BreakableStatement** node() {
      if (!node_) {
        node_ = parser_->factory()->template NewPtr<BreakableStatement>();
      }
      return node_;
    }
    inline SymbolSet* labels() const { return labels_; }
    inline void set_node(BreakableStatement* node) {
      if (node_) {
        *node_ = node;
      }
    }
   private:
    parser_type* parser_;
    Target* prev_;
    SymbolSet* labels_;
    BreakableStatement** node_;
    int type_;
  };

  class TargetSwitcher : private Noncopyable<> {
   public:
    explicit TargetSwitcher(parser_type* parser)
      : parser_(parser),
        target_(parser->target()),
        labels_(parser->labels()) {
      parser_->set_target(nullptr);
      parser_->set_labels(nullptr);
    }
    ~TargetSwitcher() {
      parser_->set_target(target_);
      parser_->set_labels(labels_);
    }
   private:
    parser_type* parser_;
    Target* target_;
    SymbolSet* labels_;
  };

  Parser(Factory* factory, const Source& source, SymbolTable* table)
    : lexer_(&source),
      error_(),
      strict_(false),
      reference_error_(false),
      error_state_(0),
      factory_(factory),
      scope_(nullptr),
      environment_(nullptr),
      target_(nullptr),
      labels_(nullptr),
      table_(table),
      last_parenthesized_(nullptr) {
  }

// Program
//   : SourceElements
  FunctionLiteral* ParseProgram() {
    Assigneds* params = factory_->template NewVector<Assigned*>();
    Statements* body = factory_->template NewVector<Statement*>();
    Scope* const scope = factory_->NewScope(FunctionLiteral::GLOBAL, params);
    FunctionEnvironment<Scope> environment(environment_, &environment_, scope);
    assert(target_ == nullptr);
    bool error_flag = true;
    bool *res = &error_flag;
    const ScopeSwitcher scope_switcher(this, scope);
    Next();
    const bool strict = ParseSourceElements(Token::TK_EOS, body, IV_CHECK);
    const std::size_t end_position = lexer_.end_position();
    scope->set_strict(strict);
    environment.Resolve(nullptr);
    return (error_flag) ?
        factory_->NewFunctionLiteral(FunctionLiteral::GLOBAL,
                                     nullptr,
                                     params,
                                     body,
                                     scope,
                                     strict,
                                     0,
                                     end_position,
                                     0,
                                     end_position,
                                     1) : nullptr;
  }

// SourceElements
//   : SourceElement
//   | SourceElement SourceElements
//
// SourceElement
//   : Statements
//   | FunctionDeclaration
  bool ParseSourceElements(Token::Type end, Statements* body, bool *res) {
    const StrictSwitcher strict_switcher(this);

    // directive prologue
    {
      bool octal_escaped_directive_found = false;
      std::size_t line = 0;
      while (token_ != end) {
        if (token_ != Token::TK_STRING) {
          // this is not directive
          break;
        }
        const typename lexer_type::State state = lexer_.StringEscapeType();
        if (!octal_escaped_directive_found && state == lexer_type::OCTAL) {
            // octal escaped string literal
            octal_escaped_directive_found = true;
            line = lexer_.line_number();
        }
        Statement* stmt = ParseStatement(IV_CHECK_WITH(false));
        body->push_back(stmt);
        if (stmt->AsExpressionStatement() &&
            stmt->AsExpressionStatement()->expr()->AsStringLiteral()) {
          // expression is directive
          StringLiteral* const literal =
              stmt->AsExpressionStatement()->expr()->AsStringLiteral();

          if (literal->IsDirective()) {
            if (!strict_switcher.IsStrict() &&
                state == lexer_type::NONE && IsStrictDirective(literal)) {
              strict_switcher.SwitchStrictMode();

              // if such a script is evaluated,
              // function target() {
              //   "octal \02";
              //   "use strict";
              // }
              // found octal string literal and raise error.
              if (octal_escaped_directive_found) {
                IV_RAISE_IMPL(
                    "octal escape sequence not allowed in strict code",
                    line,
                    false);
              }

              // and one token lexed is not in strict
              // so rescan
              if (token_ == Token::TK_IDENTIFIER) {
                token_ = Keyword<IdentifyReservedWords>::Detect(
                    lexer_.Buffer(), true);
                break;
              }
            } else {
              // other directive
            }
            continue;  // next directive
          }
        }
        // not directive
        break;
      }
    }

    // statements
    while (token_ != end) {
      Statement* stmt;
      if (token_ == Token::TK_FUNCTION) {
        // FunctionDeclaration
        stmt = ParseFunctionDeclaration(IV_CHECK_WITH(false));
      } else {
        stmt = ParseStatement(IV_CHECK_WITH(false));
      }
      body->push_back(stmt);
    }
    return strict_switcher.IsStrict();
  }

//  Statement
//    : Block
//    | FunctionStatement    // This is not standard.
//    | VariableStatement
//    | EmptyStatement
//    | ExpressionStatement
//    | IfStatement
//    | IterationStatement
//    | ContinueStatement
//    | BreakStatement
//    | ReturnStatement
//    | WithStatement
//    | LabelledStatement
//    | SwitchStatement
//    | ThrowStatement
//    | TryStatement
//    | DebuggerStatement
  Statement* ParseStatement(bool *res) {
    switch (token_) {
      case Token::TK_LBRACE:
        // Block
        return ParseBlock(IV_CHECK);

      case Token::TK_CONST:
        if (strict_) {
          IV_RAISE("\"const\" not allowed in strict code");
        }
      case Token::TK_VAR:
        // VariableStatement
        return ParseVariableStatement(IV_CHECK);

      case Token::TK_SEMICOLON:
        // EmptyStatement
        return ParseEmptyStatement();

      case Token::TK_IF:
        // IfStatement
        return ParseIfStatement(IV_CHECK);

      case Token::TK_DO:
        // IterationStatement
        // do while
        return ParseDoWhileStatement(IV_CHECK);

      case Token::TK_WHILE:
        // IterationStatement
        // while
        return ParseWhileStatement(IV_CHECK);

      case Token::TK_FOR:
        // IterationStatement
        // for
        return ParseForStatement(IV_CHECK);

      case Token::TK_CONTINUE:
        // ContinueStatement
        return ParseContinueStatement(IV_CHECK);

      case Token::TK_BREAK:
        // BreakStatement
        return ParseBreakStatement(IV_CHECK);

      case Token::TK_RETURN:
        // ReturnStatement
        return ParseReturnStatement(IV_CHECK);

      case Token::TK_WITH:
        // WithStatement
        return ParseWithStatement(IV_CHECK);

      case Token::TK_SWITCH:
        // SwitchStatement
        return ParseSwitchStatement(IV_CHECK);

      case Token::TK_THROW:
        // ThrowStatement
        return ParseThrowStatement(IV_CHECK);

      case Token::TK_TRY:
        // TryStatement
        return ParseTryStatement(IV_CHECK);

      case Token::TK_DEBUGGER:
        // DebuggerStatement
        return ParseDebuggerStatement(IV_CHECK);

      case Token::TK_FUNCTION:
        // FunctionStatement (not in ECMA-262 5th)
        // FunctionExpression
        return ParseFunctionStatement<UseFunctionStatement>(IV_CHECK);

      case Token::TK_IDENTIFIER:
        // LabelledStatement or ExpressionStatement
        return ParseExpressionOrLabelledStatement(IV_CHECK);

      case Token::TK_ILLEGAL:
        IV_UNEXPECT(token_);
        break;

      default:
        // ExpressionStatement or ILLEGAL
        return ParseExpressionStatement(IV_CHECK);
    }
  }

//  FunctionDeclaration
//    : FUNCTION IDENTIFIER '(' FormalParameterList_opt ')' '{' FunctionBody '}'
//
//  FunctionStatement
//    : FUNCTION IDENTIFIER '(' FormalParameterList_opt ')' '{' FunctionBody '}'
//
//  FunctionExpression
//    : FUNCTION
//      IDENTIFIER_opt '(' FormalParameterList_opt ')' '{' FunctionBody '}'
//
//  FunctionStatement is not standard, but implemented in SpiderMonkey
//  and this statement is very useful for not breaking FunctionDeclaration.
  Statement* ParseFunctionDeclaration(bool *res) {
    assert(token_ == Token::TK_FUNCTION);
    FunctionLiteral* const expr = ParseFunctionLiteral(
        FunctionLiteral::DECLARATION,
        FunctionLiteral::GENERAL, IV_CHECK);
    // define named function as FunctionDeclaration
    scope_->AddFunctionDeclaration(expr);
    assert(expr);
    return factory_->NewFunctionDeclaration(expr);
  }

//  Block
//    : '{' '}'
//    | '{' StatementList '}'
//
//  StatementList
//    : Statement
//    | StatementList Statement
  Block* ParseBlock(bool *res) {
    assert(token_ == Token::TK_LBRACE);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Statements* const body = factory_->template NewVector<Statement*>();
    Target target(this, Target::kNamedOnlyStatement);

    Next();
    while (token_ != Token::TK_RBRACE) {
      Statement* const stmt = ParseStatement(IV_CHECK);
      body->push_back(stmt);
    }
    Next();
    assert(body);
    Block* const block = factory_->NewBlock(body,
                                            begin,
                                            lexer_.previous_end_position(),
                                            line_number);
    target.set_node(block);
    return block;
  }

//  VariableStatement
//    : VAR VariableDeclarationList ';'
//    : CONST VariableDeclarationList ';'
  Statement* ParseVariableStatement(bool *res) {
    assert(token_ == Token::TK_VAR || token_ == Token::TK_CONST);
    const Token::Type op = token_;
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Declarations* const decls = factory_->template NewVector<Declaration*>();
    ParseVariableDeclarations(decls, token_ == Token::TK_CONST, true, IV_CHECK);
    ExpectSemicolon(IV_CHECK);
    assert(decls);
    return factory_->NewVariableStatement(op,
                                          decls,
                                          begin,
                                          lexer_.previous_end_position(),
                                          line_number);
  }

//  VariableDeclarationList
//    : VariableDeclaration
//    | VariableDeclarationList ',' VariableDeclaration
//
//  VariableDeclaration
//    : IDENTIFIER Initialiser_opt
//
//  Initialiser_opt
//    :
//    | Initialiser
//
//  Initialiser
//    : '=' AssignmentExpression
  Declarations* ParseVariableDeclarations(Declarations* decls,
                                          bool is_const,
                                          bool contains_in,
                                          bool *res) {
    do {
      Next();
      IV_IS(Token::TK_IDENTIFIER);
      const ast::SymbolHolder sym = ParseSymbol();
      // section 12.2.1
      // within the strict code, Identifier must not be "eval" or "arguments"
      if (strict_) {
        if (sym == symbol::eval()) {
          IV_RAISE("assignment to \"eval\" not allowed in strict code");
        } else if (sym == symbol::arguments()) {
          IV_RAISE("assignment to \"arguments\" not allowed in strict code");
        }
      }

      Assigned* name = factory_->NewAssigned(sym, false);
      Declaration* decl;
      if (token_ == Token::TK_ASSIGN) {
        Next();
        // AssignmentExpression
        Expression* expr = ParseAssignmentExpression(contains_in, IV_CHECK);
        decl = factory_->NewDeclaration(name, expr);
      } else {
        // Undefined Expression
        decl = factory_->NewDeclaration(name, nullptr);
      }
      decls->push_back(decl);
      scope_->AddUnresolved(name, is_const);
    } while (token_ == Token::TK_COMMA);

    return decls;
  }

//  EmptyStatement
//    : ';'
  Statement* ParseEmptyStatement() {
    assert(token_ == Token::TK_SEMICOLON);
    Next();
    return factory_->NewEmptyStatement(lexer_.previous_begin_position(),
                                       lexer_.previous_end_position(),
                                       lexer_.previous_line_number());
  }

//  IfStatement
//    : IF '(' Expression ')' Statement ELSE Statement
//    | IF '(' Expression ')' Statement
  Statement* ParseIfStatement(bool *res) {
    assert(token_ == Token::TK_IF);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Statement* else_statement = nullptr;
    Next();

    IV_EXPECT(Token::TK_LPAREN);

    Expression* const expr = ParseExpression(true, IV_CHECK);

    IV_EXPECT(Token::TK_RPAREN);

    Statement* const then_statement = ParseStatement(IV_CHECK);
    if (token_ == Token::TK_ELSE) {
      Next();
      else_statement = ParseStatement(IV_CHECK);
    }
    assert(expr && then_statement);
    return factory_->NewIfStatement(expr,
                                    then_statement,
                                    else_statement,
                                    begin,
                                    line_number);
  }

//  IterationStatement
//    : DO Statement WHILE '(' Expression ')' ';'
//    | WHILE '(' Expression ')' Statement
//    | FOR '(' ExpressionNoIn_opt ';' Expression_opt ';' Expression_opt ')'
//      Statement
//    | FOR '(' VAR VariableDeclarationListNoIn ';'
//              Expression_opt ';'
//              Expression_opt ')'
//              Statement
//    | FOR '(' LeftHandSideExpression IN Expression ')' Statement
//    | FOR '(' VAR VariableDeclarationNoIn IN Expression ')' Statement
  Statement* ParseDoWhileStatement(bool *res) {
    //  DO Statement WHILE '(' Expression ')' ';'
    assert(token_ == Token::TK_DO);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Target target(this, Target::kIterationStatement);
    Next();

    Statement* const stmt = ParseStatement(IV_CHECK);

    IV_EXPECT(Token::TK_WHILE);

    IV_EXPECT(Token::TK_LPAREN);

    Expression* const expr = ParseExpression(true, IV_CHECK);

    IV_EXPECT(Token::TK_RPAREN);

    // ex:
    //   do {
    //     print("valid syntax");
    //   } while (0) return true;
    // is syntax valid
    if (token_ == Token::TK_SEMICOLON) {
      Next();
    }
    assert(stmt && expr);
    DoWhileStatement* const dowhile =
        factory_->NewDoWhileStatement(
            stmt,
            expr,
            begin,
            lexer_.previous_end_position(),
            line_number);
    target.set_node(dowhile);
    return dowhile;
  }

//  WHILE '(' Expression ')' Statement
  Statement* ParseWhileStatement(bool *res) {
    assert(token_ == Token::TK_WHILE);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Next();

    IV_EXPECT(Token::TK_LPAREN);

    Expression* const expr = ParseExpression(true, IV_CHECK);
    Target target(this, Target::kIterationStatement);

    IV_EXPECT(Token::TK_RPAREN);

    Statement* const stmt = ParseStatement(IV_CHECK);
    assert(stmt);
    WhileStatement* const whilestmt =
        factory_->NewWhileStatement(stmt, expr, begin, line_number);

    target.set_node(whilestmt);
    return whilestmt;
  }

//  FOR '(' ExpressionNoIn_opt ';' Expression_opt ';' Expression_opt ')'
//  Statement
//  FOR '(' VAR VariableDeclarationListNoIn ';'
//          Expression_opt ';'
//          Expression_opt ')'
//          Statement
//  FOR '(' LeftHandSideExpression IN Expression ')' Statement
//  FOR '(' VAR VariableDeclarationNoIn IN Expression ')' Statement
  Statement* ParseForStatement(bool *res) {
    assert(token_ == Token::TK_FOR);
    const std::size_t for_stmt_begin = lexer_.begin_position();
    const std::size_t for_stmt_line_number = lexer_.line_number();
    Next();

    IV_EXPECT(Token::TK_LPAREN);

    Statement *init = nullptr;

    if (token_ != Token::TK_SEMICOLON) {
      if (token_ == Token::TK_VAR || token_ == Token::TK_CONST) {
        const std::size_t begin = lexer_.begin_position();
        const std::size_t line_number = lexer_.line_number();
        const Token::Type op = token_;
        Declarations* const decls =
            factory_->template NewVector<Declaration*>();
        ParseVariableDeclarations(decls,
                                  token_ == Token::TK_CONST, false, IV_CHECK);
        if (token_ == Token::TK_IN) {
          assert(decls);
          VariableStatement* const var =
              factory_->NewVariableStatement(op,
                                             decls,
                                             begin,
                                             lexer_.previous_end_position(),
                                             line_number);
          init = var;
          // for in loop
          Next();
          const Declarations& decls = var->decls();
          if (decls.size() != 1) {
            // ForInStatement requests VaraibleDeclarationNoIn (not List),
            // so check declarations' size is 1.
            IV_RAISE("invalid for-in left-hand-side");
          }
          Expression* const enumerable = ParseExpression(true, IV_CHECK);
          IV_EXPECT(Token::TK_RPAREN);
          Target target(this, Target::kIterationStatement);
          Statement* const body = ParseStatement(IV_CHECK);
          assert(body && init && enumerable);
          ForInStatement* const forstmt =
              factory_->NewForInStatement(body,
                                          init,
                                          enumerable,
                                          for_stmt_begin,
                                          for_stmt_line_number);
          target.set_node(forstmt);
          return forstmt;
        } else {
          assert(decls);
          init = factory_->NewVariableStatement(op, decls,
                                                begin,
                                                lexer_.end_position(),
                                                line_number);
        }
      } else {
        Expression* const init_expr = ParseExpression(false, IV_CHECK);
        if (token_ == Token::TK_IN) {
          // for in loop
          assert(init_expr);
          init = factory_->NewExpressionStatement(
              init_expr, lexer_.previous_end_position());
          // LHS Guard
          if (!init_expr->IsLeftHandSide()) {
            reference_error_ = true;
            IV_RAISE("invalid for-in left-hand-side");
          }
          Next();
          Expression* const enumerable = ParseExpression(true, IV_CHECK);
          IV_EXPECT(Token::TK_RPAREN);
          Target target(this, Target::kIterationStatement);
          Statement* const body = ParseStatement(IV_CHECK);
          assert(body && init && enumerable);
          ForInStatement* const forstmt =
              factory_->NewForInStatement(body, init, enumerable,
                                          for_stmt_begin, for_stmt_line_number);
          target.set_node(forstmt);
          return forstmt;
        } else {
          assert(init_expr);
          init = factory_->NewExpressionStatement(init_expr,
                                                  lexer_.end_position());
        }
      }
    }

    // ordinary for loop
    IV_EXPECT(Token::TK_SEMICOLON);

    Expression* cond = nullptr;
    if (token_ == Token::TK_SEMICOLON) {
      // no cond expr
      Next();
    } else {
      cond = ParseExpression(true, IV_CHECK);
      IV_EXPECT(Token::TK_SEMICOLON);
    }

    Expression* next = nullptr;
    if (token_ == Token::TK_RPAREN) {
      Next();
    } else {
      next = ParseExpression(true, IV_CHECK);
      assert(next);
      IV_EXPECT(Token::TK_RPAREN);
    }

    Target target(this, Target::kIterationStatement);
    Statement* const body = ParseStatement(IV_CHECK);
    assert(body);
    ForStatement* const forstmt = factory_->NewForStatement(
        body,
        init,
        cond,
        next, for_stmt_begin, for_stmt_line_number);
    target.set_node(forstmt);
    return forstmt;
  }

//  ContinueStatement
//    : CONTINUE Identifier_opt ';'
  Statement* ParseContinueStatement(bool *res) {
    assert(token_ == Token::TK_CONTINUE);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    ast::SymbolHolder label;
    IterationStatement** target;
    Next();
    if (token_ == Token::TK_IDENTIFIER &&
        !lexer_.has_line_terminator_before_next()) {
      label = ParseSymbol();
      target = LookupContinuableTarget(label);
      if (!target) {
        IV_RAISE("label not found");
      }
    } else {
      target = LookupContinuableTarget();
      if (!target) {
        IV_RAISE("label not found");
      }
    }
    ExpectSemicolon(IV_CHECK);
    return factory_->NewContinueStatement(label, target,
                                          begin,
                                          lexer_.previous_end_position(),
                                          line_number);
  }

//  BreakStatement
//    : BREAK Identifier_opt ';'
  Statement* ParseBreakStatement(bool *res) {
    assert(token_ == Token::TK_BREAK);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    ast::SymbolHolder label;
    BreakableStatement** target = nullptr;
    Next();
    if (token_ == Token::TK_IDENTIFIER &&
        !lexer_.has_line_terminator_before_next()) {
      // label
      IV_IS(Token::TK_IDENTIFIER);
      label = ParseSymbol();
      if (ContainsLabel(labels_, label)) {
        // example
        //
        //   do {
        //     test: break test;
        //   } while (0);
        //
        // This BreakStatement is interpreted as EmptyStatement
        // In iv, BreakStatement with label, but without target is
        // interpreted as EmptyStatement
      } else {
        target = LookupBreakableTarget(label);
        if (!target) {
          IV_RAISE("label not found");
        }
      }
    } else {
      target = LookupBreakableTarget();
      if (!target) {
        IV_RAISE("label not found");
      }
    }
    ExpectSemicolon(IV_CHECK);
    return factory_->NewBreakStatement(label,
                                       target,
                                       begin,
                                       lexer_.previous_end_position(),
                                       line_number);
  }

//  ReturnStatement
//    : RETURN Expression_opt ';'
  Statement* ParseReturnStatement(bool *res) {
    assert(token_ == Token::TK_RETURN);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Next();

    if (scope_->IsGlobal()) {
      // return statement found in global
      // SyntaxError
      IV_RAISE("\"return\" not in function");
    }

    Expression* expr = nullptr;
    if (!IsAutomaticSemicolonInserted()) {
      expr = ParseExpression(true, IV_CHECK);
    }
    ExpectSemicolon(IV_CHECK);
    return factory_->NewReturnStatement(expr,
                                        begin,
                                        lexer_.previous_end_position(),
                                        line_number);
  }

//  WithStatement
//    : WITH '(' Expression ')' Statement
  Statement* ParseWithStatement(bool *res) {
    assert(token_ == Token::TK_WITH);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Next();

    // section 12.10.1
    // when in strict mode code, WithStatement is not allowed.
    if (strict_) {
      IV_RAISE("with statement not allowed in strict code");
    }

    IV_EXPECT(Token::TK_LPAREN);

    Expression* expr = ParseExpression(true, IV_CHECK);

    IV_EXPECT(Token::TK_RPAREN);

    Statement* stmt = nullptr;
    {
      WithEnvironment environment(environment_, &environment_);
      stmt = ParseStatement(IV_CHECK);
    }
    assert(expr && stmt);
    return factory_->NewWithStatement(expr, stmt, begin, line_number);
  }

//  SwitchStatement
//    : SWITCH '(' Expression ')' CaseBlock
//
//  CaseBlock
//    : '{' CaseClauses_opt '}'
//    | '{' CaseClauses_opt DefaultClause CaseClauses_opt '}'
  Statement* ParseSwitchStatement(bool *res) {
    assert(token_ == Token::TK_SWITCH);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    CaseClause* case_clause;
    Next();

    IV_EXPECT(Token::TK_LPAREN);

    Expression* expr = ParseExpression(true, IV_CHECK);
    CaseClauses* clauses = factory_->template NewVector<CaseClause*>();
    Target target(this, Target::kSwitchStatement);

    IV_EXPECT(Token::TK_RPAREN);

    IV_EXPECT(Token::TK_LBRACE);

    bool default_found = false;
    while (token_ != Token::TK_RBRACE) {
      if (token_ == Token::TK_CASE ||
          token_ == Token::TK_DEFAULT) {
        case_clause = ParseCaseClause(IV_CHECK);
      } else {
        IV_UNEXPECT(token_);
      }
      if (case_clause->IsDefault()) {
        if (default_found) {
          IV_RAISE("duplicate default clause in switch");
        } else {
          default_found = true;
        }
      }
      clauses->push_back(case_clause);
    }
    Next();
    assert(expr && clauses);
    SwitchStatement* const switch_stmt =
        factory_->NewSwitchStatement(expr,
                                     clauses,
                                     begin,
                                     lexer_.previous_end_position(),
                                     line_number);
    target.set_node(switch_stmt);
    return switch_stmt;
  }

//  CaseClauses
//    : CaseClause
//    | CaseClauses CaseClause
//
//  CaseClause
//    : CASE Expression ':' StatementList_opt
//
//  DefaultClause
//    : DEFAULT ':' StatementList_opt
  CaseClause* ParseCaseClause(bool *res) {
    assert(token_ == Token::TK_CASE || token_ == Token::TK_DEFAULT);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Expression* expr = nullptr;
    Statements* const body = factory_->template NewVector<Statement*>();

    if (token_ == Token::TK_CASE) {
      Next();
      expr = ParseExpression(true, IV_CHECK);
    } else  {
      IV_EXPECT(Token::TK_DEFAULT);
    }

    IV_EXPECT(Token::TK_COLON);

    while (token_ != Token::TK_RBRACE &&
           token_ != Token::TK_CASE   &&
           token_ != Token::TK_DEFAULT) {
      Statement* const stmt = ParseStatement(IV_CHECK);
      body->push_back(stmt);
    }

    assert(body);
    return factory_->NewCaseClause(expr == nullptr,
                                   expr,
                                   body,
                                   begin,
                                   lexer_.previous_end_position(),
                                   line_number);
  }

//  ThrowStatement
//    : THROW Expression ';'
  Statement* ParseThrowStatement(bool *res) {
    assert(token_ == Token::TK_THROW);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Next();
    // Throw requires Expression and no LineTerminator is allowed between them.
    if (lexer_.has_line_terminator_before_next()) {
      IV_RAISE("missing expression between throw and newline");
    }
    Expression* const expr = ParseExpression(true, IV_CHECK);
    ExpectSemicolon(IV_CHECK);
    assert(expr);
    return factory_->NewThrowStatement(
        expr,
        begin,
        lexer_.previous_end_position(),
        line_number);
  }

// TryStatement
//    : TRY Block Catch
//    | TRY Block Finally
//    | TRY Block Catch Finally
//
//  Catch
//    : CATCH '(' IDENTIFIER ')' Block
//
//  Finally
//    : FINALLY Block
  Statement* ParseTryStatement(bool *res) {
    assert(token_ == Token::TK_TRY);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Block* catch_block = nullptr;
    Block* finally_block = nullptr;
    Assigned* name = nullptr;
    bool has_catch_or_finally = false;

    Next();

    IV_IS(Token::TK_LBRACE);
    Block* const try_block = ParseBlock(IV_CHECK);

    if (token_ == Token::TK_CATCH) {
      // Catch
      has_catch_or_finally = true;
      Next();
      IV_EXPECT(Token::TK_LPAREN);
      IV_IS(Token::TK_IDENTIFIER);
      const ast::SymbolHolder sym = ParseSymbol();
      // section 12.14.1
      // within the strict code, Identifier must not be "eval" or "arguments"
      if (strict_) {
        if (sym == symbol::eval()) {
          IV_RAISE(
              "catch placeholder \"eval\" not allowed in strict code");
        } else if (sym == symbol::arguments()) {
          IV_RAISE(
              "catch placeholder \"arguments\" not allowed in strict code");
        }
      }
      IV_EXPECT(Token::TK_RPAREN);
      IV_IS(Token::TK_LBRACE);
      name = factory_->NewAssigned(sym, false);
      {
        CatchEnvironment environment(environment_, &environment_, sym);
        catch_block = ParseBlock(IV_CHECK);
      }
    }

    if (token_ == Token::TK_FINALLY) {
      // Finally
      has_catch_or_finally= true;
      Next();
      IV_IS(Token::TK_LBRACE);
      finally_block = ParseBlock(IV_CHECK);
    }

    if (!has_catch_or_finally) {
      IV_RAISE_RECOVERABLE("missing catch or finally after try statement");
    }

    assert(try_block);
    return factory_->NewTryStatement(try_block,
                                     name,
                                     catch_block,
                                     finally_block,
                                     begin,
                                     line_number);
  }

//  DebuggerStatement
//    : DEBUGGER ';'
  Statement* ParseDebuggerStatement(bool *res) {
    assert(token_ == Token::TK_DEBUGGER);
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Next();
    ExpectSemicolon(IV_CHECK);
    return factory_->NewDebuggerStatement(begin,
                                          lexer_.previous_end_position(),
                                          line_number);
  }

  Statement* ParseExpressionStatement(bool *res) {
    Expression* const expr = ParseExpression(true, IV_CHECK);
    ExpectSemicolon(IV_CHECK);
    assert(expr);
    return factory_->NewExpressionStatement(expr,
                                            lexer_.previous_end_position());
  }

//  LabelledStatement
//    : IDENTIFIER ':' Statement
//
//  ExpressionStatement
//    : Expression ';'
  Statement* ParseExpressionOrLabelledStatement(bool *res) {
    assert(token_ == Token::TK_IDENTIFIER);
    if (lexer_.NextIsColon()) {
      // LabelledStatement
      const ast::SymbolHolder label = ParseSymbol();
      assert(token_ == Token::TK_COLON);
      Next();
      if (IsDuplicateLabel(label)) {
        // duplicate label
        IV_RAISE("duplicate label");
      }
      const LabelSwitcher label_switcher(this, labels_, label);

      Statement* const stmt = ParseStatement(IV_CHECK);
      assert(stmt);
      return factory_->NewLabelledStatement(label, stmt);
    }
    Expression* const expr = ParseExpression(true, IV_CHECK);
    ExpectSemicolon(IV_CHECK);
    assert(expr);
    return factory_->NewExpressionStatement(expr,
                                            lexer_.previous_end_position());
  }

  template<bool Use>
  Statement* ParseFunctionStatement(bool *res,
                                    typename std::enable_if<Use>::type* = 0) {
    assert(token_ == Token::TK_FUNCTION);
    if (strict_) {
      IV_RAISE("function statement not allowed in strict code");
    }
    FunctionLiteral* const expr = ParseFunctionLiteral(
        FunctionLiteral::STATEMENT,
        FunctionLiteral::GENERAL,
        IV_CHECK);
    // define named function as variable declaration
    assert(expr);
    assert(expr->name());
    Assigned* assigned = expr->name().Address();
    Assigned* target = factory_->NewAssigned(
        ast::SymbolHolder(assigned->symbol(),
                          assigned->begin_position(),
                          assigned->end_position(),
                          assigned->line_number()), false);
    scope_->AddUnresolved(target, false);
    return factory_->NewFunctionStatement(expr);
  }

  template<bool Use>
  Statement* ParseFunctionStatement(bool *res,
                                    typename std::enable_if<!Use>::type* = 0) {
    // FunctionStatement is not Standard
    // so, if template parameter UseFunctionStatement is false,
    // this parser reject FunctionStatement
    IV_RAISE("function statement is not Standard");
  }

//  Expression
//    : AssignmentExpression
//    | Expression ',' AssignmentExpression
  Expression* ParseExpression(bool contains_in, bool *res) {
    Expression* right;
    Expression* result = ParseAssignmentExpression(contains_in, IV_CHECK);
    while (token_ == Token::TK_COMMA) {
      Next();
      right = ParseAssignmentExpression(contains_in, IV_CHECK);
      assert(result && right);
      result = factory_->NewBinaryOperation(Token::TK_COMMA, result, right);
    }
    return result;
  }

//  AssignmentExpression
//    : ConditionalExpression
//    | LeftHandSideExpression AssignmentOperator AssignmentExpression
  Expression* ParseAssignmentExpression(bool contains_in, bool *res) {
    Expression* const result =
        ParseConditionalExpression(contains_in, IV_CHECK);
    if (!Token::IsAssignOp(token_)) {
      return result;
    }

    // LHS Guard
    if (!result->IsLeftHandSide()) {
      reference_error_ = true;
      IV_RAISE("assign to invalid left-hand-side");
    }

    // section 11.13.1 throwing SyntaxError
    if (strict_ && result->AsIdentifier()) {
      const Symbol sym = result->AsIdentifier()->symbol();
      if (sym == symbol::eval()) {
        IV_RAISE("assignment to \"eval\" not allowed in strict code");
      } else if (sym == symbol::arguments()) {
        IV_RAISE("assignment to \"arguments\" not allowed in strict code");
      }
    }
    const Token::Type op = token_;
    Next();
    Expression* const right = ParseAssignmentExpression(contains_in, IV_CHECK);
    assert(result && right);
    return factory_->NewAssignment(op, result, right);
  }

//  ConditionalExpression
//    : LogicalOrExpression
//    | LogicalOrExpression '?' AssignmentExpression ':' AssignmentExpression
  Expression* ParseConditionalExpression(bool contains_in, bool *res) {
    Expression* result = ParseBinaryExpression(contains_in, 9, IV_CHECK);
    if (token_ == Token::TK_CONDITIONAL) {
      Next();
      // see ECMA-262 section 11.12
      Expression* const left =
          ParseAssignmentExpression(true, IV_CHECK);
      IV_EXPECT(Token::TK_COLON);
      Expression* const right =
          ParseAssignmentExpression(contains_in, IV_CHECK);
      assert(result && left && right);
      result = factory_->NewConditionalExpression(result, left, right);
    }
    return result;
  }

//  LogicalOrExpression
//    : LogicalAndExpression
//    | LogicalOrExpression LOGICAL_OR LogicalAndExpression
//
//  LogicalAndExpression
//    : BitwiseOrExpression
//    | LogicalAndExpression LOGICAL_AND BitwiseOrExpression
//
//  BitwiseOrExpression
//    : BitwiseXorExpression
//    | BitwiseOrExpression '|' BitwiseXorExpression
//
//  BitwiseXorExpression
//    : BitwiseAndExpression
//    | BitwiseXorExpression '^' BitwiseAndExpression
//
//  BitwiseAndExpression
//    : EqualityExpression
//    | BitwiseAndExpression '&' EqualityExpression
//
//  EqualityExpression
//    : RelationalExpression
//    | EqualityExpression EQ_STRICT RelationalExpression
//    | EqualityExpression NE_STRICT RelationalExpression
//    | EqualityExpression EQ RelationalExpression
//    | EqualityExpression NE RelationalExpression
//
//  RelationalExpression
//    : ShiftExpression
//    | RelationalExpression LT ShiftExpression
//    | RelationalExpression GT ShiftExpression
//    | RelationalExpression LTE ShiftExpression
//    | RelationalExpression GTE ShiftExpression
//    | RelationalExpression INSTANCEOF ShiftExpression
//    | RelationalExpression IN ShiftExpression
//
//  ShiftExpression
//    : AdditiveExpression
//    | ShiftExpression SHL AdditiveExpression
//    | ShiftExpression SAR AdditiveExpression
//    | ShiftExpression SHR AdditiveExpression
//
//  AdditiveExpression
//    : MultiplicativeExpression
//    | AdditiveExpression ADD MultiplicativeExpression
//    | AdditiveExpression SUB MultiplicativeExpression
//
//  MultiplicativeExpression
//    : UnaryExpression
//    | MultiplicativeExpression MUL UnaryExpression
//    | MultiplicativeExpression DIV UnaryExpression
//    | MultiplicativeExpression MOD UnaryExpression
  Expression* ParseBinaryExpression(bool contains_in, int prec, bool *res) {
    Expression *left, *right;
    Token::Type op;
    left = ParseUnaryExpression(IV_CHECK);
    // MultiplicativeExpression
    while (token_ == Token::TK_MUL ||
           token_ == Token::TK_DIV ||
           token_ == Token::TK_MOD) {
      op = token_;
      Next();
      right = ParseUnaryExpression(IV_CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 1) return left;

    // AdditiveExpression
    while (token_ == Token::TK_ADD ||
           token_ == Token::TK_SUB) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 0, IV_CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 2) return left;

    // ShiftExpression
    while (token_ == Token::TK_SHL ||
           token_ == Token::TK_SAR ||
           token_ == Token::TK_SHR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 1, IV_CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 3) return left;

    // RelationalExpression
    while ((Token::TK_REL_FIRST < token_ &&
            token_ < Token::TK_REL_LAST) ||
           (contains_in && token_ == Token::TK_IN)) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 2, IV_CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    if (prec < 4) return left;

    // EqualityExpression
    while (token_ == Token::TK_EQ_STRICT ||
           token_ == Token::TK_NE_STRICT ||
           token_ == Token::TK_EQ ||
           token_ == Token::TK_NE) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 3, IV_CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    if (prec < 5) return left;

    // BitwiseAndExpression
    while (token_ == Token::TK_BIT_AND) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 4, IV_CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 6) return left;

    // BitwiseXorExpression
    while (token_ == Token::TK_BIT_XOR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 5, IV_CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 7) return left;

    // BitwiseOrExpression
    while (token_ == Token::TK_BIT_OR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 6, IV_CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 8) return left;

    // LogicalAndExpression
    while (token_ == Token::TK_LOGICAL_AND) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 7, IV_CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    if (prec < 9) return left;

    // LogicalOrExpression
    while (token_ == Token::TK_LOGICAL_OR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 8, IV_CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    return left;
  }

  template<bool Reduce>
  Expression* ReduceBinaryOperation(
      Token::Type op,
      Expression* left,
      Expression* right,
      typename std::enable_if<Reduce>::type* = 0) {
    assert(left && right);
    if (left->AsNumberLiteral() && right->AsNumberLiteral()) {
      const double l_val = left->AsNumberLiteral()->value();
      const double r_val = right->AsNumberLiteral()->value();
      Expression* res;
      switch (op) {
        case Token::TK_ADD:
          res = factory_->NewReducedNumberLiteral(l_val + r_val, left, right);
          break;

        case Token::TK_SUB:
          res = factory_->NewReducedNumberLiteral(l_val - r_val, left, right);
          break;

        case Token::TK_MUL:
          res = factory_->NewReducedNumberLiteral(l_val * r_val, left, right);
          break;

        case Token::TK_DIV:
          res = factory_->NewReducedNumberLiteral(l_val / r_val, left, right);
          break;

        case Token::TK_BIT_OR:
          res = factory_->NewReducedNumberLiteral(
              DoubleToInt32(l_val) | DoubleToInt32(r_val),
              left, right);
          break;

        case Token::TK_BIT_AND:
          res = factory_->NewReducedNumberLiteral(
              DoubleToInt32(l_val) & DoubleToInt32(r_val),
              left, right);
          break;

        case Token::TK_BIT_XOR:
          res = factory_->NewReducedNumberLiteral(
              DoubleToInt32(l_val) ^ DoubleToInt32(r_val),
              left, right);
          break;

        // section 11.7 Bitwise Shift Operators
        case Token::TK_SHL: {
          const int32_t value = DoubleToInt32(l_val)
              << (DoubleToInt32(r_val) & 0x1f);
          res = factory_->NewReducedNumberLiteral(value, left, right);
          break;
        }

        case Token::TK_SHR: {
          const uint32_t shift = DoubleToInt32(r_val) & 0x1f;
          const uint32_t value = DoubleToUInt32(l_val) >> shift;
          res = factory_->NewReducedNumberLiteral(value, left, right);
          break;
        }

        case Token::TK_SAR: {
          uint32_t shift = DoubleToInt32(r_val) & 0x1f;
          int32_t value = DoubleToInt32(l_val) >> shift;
          res = factory_->NewReducedNumberLiteral(value, left, right);
          break;
        }

        default:
          assert(left && right);
          res = factory_->NewBinaryOperation(op, left, right);
          break;
      }
      return res;
    } else if (op == Token::TK_ADD &&
               left->AsStringLiteral() && right->AsStringLiteral()) {
      return factory_->NewReducedStringLiteral(left->AsStringLiteral(),
                                               right->AsStringLiteral());
    } else {
      return factory_->NewBinaryOperation(op, left, right);
    }
  }

  template<bool Reduce>
  Expression* ReduceBinaryOperation(
      Token::Type op,
      Expression* left,
      Expression* right,
      typename std::enable_if<!Reduce>::type* = 0) {
    assert(left && right);
    return factory_->NewBinaryOperation(op, left, right);
  }

//  UnaryExpression
//    : PostfixExpression
//    | DELETE UnaryExpression
//    | VOID UnaryExpression
//    | TYPEOF UnaryExpression
//    | INC UnaryExpression
//    | DEC UnaryExpression
//    | '+' UnaryExpression
//    | '-' UnaryExpression
//    | '~' UnaryExpression
//    | '!' UnaryExpression
  Expression* ParseUnaryExpression(bool *res) {
    Expression *result, *expr;
    const Token::Type op = token_;
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    switch (token_) {
      case Token::TK_VOID:
      case Token::TK_NOT:
      case Token::TK_TYPEOF:
        Next();
        expr = ParseUnaryExpression(IV_CHECK);
        assert(expr);
        result = factory_->NewUnaryOperation(op, expr, begin, line_number);
        break;

      case Token::TK_DELETE:
        // a strict mode restriction in sec 11.4.1
        // raise SyntaxError when target is direct reference to a variable,
        // function argument, or function name
        Next();
        expr = ParseUnaryExpression(IV_CHECK);
        if (strict_ && expr->AsIdentifier()) {
          IV_RAISE("delete to direct identifier not allowed in strict code");
        }
        assert(expr);
        result = factory_->NewUnaryOperation(op, expr, begin, line_number);
        break;

      case Token::TK_BIT_NOT:
        Next();
        expr = ParseUnaryExpression(IV_CHECK);
        if (ReduceExpressions && expr->AsNumberLiteral()) {
          result = factory_->NewReducedNumberLiteral(
              ~DoubleToInt32(expr->AsNumberLiteral()->value()),
              expr,
              expr);
        } else {
          assert(expr);
          result = factory_->NewUnaryOperation(op, expr, begin, line_number);
        }
        break;

      case Token::TK_ADD:
        Next();
        expr = ParseUnaryExpression(IV_CHECK);
        if (expr->AsNumberLiteral()) {
          result = expr;
        } else {
          assert(expr);
          result = factory_->NewUnaryOperation(op, expr, begin, line_number);
        }
        break;

      case Token::TK_SUB:
        Next();
        expr = ParseUnaryExpression(IV_CHECK);
        if (ReduceExpressions && expr->AsNumberLiteral()) {
          result = factory_->NewReducedNumberLiteral(
              -(expr->AsNumberLiteral()->value()), expr, expr);
        } else {
          assert(expr);
          result = factory_->NewUnaryOperation(op, expr, begin, line_number);
        }
        break;

      case Token::TK_INC:
      case Token::TK_DEC:
        Next();
        expr = ParseUnaryExpression(IV_CHECK);
        // section 11.4.4, 11.4.5 throwing SyntaxError
        if (strict_ && expr->AsIdentifier()) {
          const Symbol sym = expr->AsIdentifier()->symbol();
          if (sym == symbol::eval()) {
            IV_RAISE("prefix expression to \"eval\" "
                  "not allowed in strict code");
          } else if (sym == symbol::arguments()) {
            IV_RAISE("prefix expression to \"arguments\" "
                  "not allowed in strict code");
          }
        }
        if (!expr->IsLeftHandSide()) {
          reference_error_ = true;
          IV_RAISE("assign to invalid left-hand-side");
        }
        assert(expr);
        result = factory_->NewUnaryOperation(op, expr, begin, line_number);
        break;

      default:
        result = ParsePostfixExpression(IV_CHECK);
        break;
    }
    return result;
  }

//  PostfixExpression
//    : LeftHandSideExpression
//    | LeftHandSideExpression INCREMENT
//    | LeftHandSideExpression DECREMENT
  Expression* ParsePostfixExpression(bool *res) {
    Expression* expr = ParseMemberExpression(true, IV_CHECK);
    if (!lexer_.has_line_terminator_before_next() &&
        (token_ == Token::TK_INC || token_ == Token::TK_DEC)) {
      // section 11.3.1, 11.3.2 throwing SyntaxError
      if (strict_ && expr->AsIdentifier()) {
        const Symbol sym = expr->AsIdentifier()->symbol();
        if (sym == symbol::eval()) {
          IV_RAISE("postfix expression to \"eval\" not allowed in strict code");
        } else if (sym == symbol::arguments()) {
          IV_RAISE("postfix expression to \"arguments\" "
                "not allowed in strict code");
        }
      }
      if (!expr->IsLeftHandSide()) {
        reference_error_ = true;
        IV_RAISE("assign to invalid left-hand-side");
      }
      assert(expr);
      expr = factory_->NewPostfixExpression(token_, expr,
                                            lexer_.end_position());
      Next();
    }
    return expr;
  }

//  LeftHandSideExpression
//    : NewExpression
//    | CallExpression
//
//  NewExpression
//    : MemberExpression
//    | NEW NewExpression
//
//  MemberExpression
//    : PrimaryExpression
//    | MemberExpression '[' Expression ']'
//    | NEW MemberExpression Arguments
  Expression* ParseMemberExpression(bool allow_call, bool *res) {
    Expression* expr;
    if (token_ != Token::TK_NEW) {
      expr = ParsePrimaryExpression(IV_CHECK);
    } else {
      Next();
      Expression* const target = ParseMemberExpression(false, IV_CHECK);
      Expressions* const args = factory_->template NewVector<Expression*>();
      if (token_ == Token::TK_LPAREN) {
        ParseArguments(args, IV_CHECK);
      }
      assert(target && args);
      expr = factory_->NewConstructorCall(
          target, args, lexer_.previous_end_position());
    }
    while (true) {
      switch (token_) {
        case Token::TK_LBRACK: {
          Next();
          Expression* const index = ParseExpression(true, IV_CHECK);
          assert(expr && index);
          expr = factory_->NewIndexAccess(expr, index);
          IV_EXPECT(Token::TK_RBRACK);
          break;
        }

        case Token::TK_PERIOD: {
          Next<IgnoreReservedWords>();  // IDENTIFIERNAME
          IV_IS(Token::TK_IDENTIFIER);
          const ast::SymbolHolder name = ParseSymbol();
          assert(expr);
          expr = factory_->NewIdentifierAccess(expr, name);
          break;
        }

        case Token::TK_LPAREN:
          if (allow_call) {
            Expressions* const args =
                factory_->template NewVector<Expression*>();
            ParseArguments(args, IV_CHECK);
            assert(expr && args);
            // record eval call
            if (expr->AsIdentifier() &&
                expr->AsIdentifier()->symbol() == symbol::eval()) {
              // this is maybe direct call to eval
              scope_->RecordDirectCallToEval();
              environment_->RecordDirectCallToEval();
            }
            expr = factory_->NewFunctionCall(expr, args,
                                             lexer_.previous_end_position());
          } else {
            return expr;
          }
          break;

        default:
          return expr;
      }
    }
  }

//  PrimaryExpression
//    : THIS
//    | IDENTIFIER
//    | Literal
//    | ArrayLiteral
//    | ObjectLiteral
//    | FunctionExpression
//    | '(' Expression ')'
//
//  Literal
//    : NULL_LITERAL
//    | TRUE_LITERAL
//    | FALSE_LITERAL
//    | NUMBER
//    | STRING
//    | REGEXP
  Expression* ParsePrimaryExpression(bool *res) {
    Expression* result = nullptr;
    switch (token_) {
      case Token::TK_FUNCTION:
        result = ParseFunctionLiteral(FunctionLiteral::EXPRESSION,
                                      FunctionLiteral::GENERAL, IV_CHECK);
        break;

      case Token::TK_THIS:
        result = factory_->NewThisLiteral(lexer_.begin_position(),
                                          lexer_.end_position(),
                                          lexer_.line_number());
        Next();
        break;

      case Token::TK_IDENTIFIER:
        result = ParseIdentifier();
        break;

      case Token::TK_NULL_LITERAL:
        result =
            factory_->NewNullLiteral(lexer_.begin_position(),
                                     lexer_.end_position(),
                                     lexer_.line_number());
        Next();
        break;

      case Token::TK_TRUE_LITERAL:
        result = factory_->NewTrueLiteral(lexer_.begin_position(),
                                          lexer_.end_position(),
                                          lexer_.line_number());
        Next();
        break;

      case Token::TK_FALSE_LITERAL:
        result = factory_->NewFalseLiteral(lexer_.begin_position(),
                                           lexer_.end_position(),
                                           lexer_.line_number());
        Next();
        break;

      case Token::TK_NUMBER:
        // section 7.8.3
        // strict mode forbids Octal Digits Literal
        if (strict_ && lexer_.NumericType() == lexer_type::OCTAL) {
          IV_RAISE("octal integer literal not allowed in strict code");
        }
        result = factory_->NewNumberLiteral(lexer_.Numeric(),
                                            lexer_.begin_position(),
                                            lexer_.end_position(),
                                            lexer_.line_number());
        Next();
        break;

      case Token::TK_STRING: {
        const typename lexer_type::State state = lexer_.StringEscapeType();
        if (strict_ && state == lexer_type::OCTAL) {
          IV_RAISE("octal escape sequence not allowed in strict code");
        }
        result = factory_->NewStringLiteral(lexer_.Buffer(),
                                            lexer_.begin_position(),
                                            lexer_.end_position(),
                                            lexer_.line_number());
        Next();
        break;
      }

      case Token::TK_DIV:
      case Token::TK_ASSIGN_DIV:
        result = ParseRegExpLiteral(token_ == Token::TK_ASSIGN_DIV, IV_CHECK);
        break;

      case Token::TK_LBRACK:
        result = ParseArrayLiteral(IV_CHECK);
        break;

      case Token::TK_LBRACE:
        result = ParseObjectLiteral(IV_CHECK);
        break;

      case Token::TK_LPAREN:
        Next();
        last_parenthesized_ = result = ParseExpression(true, IV_CHECK);
        IV_EXPECT(Token::TK_RPAREN);
        break;

      default:
        IV_UNEXPECT(token_);
        break;
    }
    return result;
  }

//  Arguments
//    : '(' ')'
//    | '(' ArgumentList ')'
//
//  ArgumentList
//    : AssignmentExpression
//    | ArgumentList ',' AssignmentExpression
  template<typename Container>
  Container* ParseArguments(Container* container, bool *res) {
    assert(token_ == Token::TK_LPAREN);
    Next();
    if (token_ != Token::TK_RPAREN) {
      Expression* const first = ParseAssignmentExpression(true, IV_CHECK);
      container->push_back(first);
      while (token_ == Token::TK_COMMA) {
        Next();
        Expression* const expr = ParseAssignmentExpression(true, IV_CHECK);
        container->push_back(expr);
      }
    }
    IV_EXPECT(Token::TK_RPAREN);
    return container;
  }

  Expression* ParseRegExpLiteral(bool contains_eq, bool *res) {
    assert(token_ == Token::TK_DIV || token_ == Token::TK_ASSIGN_DIV);
    if (lexer_.ScanRegExpLiteral(contains_eq)) {
      const std::vector<char16_t> content(lexer_.Buffer());
      if (!lexer_.ScanRegExpFlags()) {
        IV_RAISE("invalid regular expression flag");
      }
      RegExpLiteral* const expr = factory_->NewRegExpLiteral(
          content,
          lexer_.Buffer(),
          lexer_.begin_position(),
          lexer_.end_position(),
          lexer_.line_number());
      if (!expr) {
        IV_RAISE("invalid regular expression");
      }
      Next();
      return expr;
    } else {
      IV_RAISE("invalid regular expression");
    }
  }

//  ArrayLiteral
//    : '[' Elision_opt ']'
//    | '[' ElementList ']'
//    | '[' ElementList ',' Elision_opt ']'
//
//  ElementList
//    : Elision_opt AssignmentExpression
//    | ElementList ',' Elision_opt AssignmentExpression
//
//  Elision
//    : ','
//    | Elision ','
  Expression* ParseArrayLiteral(bool *res) {
    assert(token_ == Token::TK_LBRACK);
    bool is_primitive_constant_array = true;
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    MaybeExpressions* const items =
        factory_->template NewVector<Maybe<Expression> >();
    Next();
    while (token_ != Token::TK_RBRACK) {
      if (token_ == Token::TK_COMMA) {
        // when Token::TK_COMMA, only increment length
        items->push_back(nullptr);
        is_primitive_constant_array = false;
      } else {
        Expression* const expr = ParseAssignmentExpression(true, IV_CHECK);
        if (is_primitive_constant_array) {
          if (!(expr->AsStringLiteral() || expr->AsNumberLiteral() ||
               expr->AsTrueLiteral() || expr->AsFalseLiteral() ||
               expr->AsNullLiteral())) {
            // not primitive
            is_primitive_constant_array = false;
          }
        }
        items->push_back(expr);
      }
      if (token_ != Token::TK_RBRACK) {
        IV_EXPECT(Token::TK_COMMA);
      }
    }
    Next();
    assert(items);
    return factory_->NewArrayLiteral(
        items,
        is_primitive_constant_array,
        begin,
        lexer_.previous_end_position(),
        line_number);
  }



//  ObjectLiteral
//    : '{' PropertyNameAndValueList_opt '}'
//
//  PropertyNameAndValueList_opt
//    :
//    | PropertyNameAndValueList
//
//  PropertyNameAndValueList
//    : PropertyAssignment
//    | PropertyNameAndValueList ',' PropertyAssignment
//
//  PropertyAssignment
//    : PropertyName ':' AssignmentExpression
//    | 'get' PropertyName '(' ')' '{' FunctionBody '}'
//    | 'set' PropertyName '(' PropertySetParameterList ')' '{' FunctionBody '}'
//
//  PropertyName
//    : IDENTIFIER
//    | STRING
//    | NUMBER
//
//  PropertySetParameterList
//    : IDENTIFIER
  Expression* ParseObjectLiteral(bool *res) {
    typedef std::unordered_map<Symbol, int> ObjectMap;
    typedef typename ObjectLiteral::Property Property;
    typedef typename ObjectLiteral::Properties Properties;
    const std::size_t begin = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    Properties* const prop = factory_->template NewVector<Property>();
    ObjectMap map;

    // IDENTIFIERNAME
    Next<IgnoreReservedWordsAndIdentifyGetterOrSetter>();
    while (token_ != Token::TK_RBRACE) {
      if (token_ == Token::TK_GET || token_ == Token::TK_SET) {
        const bool is_get = token_ == Token::TK_GET;
        // this is getter or setter or usual prop
        Next<IgnoreReservedWords>();  // IDENTIFIERNAME
        if (token_ == Token::TK_COLON) {
          // property
          const ast::SymbolHolder ident(
              (is_get) ? symbol::get() : symbol::set(),
              lexer_.previous_begin_position(),
              lexer_.previous_end_position(),
              lexer_.previous_line_number());
          Next();
          Expression* expr = ParseAssignmentExpression(true, IV_CHECK);
          ObjectLiteral::AddDataProperty(prop, ident, expr);
          typename ObjectMap::iterator it = map.find(ident);
          if (it == map.end()) {
            map.insert(std::make_pair(ident, ObjectLiteral::DATA));
          } else {
            if (it->second != ObjectLiteral::DATA) {
              IV_RAISE("accessor property and data property "
                    "exist with the same name");
            } else {
              if (strict_) {
                IV_RAISE("multiple data property assignments "
                      "with the same name not allowed in strict code");
              }
            }
          }
        } else {
          // getter or setter
          if (Token::IsPropertyName(token_)) {
            const ast::SymbolHolder ident = ParsePropertyName(IV_CHECK);
            typename ObjectLiteral::PropertyDescriptorType type =
                (is_get) ? ObjectLiteral::GET : ObjectLiteral::SET;
            Expression* expr = ParseFunctionLiteral(
                FunctionLiteral::EXPRESSION,
                (is_get) ? FunctionLiteral::GETTER : FunctionLiteral::SETTER,
                IV_CHECK);
            ObjectLiteral::AddAccessor(prop, type, ident, expr);
            typename ObjectMap::iterator it = map.find(ident);
            if (it == map.end()) {
              map.insert(std::make_pair(ident, type));
            } else if (it->second & (ObjectLiteral::DATA | type)) {
              if (it->second & ObjectLiteral::DATA) {
                IV_RAISE("data property and accessor property "
                      "exist with the same name");
              } else {
                IV_RAISE("multiple same accessor properties "
                      "exist with the same name");
              }
            } else {
              it->second |= type;
            }
          } else {
            IV_RAISE_RECOVERABLE("invalid property name");
          }
        }
      } else if (Token::IsPropertyName(token_)) {
        const ast::SymbolHolder ident = ParsePropertyName(IV_CHECK);
        IV_EXPECT(Token::TK_COLON);
        Expression* expr = ParseAssignmentExpression(true, IV_CHECK);
        ObjectLiteral::AddDataProperty(prop, ident, expr);
        typename ObjectMap::iterator it = map.find(ident);
        if (it == map.end()) {
          map.insert(std::make_pair(ident, ObjectLiteral::DATA));
        } else {
          if (it->second != ObjectLiteral::DATA) {
            IV_RAISE("accessor property and data property "
                  "exist with the same name");
          } else {
            if (strict_) {
              IV_RAISE("multiple data property assignments "
                    "with the same name not allowed in strict code");
            }
          }
        }
      } else {
        IV_RAISE_RECOVERABLE("invalid property name");
      }

      if (token_ != Token::TK_RBRACE) {
        IV_IS(Token::TK_COMMA);
        // IDENTIFIERNAME
        Next<IgnoreReservedWordsAndIdentifyGetterOrSetter>();
      }
    }
    const std::size_t end = lexer_.begin_position();
    Next();
    assert(prop);
    return factory_->NewObjectLiteral(prop, begin, end, line_number);
  }

  FunctionLiteral* ParseFunctionLiteral(
      typename FunctionLiteral::DeclType decl_type,
      typename FunctionLiteral::ArgType arg_type,
      bool *res) {
    // IDENTIFIER
    // IDENTIFIER_opt
    std::size_t throw_error_if_strict_code_line = 0;
    const std::size_t begin_position = lexer_.begin_position();
    const std::size_t line_number = lexer_.line_number();
    enum {
      kDetectNone = 0,
      kDetectEvalName,
      kDetectArgumentsName,
      kDetectEvalParameter,
      kDetectArgumentsParameter,
      kDetectDuplicateParameter,
      kDetectFutureReservedWords
    } throw_error_if_strict_code = kDetectNone;

    Assigneds* const params = factory_->template NewVector<Assigned*>();
    Assigned* name = nullptr;

    if (arg_type == FunctionLiteral::GENERAL) {
      assert(token_ == Token::TK_FUNCTION);
      Next(true);  // preparing for strict directive
      const Token::Type current = token_;
      if (current == Token::TK_IDENTIFIER ||
          Token::IsAddedFutureReservedWordInStrictCode(current)) {
        const ast::SymbolHolder sym = ParseSymbol();
        if (Token::IsAddedFutureReservedWordInStrictCode(current)) {
          throw_error_if_strict_code = kDetectFutureReservedWords;
          throw_error_if_strict_code_line = lexer_.line_number();
        } else {
          assert(current == Token::TK_IDENTIFIER);
          if (sym == symbol::eval() || sym == symbol::arguments()) {
            throw_error_if_strict_code =
                (sym == symbol::eval()) ?
                kDetectEvalName : kDetectArgumentsName;
            throw_error_if_strict_code_line = lexer_.line_number();
          }
        }
        name = factory_->NewAssigned(
            sym,
            (decl_type == FunctionLiteral::STATEMENT ||
             decl_type == FunctionLiteral::EXPRESSION));
      } else if (decl_type == FunctionLiteral::DECLARATION ||
                 decl_type == FunctionLiteral::STATEMENT) {
        IV_IS(Token::TK_IDENTIFIER);
      }
    }

    const std::size_t begin_block_position = lexer_.begin_position();

    //  '(' FormalParameterList_opt ')'
    IV_IS(Token::TK_LPAREN);
    Next(true);  // preparing for strict directive

    if (arg_type == FunctionLiteral::GETTER) {
      // if getter, parameter count is 0
      IV_EXPECT(Token::TK_RPAREN);
    } else if (arg_type == FunctionLiteral::SETTER) {
      // if setter, parameter count is 1
      const Token::Type current = token_;
      if (current != Token::TK_IDENTIFIER &&
          !Token::IsAddedFutureReservedWordInStrictCode(current)) {
        IV_IS(Token::TK_IDENTIFIER);
      }
      const ast::SymbolHolder sym = ParseSymbol();
      if (!throw_error_if_strict_code) {
        if (Token::IsAddedFutureReservedWordInStrictCode(current)) {
          throw_error_if_strict_code = kDetectFutureReservedWords;
          throw_error_if_strict_code_line = lexer_.line_number();
        } else {
          assert(current == Token::TK_IDENTIFIER);
          if (sym == symbol::eval() || sym == symbol::arguments()) {
            throw_error_if_strict_code =
                (sym == symbol::eval()) ?
                kDetectEvalName : kDetectArgumentsName;
            throw_error_if_strict_code_line = lexer_.line_number();
          }
        }
      }
      params->push_back(factory_->NewAssigned(sym, false));
      IV_EXPECT(Token::TK_RPAREN);
    } else {
      if (token_ != Token::TK_RPAREN) {
        SymbolSet param_set;
        do {
          const Token::Type current = token_;
          if (current != Token::TK_IDENTIFIER &&
              !Token::IsAddedFutureReservedWordInStrictCode(current)) {
            IV_IS(Token::TK_IDENTIFIER);
          }
          const ast::SymbolHolder sym = ParseSymbol();
          if (!throw_error_if_strict_code) {
            if (Token::IsAddedFutureReservedWordInStrictCode(current)) {
              throw_error_if_strict_code = kDetectFutureReservedWords;
              throw_error_if_strict_code_line = lexer_.line_number();
            } else {
              assert(current == Token::TK_IDENTIFIER);
              if (sym == symbol::eval() || sym == symbol::arguments()) {
                throw_error_if_strict_code =
                    (sym == symbol::eval()) ?
                    kDetectEvalName : kDetectArgumentsName;
                throw_error_if_strict_code_line = lexer_.line_number();
              }
            }
            if ((!throw_error_if_strict_code) &&
                (param_set.find(sym) != param_set.end())) {
              throw_error_if_strict_code = kDetectDuplicateParameter;
              throw_error_if_strict_code_line = lexer_.line_number();
            }
          }
          params->push_back(factory_->NewAssigned(sym, false));
          param_set.insert(sym);
          if (token_ == Token::TK_COMMA) {
            Next(true);
          } else {
            break;
          }
        } while (true);
      }
      IV_EXPECT(Token::TK_RPAREN);
    }

    //  '{' FunctionBody '}'
    //
    //  FunctionBody
    //    :
    //    | SourceElements
    IV_EXPECT(Token::TK_LBRACE);

    Statements* const body = factory_->template NewVector<Statement*>();
    Scope* const scope = factory_->NewScope(decl_type, params);
    FunctionEnvironment<Scope> environment(environment_, &environment_, scope);
    const ScopeSwitcher scope_switcher(this, scope);
    const TargetSwitcher target_switcher(this);
    const bool function_is_strict =
        ParseSourceElements(Token::TK_RBRACE, body, IV_CHECK);
    if (strict_ || function_is_strict) {
      // section 13.1
      // Strict Mode Restrictions
      switch (throw_error_if_strict_code) {
        case kDetectNone:
          break;
        case kDetectEvalName:
          IV_RAISE_NUMBER(
              "function name \"eval\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectArgumentsName:
          IV_RAISE_NUMBER(
              "function name \"arguments\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectEvalParameter:
          IV_RAISE_NUMBER(
              "parameter \"eval\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectArgumentsParameter:
          IV_RAISE_NUMBER(
              "parameter \"arguments\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectDuplicateParameter:
          IV_RAISE_NUMBER(
              "duplicate parameter not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectFutureReservedWords:
          IV_RAISE_NUMBER(
              "FutureReservedWords is found in strict code",
              throw_error_if_strict_code_line);
          break;
      }
    }
    Next();
    const std::size_t end_block_position = lexer_.previous_end_position();
    scope->set_strict(function_is_strict);
    environment.Resolve((name && name->immutable()) ? name : nullptr);
    assert(params && body && scope);
    return factory_->NewFunctionLiteral(decl_type,
                                        name,
                                        params,
                                        body,
                                        scope,
                                        function_is_strict,
                                        begin_block_position,
                                        end_block_position,
                                        begin_position,
                                        end_block_position,
                                        line_number);
  }

  ast::SymbolHolder ParsePropertyName(bool* res) {
    assert(Token::IsPropertyName(token_));
    if (token_ == Token::TK_NUMBER) {
      if (strict_ && lexer_.NumericType() == lexer_type::OCTAL) {
        IV_RAISE_WITH(
            "octal integer literal not allowed in strict code",
            ast::SymbolHolder());
      }
      const double val = lexer_.Numeric();
      dtoa::StringPieceDToA builder;
      builder.Build(val);
      const Symbol name = table_->Lookup(builder.buffer());
      Next();
      return ast::SymbolHolder(
          name,
          lexer_.previous_begin_position(),
          lexer_.previous_end_position(),
          lexer_.previous_line_number());
    } else if (token_ == Token::TK_STRING) {
      if (strict_ && lexer_.StringEscapeType() == lexer_type::OCTAL) {
        IV_RAISE_WITH(
            "octal escape sequence not allowed in strict code",
            ast::SymbolHolder());
      }
      const Symbol name = table_->Lookup(
          UStringPiece(lexer_.Buffer().data(), lexer_.Buffer().size()));
      Next();
      return ast::SymbolHolder(
          name,
          lexer_.previous_begin_position(),
          lexer_.previous_end_position(),
          lexer_.previous_line_number());
    } else {
      return ParseSymbol();
    }
  }

  ast::SymbolHolder ParseSymbol() {
    const Symbol sym = table_->Lookup(
        UStringPiece(lexer_.Buffer().data(), lexer_.Buffer().size()));
    Next();
    return ast::SymbolHolder(
        sym,
        lexer_.previous_begin_position(),
        lexer_.previous_end_position(),
        lexer_.previous_line_number());
  }

  Identifier* ParseIdentifier() {
    assert(token_ == Token::TK_IDENTIFIER);
    const Symbol symbol = table_->Lookup(
        UStringPiece(lexer_.Buffer().data(), lexer_.Buffer().size()));
    Identifier* const ident = factory_->NewIdentifier(
        Token::TK_IDENTIFIER,
        symbol,
        lexer_.begin_position(),
        lexer_.end_position(),
        lexer_.line_number());
    if (symbol == symbol::arguments()) {
      scope_->RecordArguments();
    }
    environment_->Referencing(symbol);
    Next();
    return ident;
  }

  bool ContainsLabel(const SymbolSet* labels, Symbol label) const {
    return labels && labels->find(label) != labels->end();
  }

  bool IsDuplicateLabel(Symbol label) const {
    if (ContainsLabel(labels_, label)) {
      return true;
    }
    for (const Target* target = target_;
         target != nullptr; target = target->previous()) {
      if (ContainsLabel(target->labels(), label)) {
        return true;
      }
    }
    return false;
  }

  BreakableStatement** LookupBreakableTarget(Symbol label) const {
    for (Target* target = target_;
         target != nullptr; target = target->previous()) {
      if (ContainsLabel(target->labels(), label)) {
        return target->node();
      }
    }
    return nullptr;
  }

  BreakableStatement** LookupBreakableTarget() const {
    for (Target* target = target_;
         target != nullptr; target = target->previous()) {
      if (target->IsAnonymous()) {
        return target->node();
      }
    }
    return nullptr;
  }

  IterationStatement** LookupContinuableTarget(Symbol label) const {
    for (Target* target = target_;
         target != nullptr; target = target->previous()) {
      if (target->IsIteration() && ContainsLabel(target->labels(), label)) {
        return reinterpret_cast<IterationStatement**>(target->node());
      }
    }
    return nullptr;
  }

  IterationStatement** LookupContinuableTarget() const {
    for (Target* target = target_;
         target != nullptr; target = target->previous()) {
      if (target->IsIteration()) {
        return reinterpret_cast<IterationStatement**>(target->node());
      }
    }
    return nullptr;
  }

  void SetErrorHeader(std::size_t line) {
    error_.append(lexer_.filename());
    error_.append(":");
    UInt64ToString(line, std::back_inserter(error_));
    if (!reference_error_) {
      error_.append(": SyntaxError: ");
    } else {
      error_.append(": ReferenceError: ");
    }
  }

  void ReportUnexpectedToken(Token::Type expected_token) {
    SetErrorHeader(lexer_.line_number());
    switch (token_) {
      case Token::TK_STRING:
        error_.append("unexpected string");
        break;
      case Token::TK_NUMBER:
        error_.append("unexpected number");
        break;
      case Token::TK_IDENTIFIER:
        error_.append("unexpected identifier");
        break;
      case Token::TK_EOS:
        error_.append("unexpected EOS");
        break;
      case Token::TK_ILLEGAL: {
        error_.append("illegal character");
        break;
      }
      default: {
        error_.append("unexpected token ");
        error_.append(Token::ToString(token_));
        break;
      }
    }
  }

  // return true if Automatic Semicolon Insertion (ASI) is executed
  bool IsAutomaticSemicolonInserted() {
    return
        lexer_.has_line_terminator_before_next() ||
        token_ == Token::TK_SEMICOLON ||
        token_ == Token::TK_RBRACE ||
        token_ == Token::TK_EOS;
  }

  bool ExpectSemicolon(bool *res) {
    if (lexer_.has_line_terminator_before_next()) {
      return true;
    }
    if (token_ == Token::TK_SEMICOLON) {
      Next();
      return true;
    }
    if (token_ == Token::TK_RBRACE || token_ == Token::TK_EOS) {
      return true;
    }
    IV_UNEXPECT_WITH(token_, false);
  }

  template<typename LexType>
  inline Token::Type Next() {
    return token_ = lexer_.template Next<LexType>(strict_);
  }
  inline Token::Type Next() {
    return token_ = lexer_.template Next<IdentifyReservedWords>(strict_);
  }
  inline Token::Type Next(bool strict) {
    return token_ = lexer_.template Next<IdentifyReservedWords>(strict);
  }

  inline Scope* scope() const {
    return scope_;
  }
  inline void set_scope(Scope* scope) {
    scope_ = scope;
  }

  inline const std::string& error() const {
    return error_;
  }

  inline Target* target() const {
    return target_;
  }
  inline void set_target(Target* target) {
    target_ = target;
  }

  inline Factory* factory() const {
    return factory_;
  }

  inline SymbolSet* labels() const {
    return labels_;
  }
  inline void set_labels(SymbolSet* labels) {
    labels_ = labels;
  }

  inline bool strict() const {
    return strict_;
  }

  inline void set_strict(bool strict) {
    strict_ = strict;
  }

  bool reference_error() const { return reference_error_; }

  inline bool RecoverableError() const {
    return (!(error_state_ & kNotRecoverable)) && token_ == Token::TK_EOS;
  }

  static bool IsStrictDirective(StringLiteral* literal) {
    return literal->value().compare(detail::kUseStrict.data()) == 0;
  }

 protected:
  class ScopeSwitcher : private Noncopyable<> {
   public:
    ScopeSwitcher(parser_type* parser, Scope* scope)
      : parser_(parser) {
      scope->SetUpperScope(parser_->scope());
      parser_->set_scope(scope);
    }
    ~ScopeSwitcher() {
      assert(parser_->scope() != nullptr);
      parser_->set_scope(parser_->scope()->GetUpperScope());
    }
   private:
    parser_type* parser_;
  };

  class LabelSwitcher : private Noncopyable<> {
   public:
    LabelSwitcher(parser_type* parser, SymbolSet* labels, Symbol label)
      : parser_(parser),
        labels_(labels),
        newly_created_(!labels_) {
      if (newly_created_) {
        labels_ = new SymbolSet;
        parser_->set_labels(labels_);
      }
      labels_->insert(label);
    }
    ~LabelSwitcher() {
      if (newly_created_) {
        parser_->set_labels(nullptr);
        delete labels_;
      }
    }
   private:
    parser_type* parser_;
    SymbolSet* original_;
    SymbolSet* labels_;
    bool newly_created_;
  };

  class StrictSwitcher : private Noncopyable<> {
   public:
    explicit StrictSwitcher(parser_type* parser)
      : parser_(parser),
        prev_(parser->strict()) {
    }
    ~StrictSwitcher() {
      parser_->set_strict(prev_);
    }
    inline void SwitchStrictMode() const {
      parser_->set_strict(true);
    }
    inline bool IsStrict() const {
      return parser_->strict();
    }
   private:
    parser_type* parser_;
    bool prev_;
  };

  lexer_type lexer_;
  Token::Type token_;
  std::string error_;
  bool strict_;
  bool reference_error_;
  int error_state_;
  Factory* factory_;
  Scope* scope_;
  Environment* environment_;
  Target* target_;
  SymbolSet* labels_;
  SymbolTable* table_;
  Expression* last_parenthesized_;
};
#undef IV_IS
#undef IV_EXPECT
#undef IV_UNEXPECT
#undef IV_UNEXPECT_WITH
#undef IV_RAISE
#undef IV_RAISE_NUMBER
#undef IV_RAISE_WITH
#undef IV_RAISE_IMPL
#undef IV_RAISE_RECOVERABLE
#undef IV_CHECK
#undef IV_CHECK_WITH
} }  // namespace iv::core
#endif  // IV_PARSER_H_
