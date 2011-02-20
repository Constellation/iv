#ifndef _IV_PARSER_H_
#define _IV_PARSER_H_
#include <cstdio>
#include <cstring>
#include <string>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <tr1/type_traits>
#include <tr1/array>
#include "static_assert.h"
#include "maybe.h"
#include "ast.h"
#include "ast_factory.h"
#include "lexer.h"
#include "dtoa.h"
#include "noncopyable.h"
#include "utils.h"
#include "ustring.h"
#include "enable_if.h"
#include "none.h"

#define IS(token)\
  do {\
    if (token_ != token) {\
      *res = false;\
      ReportUnexpectedToken(token);\
      return NULL;\
    }\
  } while (0)

#define EXPECT(token)\
  do {\
    if (token_ != token) {\
      *res = false;\
      ReportUnexpectedToken(token);\
      return NULL;\
    }\
    Next();\
  } while (0)

#define UNEXPECT(token)\
  do {\
    *res = false;\
    ReportUnexpectedToken(token);\
    return NULL;\
  } while (0)

#define RAISE(str)\
  do {\
    *res = false;\
    error_state_ |= kNotRecoverable;\
    SetErrorHeader(lexer_.line_number());\
    error_.append(str);\
    return NULL;\
  } while (0)

#define RAISE_RECOVERVABLE(str)\
  do {\
    *res = false;\
    SetErrorHeader(lexer_.line_number());\
    error_.append(str);\
    return NULL;\
  } while (0)

#define RAISE_WITH_NUMBER(str, line)\
  do {\
    *res = false;\
    error_state_ |= kNotRecoverable;\
    SetErrorHeader(line);\
    error_.append(str);\
    return NULL;\
  } while (0)

#define CHECK  res);\
  if (!*res) {\
    return NULL;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

namespace iv {
namespace core {
namespace detail {
template<typename T>
class ParserData {
 private:
  static const char* use_strict;
  static const char* arguments;
  static const char* eval;
  static const char* get;
  static const char* set;
 public:
  static const UString kUseStrict;
  static const UString kArguments;
  static const UString kEval;
  static const UString kGet;
  static const UString kSet;
};

template<typename T>
const char * ParserData<T>::use_strict = "use strict";

template<typename T>
const char * ParserData<T>::arguments = "arguments";

template<typename T>
const char * ParserData<T>::eval = "eval";

template<typename T>
const char * ParserData<T>::get = "get";

template<typename T>
const char * ParserData<T>::set = "set";

template<typename T>
const UString ParserData<T>::kUseStrict(
    ParserData<T>::use_strict,
    ParserData<T>::use_strict + std::strlen(ParserData<T>::use_strict));
template<typename T>
const UString ParserData<T>::kArguments(
    ParserData<T>::arguments,
    ParserData<T>::arguments + std::strlen(ParserData<T>::arguments));
template<typename T>
const UString ParserData<T>::kEval(
    ParserData<T>::eval,
    ParserData<T>::eval + std::strlen(ParserData<T>::eval));
template<typename T>
const UString ParserData<T>::kGet(
    ParserData<T>::get,
    ParserData<T>::get + std::strlen(ParserData<T>::get));
template<typename T>
const UString ParserData<T>::kSet(
    ParserData<T>::set,
    ParserData<T>::set + std::strlen(ParserData<T>::set));

}  // namespace iv::core::detail

typedef detail::ParserData<None> ParserData;

template<typename Factory,
         typename Source,
         bool UseFunctionStatement,
         bool ReduceExpressions>
class Parser
  : private Noncopyable<
    Parser<Factory, Source, UseFunctionStatement, ReduceExpressions> >::type {
 public:
  typedef Parser<Factory, Source,
                 UseFunctionStatement, ReduceExpressions> this_type;
  typedef this_type parser_type;
  typedef Lexer<Source> lexer_type;
#define V(AST) typedef typename ast::AST<Factory> AST;
  AST_NODE_LIST(V)
#undef V
#define V(XS) typedef typename ast::AstNode<Factory>::XS XS;
  AST_LIST_LIST(V)
#undef V
#define V(S) typedef typename SpaceUString<Factory>::type S;
  AST_STRING(V)
#undef V

  enum ErrorState {
    kNotRecoverable = 1
  };

  class Target : private Noncopyable<Target>::type {
   public:
    typedef typename SpaceVector<Factory, Identifier*>::type Identifiers;
    enum Type {
      kNamedOnlyStatement = 0,  // (00)2
      kIterationStatement = 2,  // (10)2
      kSwitchStatement = 3      // (11)2
    };
    Target(parser_type* parser, Type type)
      : parser_(parser),
        prev_(parser->target()),
        labels_(parser->labels()),
        node_(NULL),
        type_(type) {
      parser_->set_target(this);
      parser_->set_labels(NULL);
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
    inline Identifiers* labels() const {
      return labels_;
    }
    inline void set_node(BreakableStatement* node) {
      if (node_) {
        *node_ = node;
      }
    }
   private:
    parser_type* parser_;
    Target* prev_;
    Identifiers* labels_;
    BreakableStatement** node_;
    int type_;
  };

  class TargetSwitcher : private Noncopyable<TargetSwitcher>::type {
   public:
    explicit TargetSwitcher(parser_type* parser)
      : parser_(parser),
        target_(parser->target()),
        labels_(parser->labels()) {
      parser_->set_target(NULL);
      parser_->set_labels(NULL);
    }
    ~TargetSwitcher() {
      parser_->set_target(target_);
      parser_->set_labels(labels_);
    }
   private:
    parser_type* parser_;
    Target* target_;
    Identifiers* labels_;
  };

  Parser(Factory* factory, const Source* source)
    : lexer_(source),
      error_(),
      strict_(false),
      error_state_(0),
      factory_(factory),
      scope_(NULL),
      target_(NULL),
      labels_(NULL) {
  }

// Program
//   : SourceElements
  FunctionLiteral* ParseProgram() {
    Identifiers* params = factory_->template NewVector<Identifier*>();
    Statements* body = factory_->template NewVector<Statement*>();
    Scope* const scope = factory_->NewScope(FunctionLiteral::GLOBAL);
    assert(target_ == NULL);
    bool error_flag = true;
    bool *res = &error_flag;
    const ScopeSwitcher scope_switcher(this, scope);
    Next();
    const bool strict = ParseSourceElements(Token::EOS, body, CHECK);
    const std::size_t end_position = lexer_.end_position();
#ifdef DEBUG
    if (error_flag) {
      assert(params && body && scope);
    }
#endif
    return (error_flag) ?
        factory_->NewFunctionLiteral(FunctionLiteral::GLOBAL,
                                     NULL,
                                     params,
                                     body,
                                     scope,
                                     strict,
                                     0,
                                     end_position,
                                     0,
                                     end_position) : NULL;
  }

// SourceElements
//   : SourceElement
//   | SourceElement SourceElements
//
// SourceElement
//   : Statements
//   | FunctionDeclaration
  bool ParseSourceElements(Token::Type end,
                           Statements* body,
                           bool *res) {
    Statement* stmt;
    bool recognize_directive = true;
    const StrictSwitcher strict_switcher(this);
    while (token_ != end) {
      if (token_ == Token::FUNCTION) {
        // FunctionDeclaration
        stmt = ParseFunctionDeclaration(CHECK);
        body->push_back(stmt);
        recognize_directive = false;
      } else {
        stmt = ParseStatement(CHECK);
        // directive prologue
        if (recognize_directive) {
          if (stmt->AsExpressionStatement()) {
            Expression* const expr = stmt->AsExpressionStatement()->expr();
            if (expr->AsDirectivable()) {
              // expression is directive
              if (!strict_switcher.IsStrict() &&
                  expr->AsStringLiteral()->value().compare(
                      ParserData::kUseStrict.data()) == 0) {
                strict_switcher.SwitchStrictMode();
                // and one token lexed is not in strict
                // so rescan
                if (token_ == Token::IDENTIFIER) {
                  typedef detail::Keyword<IdentifyReservedWords> KeywordChecker;
                  token_ =  KeywordChecker::Detect(lexer_.Buffer(), true);
                }
              }
            } else {
              recognize_directive = false;
            }
          } else {
            recognize_directive = false;
          }
        }
        body->push_back(stmt);
      }
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
    Statement *result = NULL;
    switch (token_) {
      case Token::LBRACE:
        // Block
        result = ParseBlock(CHECK);
        break;

      case Token::CONST:
        if (strict_) {
          RAISE("\"const\" not allowed in strict code");
        }
      case Token::VAR:
        // VariableStatement
        result = ParseVariableStatement(CHECK);
        break;

      case Token::SEMICOLON:
        // EmptyStatement
        result = ParseEmptyStatement();
        break;

      case Token::IF:
        // IfStatement
        result = ParseIfStatement(CHECK);
        break;

      case Token::DO:
        // IterationStatement
        // do while
        result = ParseDoWhileStatement(CHECK);
        break;

      case Token::WHILE:
        // IterationStatement
        // while
        result = ParseWhileStatement(CHECK);
        break;

      case Token::FOR:
        // IterationStatement
        // for
        result = ParseForStatement(CHECK);
        break;

      case Token::CONTINUE:
        // ContinueStatement
        result = ParseContinueStatement(CHECK);
        break;

      case Token::BREAK:
        // BreakStatement
        result = ParseBreakStatement(CHECK);
        break;

      case Token::RETURN:
        // ReturnStatement
        result = ParseReturnStatement(CHECK);
        break;

      case Token::WITH:
        // WithStatement
        result = ParseWithStatement(CHECK);
        break;

      case Token::SWITCH:
        // SwitchStatement
        result = ParseSwitchStatement(CHECK);
        break;

      case Token::THROW:
        // ThrowStatement
        result = ParseThrowStatement(CHECK);
        break;

      case Token::TRY:
        // TryStatement
        result = ParseTryStatement(CHECK);
        break;

      case Token::DEBUGGER:
        // DebuggerStatement
        result = ParseDebuggerStatement(CHECK);
        break;

      case Token::FUNCTION:
        // FunctionStatement (not in ECMA-262 5th)
        // FunctionExpression
        result = ParseFunctionStatement<UseFunctionStatement>(CHECK);
        break;

      case Token::IDENTIFIER:
        // LabelledStatement or ExpressionStatement
        result = ParseExpressionOrLabelledStatement(CHECK);
        break;

      case Token::ILLEGAL:
        UNEXPECT(token_);
        break;

      default:
        // ExpressionStatement or ILLEGAL
        result = ParseExpressionStatement(CHECK);
        break;
    }
    return result;
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
    assert(token_ == Token::FUNCTION);
    FunctionLiteral* const expr = ParseFunctionLiteral(
        FunctionLiteral::DECLARATION,
        FunctionLiteral::GENERAL, CHECK);
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
    assert(token_ == Token::LBRACE);
    const std::size_t begin = lexer_.begin_position();
    Statements* const body = factory_->template NewVector<Statement*>();
    Target target(this, Target::kNamedOnlyStatement);

    Next();
    while (token_ != Token::RBRACE) {
      Statement* const stmt = ParseStatement(CHECK);
      body->push_back(stmt);
    }
    Next();
    assert(body);
    Block* const block = factory_->NewBlock(body,
                                            begin,
                                            lexer_.previous_end_position());
    target.set_node(block);
    return block;
  }

//  VariableStatement
//    : VAR VariableDeclarationList ';'
//    : CONST VariableDeclarationList ';'
  Statement* ParseVariableStatement(bool *res) {
    assert(token_ == Token::VAR || token_ == Token::CONST);
    const Token::Type op = token_;
    const std::size_t begin = lexer_.begin_position();
    Declarations* const decls = factory_->template NewVector<Declaration*>();
    ParseVariableDeclarations(decls, token_ == Token::CONST, true, CHECK);
    ExpectSemicolon(CHECK);
    assert(decls);
    return factory_->NewVariableStatement(op,
                                          decls,
                                          begin,
                                          lexer_.previous_end_position());
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
    Identifier* name;
    Expression* expr;
    Declaration* decl;

    do {
      Next();
      IS(Token::IDENTIFIER);
      name = ParseIdentifier(lexer_.Buffer());
      // section 12.2.1
      // within the strict code, Identifier must not be "eval" or "arguments"
      if (strict_) {
        const EvalOrArguments val = IsEvalOrArguments(name);
        if (val) {
          if (val == kEval) {
            RAISE("assignment to \"eval\" not allowed in strict code");
          } else {
            assert(val == kArguments);
            RAISE("assignment to \"arguments\" not allowed in strict code");
          }
        }
      }

      if (token_ == Token::ASSIGN) {
        Next();
        // AssignmentExpression
        expr = ParseAssignmentExpression(contains_in, CHECK);
        assert(name);
        decl = factory_->NewDeclaration(name, expr);
      } else {
        // Undefined Expression
        assert(name);
        decl = factory_->NewDeclaration(name, NULL);
      }
      decls->push_back(decl);
      scope_->AddUnresolved(name, is_const);
    } while (token_ == Token::COMMA);

    return decls;
  }

//  EmptyStatement
//    : ';'
  Statement* ParseEmptyStatement() {
    assert(token_ == Token::SEMICOLON);
    Next();
    return factory_->NewEmptyStatement(lexer_.previous_begin_position(),
                                       lexer_.previous_end_position());
  }

//  IfStatement
//    : IF '(' Expression ')' Statement ELSE Statement
//    | IF '(' Expression ')' Statement
  Statement* ParseIfStatement(bool *res) {
    assert(token_ == Token::IF);
    const std::size_t begin = lexer_.begin_position();
    Statement* else_statement = NULL;
    Next();

    EXPECT(Token::LPAREN);

    Expression* const expr = ParseExpression(true, CHECK);

    EXPECT(Token::RPAREN);

    Statement* const then_statement = ParseStatement(CHECK);
    if (token_ == Token::ELSE) {
      Next();
      else_statement = ParseStatement(CHECK);
    }
    assert(expr && then_statement);
    return factory_->NewIfStatement(expr,
                                    then_statement,
                                    else_statement,
                                    begin);
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
    assert(token_ == Token::DO);
    const std::size_t begin = lexer_.begin_position();
    Target target(this, Target::kIterationStatement);
    Next();

    Statement* const stmt = ParseStatement(CHECK);

    EXPECT(Token::WHILE);

    EXPECT(Token::LPAREN);

    Expression* const expr = ParseExpression(true, CHECK);

    EXPECT(Token::RPAREN);

    // ex:
    //   do {
    //     print("valid syntax");
    //   } while (0) return true;
    // is syntax valid
    if (token_ == Token::SEMICOLON) {
      Next();
    }
    assert(stmt && expr);
    DoWhileStatement* const dowhile = factory_->NewDoWhileStatement(
        stmt, expr, begin, lexer_.previous_end_position());
    target.set_node(dowhile);
    return dowhile;
  }

//  WHILE '(' Expression ')' Statement
  Statement* ParseWhileStatement(bool *res) {
    assert(token_ == Token::WHILE);
    const std::size_t begin = lexer_.begin_position();
    Next();

    EXPECT(Token::LPAREN);

    Expression* const expr = ParseExpression(true, CHECK);
    Target target(this, Target::kIterationStatement);

    EXPECT(Token::RPAREN);

    Statement* const stmt = ParseStatement(CHECK);
    assert(stmt);
    WhileStatement* const whilestmt = factory_->NewWhileStatement(stmt,
                                                                  expr, begin);

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
    assert(token_ == Token::FOR);
    const std::size_t for_stmt_begin = lexer_.begin_position();
    Next();

    EXPECT(Token::LPAREN);

    Statement *init = NULL;

    if (token_ != Token::SEMICOLON) {
      if (token_ == Token::VAR || token_ == Token::CONST) {
        const std::size_t begin = lexer_.begin_position();
        const Token::Type op = token_;
        Declarations* const decls =
            factory_->template NewVector<Declaration*>();
        ParseVariableDeclarations(decls, token_ == Token::CONST, false, CHECK);
        if (token_ == Token::IN) {
          assert(decls);
          VariableStatement* const var =
              factory_->NewVariableStatement(op,
                                             decls,
                                             begin,
                                             lexer_.previous_end_position());
          init = var;
          // for in loop
          Next();
          const Declarations& decls = var->decls();
          if (decls.size() != 1) {
            // ForInStatement requests VaraibleDeclarationNoIn (not List),
            // so check declarations' size is 1.
            RAISE("invalid for-in left-hand-side");
          }
          Expression* const enumerable = ParseExpression(true, CHECK);
          EXPECT(Token::RPAREN);
          Target target(this, Target::kIterationStatement);
          Statement* const body = ParseStatement(CHECK);
          assert(body && init && enumerable);
          ForInStatement* const forstmt =
              factory_->NewForInStatement(body, init, enumerable,
                                          for_stmt_begin);
          target.set_node(forstmt);
          return forstmt;
        } else {
          assert(decls);
          init = factory_->NewVariableStatement(op, decls,
                                                begin,
                                                lexer_.end_position());
        }
      } else {
        Expression* const init_expr = ParseExpression(false, CHECK);
        if (token_ == Token::IN) {
          // for in loop
          assert(init_expr);
          init = factory_->NewExpressionStatement(init_expr,
                                                  lexer_.previous_end_position());
          if (!init_expr->IsValidLeftHandSide()) {
            RAISE("invalid for-in left-hand-side");
          }
          Next();
          Expression* const enumerable = ParseExpression(true, CHECK);
          EXPECT(Token::RPAREN);
          Target target(this, Target::kIterationStatement);
          Statement* const body = ParseStatement(CHECK);
          assert(body && init && enumerable);
          ForInStatement* const forstmt =
              factory_->NewForInStatement(body, init, enumerable,
                                          for_stmt_begin);
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
    EXPECT(Token::SEMICOLON);

    Expression* cond = NULL;
    if (token_ == Token::SEMICOLON) {
      // no cond expr
      Next();
    } else {
      cond = ParseExpression(true, CHECK);
      EXPECT(Token::SEMICOLON);
    }

    ExpressionStatement* next = NULL;
    if (token_ == Token::RPAREN) {
      Next();
    } else {
      Expression* const next_expr = ParseExpression(true, CHECK);
      assert(next_expr);
      next = factory_->NewExpressionStatement(next_expr,
                                              lexer_.previous_end_position());
      EXPECT(Token::RPAREN);
    }

    Target target(this, Target::kIterationStatement);
    Statement* const body = ParseStatement(CHECK);
    assert(body);
    ForStatement* const forstmt =
        factory_->NewForStatement(body, init, cond, next, for_stmt_begin);
    target.set_node(forstmt);
    return forstmt;
  }

//  ContinueStatement
//    : CONTINUE Identifier_opt ';'
  Statement* ParseContinueStatement(bool *res) {
    assert(token_ == Token::CONTINUE);
    const std::size_t begin = lexer_.begin_position();
    Identifier* label = NULL;
    IterationStatement** target;
    Next();
    if (!lexer_.has_line_terminator_before_next() &&
        token_ != Token::SEMICOLON &&
        token_ != Token::RBRACE &&
        token_ != Token::EOS) {
      IS(Token::IDENTIFIER);
      label = ParseIdentifier(lexer_.Buffer());
      target = LookupContinuableTarget(label);
      if (!target) {
        RAISE("label not found");
      }
    } else {
      target = LookupContinuableTarget();
      if (!target) {
        RAISE("label not found");
      }
    }
    ExpectSemicolon(CHECK);
    return factory_->NewContinueStatement(label, target, begin,
                                          lexer_.previous_end_position());
  }

//  BreakStatement
//    : BREAK Identifier_opt ';'
  Statement* ParseBreakStatement(bool *res) {
    assert(token_ == Token::BREAK);
    const std::size_t begin = lexer_.begin_position();
    Identifier* label = NULL;
    BreakableStatement** target = NULL;
    Next();
    if (!lexer_.has_line_terminator_before_next() &&
        token_ != Token::SEMICOLON &&
        token_ != Token::RBRACE &&
        token_ != Token::EOS) {
      // label
      IS(Token::IDENTIFIER);
      label = ParseIdentifier(lexer_.Buffer());
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
          RAISE("label not found");
        }
      }
    } else {
      target = LookupBreakableTarget();
      if (!target) {
        RAISE("label not found");
      }
    }
    ExpectSemicolon(CHECK);
    return factory_->NewBreakStatement(label, target, begin,
                                       lexer_.previous_end_position());
  }

//  ReturnStatement
//    : RETURN Expression_opt ';'
  Statement* ParseReturnStatement(bool *res) {
    assert(token_ == Token::RETURN);
    const std::size_t begin = lexer_.begin_position();
    Next();

    if (scope_->IsGlobal()) {
      // return statement found in global
      // SyntaxError
      RAISE("\"return\" not in function");
    }

    if (lexer_.has_line_terminator_before_next() ||
        token_ == Token::SEMICOLON ||
        token_ == Token::RBRACE ||
        token_ == Token::EOS) {
      ExpectSemicolon(CHECK);
      return factory_->NewReturnStatement(NULL,
                                          begin,
                                          lexer_.previous_end_position());
    }
    Expression* const expr = ParseExpression(true, CHECK);
    ExpectSemicolon(CHECK);
    return factory_->NewReturnStatement(expr, begin,
                                        lexer_.previous_end_position());
  }

//  WithStatement
//    : WITH '(' Expression ')' Statement
  Statement* ParseWithStatement(bool *res) {
    assert(token_ == Token::WITH);
    const std::size_t begin = lexer_.begin_position();
    Next();

    // section 12.10.1
    // when in strict mode code, WithStatement is not allowed.
    if (strict_) {
      RAISE("with statement not allowed in strict code");
    }

    EXPECT(Token::LPAREN);

    Expression *expr = ParseExpression(true, CHECK);

    EXPECT(Token::RPAREN);

    Statement *stmt = ParseStatement(CHECK);
    assert(expr && stmt);
    return factory_->NewWithStatement(expr, stmt, begin);
  }

//  SwitchStatement
//    : SWITCH '(' Expression ')' CaseBlock
//
//  CaseBlock
//    : '{' CaseClauses_opt '}'
//    | '{' CaseClauses_opt DefaultClause CaseClauses_opt '}'
  Statement* ParseSwitchStatement(bool *res) {
    assert(token_ == Token::SWITCH);
    const std::size_t begin = lexer_.begin_position();
    CaseClause* case_clause;
    Next();

    EXPECT(Token::LPAREN);

    Expression* expr = ParseExpression(true, CHECK);
    CaseClauses* clauses = factory_->template NewVector<CaseClause*>();
    Target target(this, Target::kSwitchStatement);

    EXPECT(Token::RPAREN);

    EXPECT(Token::LBRACE);

    bool default_found = false;
    while (token_ != Token::RBRACE) {
      if (token_ == Token::CASE ||
          token_ == Token::DEFAULT) {
        case_clause = ParseCaseClause(CHECK);
      } else {
        UNEXPECT(token_);
      }
      if (case_clause->IsDefault()) {
        if (default_found) {
          RAISE("duplicate default clause in switch");
        } else {
          default_found = true;
        }
      }
      clauses->push_back(case_clause);
    }
    Next();
    assert(expr && clauses);
    SwitchStatement* const switch_stmt =
        factory_->NewSwitchStatement(expr, clauses,
                                     begin,
                                     lexer_.previous_end_position());
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
    assert(token_ == Token::CASE || token_ == Token::DEFAULT);
    const std::size_t begin = lexer_.begin_position();
    Expression* expr = NULL;
    Statements* const body = factory_->template NewVector<Statement*>();

    if (token_ == Token::CASE) {
      Next();
      expr = ParseExpression(true, CHECK);
    } else  {
      EXPECT(Token::DEFAULT);
    }

    EXPECT(Token::COLON);

    while (token_ != Token::RBRACE &&
           token_ != Token::CASE   &&
           token_ != Token::DEFAULT) {
      Statement* const stmt = ParseStatement(CHECK);
      body->push_back(stmt);
    }

    assert(body);
    return factory_->NewCaseClause(expr == NULL,
                                   expr, body,
                                   begin, lexer_.previous_end_position());
  }

//  ThrowStatement
//    : THROW Expression ';'
  Statement* ParseThrowStatement(bool *res) {
    assert(token_ == Token::THROW);
    const std::size_t begin = lexer_.begin_position();
    Next();
    // Throw requires Expression
    if (lexer_.has_line_terminator_before_next()) {
      RAISE("missing expression between throw and newline");
    }
    Expression* const expr = ParseExpression(true, CHECK);
    ExpectSemicolon(CHECK);
    assert(expr);
    return factory_->NewThrowStatement(expr,
                                       begin, lexer_.previous_end_position());
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
    assert(token_ == Token::TRY);
    const std::size_t begin = lexer_.begin_position();
    Identifier* name = NULL;
    Block* catch_block = NULL;
    Block* finally_block = NULL;
    bool has_catch_or_finally = false;

    Next();

    IS(Token::LBRACE);
    Block* const try_block = ParseBlock(CHECK);

    if (token_ == Token::CATCH) {
      // Catch
      has_catch_or_finally = true;
      Next();
      EXPECT(Token::LPAREN);
      IS(Token::IDENTIFIER);
      name = ParseIdentifier(lexer_.Buffer());
      // section 12.14.1
      // within the strict code, Identifier must not be "eval" or "arguments"
      if (strict_) {
        const EvalOrArguments val = IsEvalOrArguments(name);
        if (val) {
          if (val == kEval) {
            RAISE("catch placeholder \"eval\" not allowed in strict code");
          } else {
            assert(val == kArguments);
            RAISE(
                "catch placeholder \"arguments\" not allowed in strict code");
          }
        }
      }
      EXPECT(Token::RPAREN);
      IS(Token::LBRACE);
      catch_block = ParseBlock(CHECK);
    }

    if (token_ == Token::FINALLY) {
      // Finally
      has_catch_or_finally= true;
      Next();
      IS(Token::LBRACE);
      finally_block = ParseBlock(CHECK);
    }

    if (!has_catch_or_finally) {
      RAISE("missing catch or finally after try statement");
    }

    assert(try_block);
    return factory_->NewTryStatement(try_block,
                                     name, catch_block,
                                     finally_block, begin);
  }

//  DebuggerStatement
//    : DEBUGGER ';'
  Statement* ParseDebuggerStatement(bool *res) {
    assert(token_ == Token::DEBUGGER);
    const std::size_t begin = lexer_.begin_position();
    Next();
    ExpectSemicolon(CHECK);
    return factory_->NewDebuggerStatement(begin,
                                          lexer_.previous_end_position());
  }

  Statement* ParseExpressionStatement(bool *res) {
    Expression* const expr = ParseExpression(true, CHECK);
    ExpectSemicolon(CHECK);
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
    assert(token_ == Token::IDENTIFIER);
    Expression* const expr = ParseExpression(true, CHECK);
    if (token_ == Token::COLON &&
        expr->AsIdentifier()) {
      // LabelledStatement
      Next();

      Identifiers* labels = labels_;
      Identifier* const label = expr->AsIdentifier();
      const bool exist_labels = labels;
      if (!exist_labels) {
        labels = factory_->template NewVector<Identifier*>();
      }
      if (ContainsLabel(labels, label) || TargetsContainsLabel(label)) {
        // duplicate label
        RAISE("duplicate label");
      }
      labels->push_back(label);
      const LabelSwitcher label_switcher(this, labels, exist_labels);

      Statement* const stmt = ParseStatement(CHECK);
      assert(expr && stmt);
      return factory_->NewLabelledStatement(expr, stmt);
    }
    ExpectSemicolon(CHECK);
    assert(expr);
    return factory_->NewExpressionStatement(expr,
                                            lexer_.previous_end_position());
  }

  template<bool Use>
  Statement* ParseFunctionStatement(bool *res,
                                    typename enable_if_c<Use>::type* = 0) {
    assert(token_ == Token::FUNCTION);
    if (strict_) {
      RAISE("function statement not allowed in strict code");
    }
    FunctionLiteral* const expr = ParseFunctionLiteral(
        FunctionLiteral::STATEMENT,
        FunctionLiteral::GENERAL,
        CHECK);
    // define named function as variable declaration
    assert(expr);
    assert(expr->name());
    scope_->AddUnresolved(expr->name().Address(), false);
    return factory_->NewFunctionStatement(expr);
  }

  template<bool Use>
  Statement* ParseFunctionStatement(bool *res,
                                    typename enable_if_c<!Use>::type* = 0) {
    // FunctionStatement is not Standard
    // so, if template parameter UseFunctionStatement is false,
    // this parser reject FunctionStatement
    RAISE("function statement is not Standard");
  }

//  Expression
//    : AssignmentExpression
//    | Expression ',' AssignmentExpression
  Expression* ParseExpression(bool contains_in, bool *res) {
    Expression* right;
    Expression* result = ParseAssignmentExpression(contains_in, CHECK);
    while (token_ == Token::COMMA) {
      Next();
      right = ParseAssignmentExpression(contains_in, CHECK);
      assert(result && right);
      result = factory_->NewBinaryOperation(Token::COMMA, result, right);
    }
    return result;
  }

//  AssignmentExpression
//    : ConditionalExpression
//    | LeftHandSideExpression AssignmentOperator AssignmentExpression
  Expression* ParseAssignmentExpression(bool contains_in, bool *res) {
    Expression* const result = ParseConditionalExpression(contains_in, CHECK);
    if (!Token::IsAssignOp(token_)) {
      return result;
    }
    if (!result->IsValidLeftHandSide()) {
      RAISE("invalid left-hand-side in assignment");
    }
    // section 11.13.1 throwing SyntaxError
    if (strict_ &&
        result->AsIdentifier()) {
      const EvalOrArguments val = IsEvalOrArguments(result->AsIdentifier());
      if (val) {
        if (val == kEval) {
          RAISE("assignment to \"eval\" not allowed in strict code");
        } else {
          assert(val == kArguments);
          RAISE("assignment to \"arguments\" not allowed in strict code");
        }
      }
    }
    const Token::Type op = token_;
    Next();
    Expression* const right = ParseAssignmentExpression(contains_in, CHECK);
    assert(result && right);
    return factory_->NewAssignment(op, result, right);
  }

//  ConditionalExpression
//    : LogicalOrExpression
//    | LogicalOrExpression '?' AssignmentExpression ':' AssignmentExpression
  Expression* ParseConditionalExpression(bool contains_in, bool *res) {
    Expression* result = ParseBinaryExpression(contains_in, 9, CHECK);
    if (token_ == Token::CONDITIONAL) {
      Next();
      // see ECMA-262 section 11.12
      Expression* const left = ParseAssignmentExpression(true, CHECK);
      EXPECT(Token::COLON);
      Expression* const right = ParseAssignmentExpression(contains_in, CHECK);
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
  Expression* ParseBinaryExpression(bool contains_in,
                                    int prec, bool *res) {
    Expression *left, *right;
    Token::Type op;
    left = ParseUnaryExpression(CHECK);
    // MultiplicativeExpression
    while (token_ == Token::MUL ||
           token_ == Token::DIV ||
           token_ == Token::MOD) {
      op = token_;
      Next();
      right = ParseUnaryExpression(CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 1) return left;

    // AdditiveExpression
    while (token_ == Token::ADD ||
           token_ == Token::SUB) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 0, CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 2) return left;

    // ShiftExpression
    while (token_ == Token::SHL ||
           token_ == Token::SAR ||
           token_ == Token::SHR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 1, CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 3) return left;

    // RelationalExpression
    while ((Token::REL_FIRST < token_ &&
            token_ < Token::REL_LAST) ||
           (contains_in && token_ == Token::IN)) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 2, CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    if (prec < 4) return left;

    // EqualityExpression
    while (token_ == Token::EQ_STRICT ||
           token_ == Token::NE_STRICT ||
           token_ == Token::EQ ||
           token_ == Token::NE) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 3, CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    if (prec < 5) return left;

    // BitwiseAndExpression
    while (token_ == Token::BIT_AND) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 4, CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 6) return left;

    // BitwiseXorExpression
    while (token_ == Token::BIT_XOR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 5, CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 7) return left;

    // BitwiseOrExpression
    while (token_ == Token::BIT_OR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 6, CHECK);
      left = ReduceBinaryOperation<ReduceExpressions>(op, left, right);
    }
    if (prec < 8) return left;

    // LogicalAndExpression
    while (token_ == Token::LOGICAL_AND) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 7, CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    if (prec < 9) return left;

    // LogicalOrExpression
    while (token_ == Token::LOGICAL_OR) {
      op = token_;
      Next();
      right = ParseBinaryExpression(contains_in, 8, CHECK);
      assert(left && right);
      left = factory_->NewBinaryOperation(op, left, right);
    }
    return left;
  }

  template<bool Reduce>
  Expression* ReduceBinaryOperation(Token::Type op,
                                    Expression* left,
                                    Expression* right,
                                    typename enable_if_c<Reduce>::type* = 0) {
    if (left->AsNumberLiteral() &&
        right->AsNumberLiteral()) {
      const double l_val = left->AsNumberLiteral()->value();
      const double r_val = right->AsNumberLiteral()->value();
      Expression* res;
      switch (op) {
        case Token::ADD:
          res = factory_->NewReducedNumberLiteral(l_val + r_val);
          break;

        case Token::SUB:
          res = factory_->NewReducedNumberLiteral(l_val - r_val);
          break;

        case Token::MUL:
          res = factory_->NewReducedNumberLiteral(l_val * r_val);
          break;

        case Token::DIV:
          res = factory_->NewReducedNumberLiteral(l_val / r_val);
          break;

        case Token::BIT_OR:
          res = factory_->NewReducedNumberLiteral(
              DoubleToInt32(l_val) | DoubleToInt32(r_val));
          break;

        case Token::BIT_AND:
          res = factory_->NewReducedNumberLiteral(
              DoubleToInt32(l_val) & DoubleToInt32(r_val));
          break;

        case Token::BIT_XOR:
          res = factory_->NewReducedNumberLiteral(
              DoubleToInt32(l_val) ^ DoubleToInt32(r_val));
          break;

        // section 11.7 Bitwise Shift Operators
        case Token::SHL: {
          const int32_t value = DoubleToInt32(l_val)
              << (DoubleToInt32(r_val) & 0x1f);
          res = factory_->NewReducedNumberLiteral(value);
          break;
        }

        case Token::SHR: {
          const uint32_t shift = DoubleToInt32(r_val) & 0x1f;
          const uint32_t value = DoubleToUInt32(l_val) >> shift;
          res = factory_->NewReducedNumberLiteral(value);
          break;
        }

        case Token::SAR: {
          uint32_t shift = DoubleToInt32(r_val) & 0x1f;
          int32_t value = DoubleToInt32(l_val) >> shift;
          res = factory_->NewReducedNumberLiteral(value);
          break;
        }

        default:
          assert(left && right);
          res = factory_->NewBinaryOperation(op, left, right);
          break;
      }
      return res;
    } else {
      assert(left && right);
      return factory_->NewBinaryOperation(op, left, right);
    }
  }

  template<bool Reduce>
  Expression* ReduceBinaryOperation(Token::Type op,
                                    Expression* left,
                                    Expression* right,
                                    typename enable_if_c<!Reduce>::type* = 0) {
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
    switch (token_) {
      case Token::VOID:
      case Token::NOT:
      case Token::TYPEOF:
        Next();
        expr = ParseUnaryExpression(CHECK);
        assert(expr);
        result = factory_->NewUnaryOperation(op, expr, begin);
        break;

      case Token::DELETE:
        // a strict mode restriction in sec 11.4.1
        // raise SyntaxError when target is direct reference to a variable,
        // function argument, or function name
        Next();
        expr = ParseUnaryExpression(CHECK);
        if (strict_ &&
            expr->AsIdentifier()) {
          RAISE("delete to direct identifier not allowed in strict code");
        }
        assert(expr);
        result = factory_->NewUnaryOperation(op, expr, begin);
        break;

      case Token::BIT_NOT:
        Next();
        expr = ParseUnaryExpression(CHECK);
        if (ReduceExpressions && expr->AsNumberLiteral()) {
          result = factory_->NewReducedNumberLiteral(
              ~DoubleToInt32(expr->AsNumberLiteral()->value()));
        } else {
          assert(expr);
          result = factory_->NewUnaryOperation(op, expr, begin);
        }
        break;

      case Token::ADD:
        Next();
        expr = ParseUnaryExpression(CHECK);
        if (expr->AsNumberLiteral()) {
          result = expr;
        } else {
          assert(expr);
          result = factory_->NewUnaryOperation(op, expr, begin);
        }
        break;

      case Token::SUB:
        Next();
        expr = ParseUnaryExpression(CHECK);
        if (ReduceExpressions && expr->AsNumberLiteral()) {
          result = factory_->NewReducedNumberLiteral(
              -(expr->AsNumberLiteral()->value()));
        } else {
          assert(expr);
          result = factory_->NewUnaryOperation(op, expr, begin);
        }
        break;

      case Token::INC:
      case Token::DEC:
        Next();
        expr = ParseMemberExpression(true, CHECK);
        if (!expr->IsValidLeftHandSide()) {
          RAISE("invalid left-hand-side in prefix expression");
        }
        // section 11.4.4, 11.4.5 throwing SyntaxError
        if (strict_ &&
            expr->AsIdentifier()) {
          const EvalOrArguments val = IsEvalOrArguments(expr->AsIdentifier());
          if (val) {
            if (val == kEval) {
              RAISE("prefix expression to \"eval\" "
                    "not allowed in strict code");
            } else {
              assert(val == kArguments);
              RAISE("prefix expression to \"arguments\" "
                    "not allowed in strict code");
            }
          }
        }
        assert(expr);
        result = factory_->NewUnaryOperation(op, expr, begin);
        break;

      default:
        result = ParsePostfixExpression(CHECK);
        break;
    }
    return result;
  }

//  PostfixExpression
//    : LeftHandSideExpression
//    | LeftHandSideExpression INCREMENT
//    | LeftHandSideExpression DECREMENT
  Expression* ParsePostfixExpression(bool *res) {
    Expression* expr = ParseMemberExpression(true, CHECK);
    if (!lexer_.has_line_terminator_before_next() &&
        (token_ == Token::INC || token_ == Token::DEC)) {
      if (!expr->IsValidLeftHandSide()) {
        RAISE("invalid left-hand-side in postfix expression");
      }
      // section 11.3.1, 11.3.2 throwing SyntaxError
      if (strict_ &&
          expr->AsIdentifier()) {
        const EvalOrArguments val = IsEvalOrArguments(expr->AsIdentifier());
        if (val) {
          if (val == kEval) {
            RAISE("postfix expression to \"eval\" not allowed in strict code");
          } else {
            assert(val == kArguments);
            RAISE("postfix expression to \"arguments\" "
                  "not allowed in strict code");
          }
        }
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
//    | FunctionExpression
//    | MemberExpression '[' Expression ']'
//    | NEW MemberExpression Arguments
  Expression* ParseMemberExpression(bool allow_call, bool *res) {
    Expression* expr;
    if (token_ != Token::NEW) {
      if (token_ == Token::FUNCTION) {
        // FunctionExpression
        expr = ParseFunctionLiteral(FunctionLiteral::EXPRESSION,
                                    FunctionLiteral::GENERAL, CHECK);
      } else {
        expr = ParsePrimaryExpression(CHECK);
      }
    } else {
      Next();
      Expression* const target = ParseMemberExpression(false, CHECK);
      Expressions* const args = factory_->template NewVector<Expression*>();
      if (token_ == Token::LPAREN) {
        ParseArguments(args, CHECK);
      }
      assert(target && args);
      expr = factory_->NewConstructorCall(target, args, lexer_.previous_end_position());
    }
    while (true) {
      switch (token_) {
        case Token::LBRACK: {
          Next();
          Expression* const index = ParseExpression(true, CHECK);
          assert(expr && index);
          expr = factory_->NewIndexAccess(expr, index);
          EXPECT(Token::RBRACK);
          break;
        }

        case Token::PERIOD: {
          Next<IgnoreReservedWords>();  // IDENTIFIERNAME
          IS(Token::IDENTIFIER);
          Identifier* const ident = ParseIdentifier(lexer_.Buffer());
          assert(expr && ident);
          expr = factory_->NewIdentifierAccess(expr, ident);
          break;
        }

        case Token::LPAREN:
          if (allow_call) {
            Expressions* const args =
                factory_->template NewVector<Expression*>();
            ParseArguments(args, CHECK);
            assert(expr && args);
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
    Expression* result = NULL;
    switch (token_) {
      case Token::THIS:
        result = factory_->NewThisLiteral(lexer_.begin_position(),
                                          lexer_.end_position());
        Next();
        break;

      case Token::IDENTIFIER:
        result = ParseIdentifier(lexer_.Buffer());
        break;

      case Token::NULL_LITERAL:
        result = factory_->NewNullLiteral(lexer_.begin_position(),
                                          lexer_.end_position());
        Next();
        break;

      case Token::TRUE_LITERAL:
        result = factory_->NewTrueLiteral(lexer_.begin_position(),
                                          lexer_.end_position());
        Next();
        break;

      case Token::FALSE_LITERAL:
        result = factory_->NewFalseLiteral(lexer_.begin_position(),
                                           lexer_.end_position());
        Next();
        break;

      case Token::NUMBER:
        // section 7.8.3
        // strict mode forbids Octal Digits Literal
        if (strict_ && lexer_.NumericType() == lexer_type::OCTAL) {
          RAISE("octal integer literal not allowed in strict code");
        }
        result = factory_->NewNumberLiteral(lexer_.Numeric(),
                                            lexer_.begin_position(),
                                            lexer_.end_position());
        Next();
        break;

      case Token::STRING: {
        const typename lexer_type::State state = lexer_.StringEscapeType();
        if (strict_ && state == lexer_type::OCTAL) {
          RAISE("octal excape sequence not allowed in strict code");
        }
        if (state == lexer_type::NONE) {
          result = factory_->NewDirectivable(lexer_.Buffer(),
                                             lexer_.begin_position(),
                                             lexer_.end_position());
        } else {
          result = factory_->NewStringLiteral(lexer_.Buffer(),
                                              lexer_.begin_position(),
                                              lexer_.end_position());
        }
        Next();
        break;
      }

      case Token::DIV:
        result = ParseRegExpLiteral(false, CHECK);
        break;

      case Token::ASSIGN_DIV:
        result = ParseRegExpLiteral(true, CHECK);
        break;

      case Token::LBRACK:
        result = ParseArrayLiteral(CHECK);
        break;

      case Token::LBRACE:
        result = ParseObjectLiteral(CHECK);
        break;

      case Token::LPAREN:
        Next();
        result = ParseExpression(true, CHECK);
        EXPECT(Token::RPAREN);
        break;

      default:
        UNEXPECT(token_);
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
    Next();
    if (token_ != Token::RPAREN) {
      Expression* const first = ParseAssignmentExpression(true, CHECK);
      container->push_back(first);
      while (token_ == Token::COMMA) {
        Next();
        Expression* const expr = ParseAssignmentExpression(true, CHECK);
        container->push_back(expr);
      }
    }
    EXPECT(Token::RPAREN);
    return container;
  }

  Expression* ParseRegExpLiteral(bool contains_eq, bool *res) {
    if (lexer_.ScanRegExpLiteral(contains_eq)) {
      const std::vector<uc16> content(lexer_.Buffer());
      if (!lexer_.ScanRegExpFlags()) {
        RAISE("invalid regular expression flag");
      }
      RegExpLiteral* const expr = factory_->NewRegExpLiteral(
          content, lexer_.Buffer(),
          lexer_.begin_position(),
          lexer_.end_position());
      if (!expr) {
        RAISE("invalid regular expression");
      }
      Next();
      return expr;
    } else {
      RAISE("invalid regular expression");
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
    const std::size_t begin = lexer_.begin_position();
    MaybeExpressions* const items = factory_->template NewVector<Maybe<Expression> >();
    Next();
    while (token_ != Token::RBRACK) {
      if (token_ == Token::COMMA) {
        // when Token::COMMA, only increment length
        items->push_back(Maybe<Expression>());
      } else {
        Expression* const expr = ParseAssignmentExpression(true, CHECK);
        items->push_back(expr);
      }
      if (token_ != Token::RBRACK) {
        EXPECT(Token::COMMA);
      }
    }
    Next();
    assert(items);
    return factory_->NewArrayLiteral(items,
                                     begin, lexer_.previous_end_position());
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
    typedef std::tr1::unordered_map<IdentifierKey, int> ObjectMap;
    typedef typename ObjectLiteral::Property Property;
    typedef typename ObjectLiteral::Properties Properties;
    const std::size_t begin = lexer_.begin_position();
    Properties* const prop = factory_->template NewVector<Property>();
    ObjectMap map;
    Expression* expr;
    Identifier* ident;

    // IDENTIFIERNAME
    Next<IgnoreReservedWordsAndIdentifyGetterOrSetter>();
    while (token_ != Token::RBRACE) {
      if (token_ == Token::GET || token_ == Token::SET) {
        const bool is_get = token_ == Token::GET;
        // this is getter or setter or usual prop
        Next<IgnoreReservedWords>();  // IDENTIFIERNAME
        if (token_ == Token::COLON) {
          // property
          ident = ParseIdentifierWithPosition(
              is_get ? ParserData::kGet : ParserData::kSet,
              lexer_.previous_begin_position(),
              lexer_.previous_end_position());
          expr = ParseAssignmentExpression(true, CHECK);
          ObjectLiteral::AddDataProperty(prop, ident, expr);
          typename ObjectMap::iterator it = map.find(ident);
          if (it == map.end()) {
            map.insert(std::make_pair(ident, ObjectLiteral::DATA));
          } else {
            if (it->second != ObjectLiteral::DATA) {
              RAISE("accessor property and data property "
                    "exist with the same name");
            } else {
              if (strict_) {
                RAISE("multiple data property assignments "
                      "with the same name not allowed in strict code");
              }
            }
          }
        } else {
          // getter or setter
          if (token_ == Token::IDENTIFIER ||
              token_ == Token::STRING ||
              token_ == Token::NUMBER) {
            if (token_ == Token::NUMBER) {
              ident = ParseIdentifierNumber(CHECK);
            } else if (token_ == Token::STRING) {
              ident = ParseIdentifierString(CHECK);
            } else {
              ident = ParseIdentifier(lexer_.Buffer());
            }
            typename ObjectLiteral::PropertyDescriptorType type =
                (is_get) ? ObjectLiteral::GET : ObjectLiteral::SET;
            expr = ParseFunctionLiteral(
                FunctionLiteral::EXPRESSION,
                (is_get) ? FunctionLiteral::GETTER : FunctionLiteral::SETTER,
                CHECK);
            ObjectLiteral::AddAccessor(prop, type, ident, expr);
            typename ObjectMap::iterator it = map.find(ident);
            if (it == map.end()) {
              map.insert(std::make_pair(ident, type));
            } else if (it->second & (ObjectLiteral::DATA | type)) {
              if (it->second & ObjectLiteral::DATA) {
                RAISE("data property and accessor property "
                      "exist with the same name");
              } else {
                RAISE("multiple same accessor properties "
                      "exist with the same name");
              }
            } else {
              it->second |= type;
            }
          } else {
            RAISE_RECOVERVABLE("invalid property name");
          }
        }
      } else if (token_ == Token::IDENTIFIER ||
                 token_ == Token::STRING ||
                 token_ == Token::NUMBER) {
        if (token_ == Token::NUMBER) {
          ident = ParseIdentifierNumber(CHECK);
        } else if (token_ == Token::STRING) {
          ident = ParseIdentifierString(CHECK);
        } else {
          ident = ParseIdentifier(lexer_.Buffer());
        }
        EXPECT(Token::COLON);
        expr = ParseAssignmentExpression(true, CHECK);
        ObjectLiteral::AddDataProperty(prop, ident, expr);
        typename ObjectMap::iterator it = map.find(ident);
        if (it == map.end()) {
          map.insert(std::make_pair(ident, ObjectLiteral::DATA));
        } else {
          if (it->second != ObjectLiteral::DATA) {
            RAISE("accessor property and data property "
                  "exist with the same name");
          } else {
            if (strict_) {
              RAISE("multiple data property assignments "
                    "with the same name not allowed in strict code");
            }
          }
        }
      } else {
        RAISE_RECOVERVABLE("invalid property name");
      }

      if (token_ != Token::RBRACE) {
        IS(Token::COMMA);
        // IDENTIFIERNAME
        Next<IgnoreReservedWordsAndIdentifyGetterOrSetter>();
      }
    }
    const std::size_t end = lexer_.begin_position();
    Next();
    assert(prop);
    return factory_->NewObjectLiteral(prop, begin, end);
  }

  FunctionLiteral* ParseFunctionLiteral(
      typename FunctionLiteral::DeclType decl_type,
      typename FunctionLiteral::ArgType arg_type,
      bool *res) {
    // IDENTIFIER
    // IDENTIFIER_opt
    std::tr1::unordered_set<IdentifierKey> param_set;
    std::size_t throw_error_if_strict_code_line = 0;
    const std::size_t begin_position = lexer_.begin_position();
    enum {
      kDetectNone = 0,
      kDetectEvalName,
      kDetectArgumentsName,
      kDetectEvalParameter,
      kDetectArgumentsParameter,
      kDetectDuplicateParameter,
      kDetectFutureReservedWords
    } throw_error_if_strict_code = kDetectNone;

    Identifiers* const params = factory_->template NewVector<Identifier*>();
    Identifier* name = NULL;

    if (arg_type == FunctionLiteral::GENERAL) {
      assert(token_ == Token::FUNCTION);
      Next(true);  // preparing for strict directive
      const Token::Type current = token_;
      if (current == Token::IDENTIFIER ||
          Token::IsAddedFutureReservedWordInStrictCode(current)) {
        name = ParseIdentifier(lexer_.Buffer());
        if (Token::IsAddedFutureReservedWordInStrictCode(current)) {
          throw_error_if_strict_code = kDetectFutureReservedWords;
          throw_error_if_strict_code_line = lexer_.line_number();
        } else {
          assert(current == Token::IDENTIFIER);
          const EvalOrArguments val = IsEvalOrArguments(name);
          if (val) {
            throw_error_if_strict_code = (val == kEval) ?
                kDetectEvalName : kDetectArgumentsName;
            throw_error_if_strict_code_line = lexer_.line_number();
          }
        }
      } else if (decl_type == FunctionLiteral::DECLARATION ||
                 decl_type == FunctionLiteral::STATEMENT) {
        IS(Token::IDENTIFIER);
      }
    }

    const std::size_t begin_block_position = lexer_.begin_position();

    //  '(' FormalParameterList_opt ')'
    IS(Token::LPAREN);
    Next(true);  // preparing for strict directive

    if (arg_type == FunctionLiteral::GETTER) {
      // if getter, parameter count is 0
      EXPECT(Token::RPAREN);
    } else if (arg_type == FunctionLiteral::SETTER) {
      // if setter, parameter count is 1
      const Token::Type current = token_;
      if (current != Token::IDENTIFIER &&
          !Token::IsAddedFutureReservedWordInStrictCode(current)) {
        IS(Token::IDENTIFIER);
      }
      Identifier* const ident = ParseIdentifier(lexer_.Buffer());
      if (!throw_error_if_strict_code) {
        if (Token::IsAddedFutureReservedWordInStrictCode(current)) {
          throw_error_if_strict_code = kDetectFutureReservedWords;
          throw_error_if_strict_code_line = lexer_.line_number();
        } else {
          assert(current == Token::IDENTIFIER);
          const EvalOrArguments val = IsEvalOrArguments(ident);
          if (val) {
            throw_error_if_strict_code = (val == kEval) ?
                kDetectEvalName : kDetectArgumentsName;
            throw_error_if_strict_code_line = lexer_.line_number();
          }
        }
      }
      params->push_back(ident);
      EXPECT(Token::RPAREN);
    } else {
      if (token_ != Token::RPAREN) {
        do {
          const Token::Type current = token_;
          if (current != Token::IDENTIFIER &&
              !Token::IsAddedFutureReservedWordInStrictCode(current)) {
            IS(Token::IDENTIFIER);
          }
          Identifier* const ident = ParseIdentifier(lexer_.Buffer());
          if (!throw_error_if_strict_code) {
            if (Token::IsAddedFutureReservedWordInStrictCode(current)) {
              throw_error_if_strict_code = kDetectFutureReservedWords;
              throw_error_if_strict_code_line = lexer_.line_number();
            } else {
              assert(current == Token::IDENTIFIER);
              const EvalOrArguments val = IsEvalOrArguments(ident);
              if (val) {
                throw_error_if_strict_code = (val == kEval) ?
                    kDetectEvalName : kDetectArgumentsName;
                throw_error_if_strict_code_line = lexer_.line_number();
              }
            }
            if ((!throw_error_if_strict_code) &&
                (param_set.find(ident) != param_set.end())) {
              throw_error_if_strict_code = kDetectDuplicateParameter;
              throw_error_if_strict_code_line = lexer_.line_number();
            }
          }
          params->push_back(ident);
          param_set.insert(ident);
          if (token_ == Token::COMMA) {
            Next(true);
          } else {
            break;
          }
        } while (true);
      }
      EXPECT(Token::RPAREN);
    }

    //  '{' FunctionBody '}'
    //
    //  FunctionBody
    //    :
    //    | SourceElements
    EXPECT(Token::LBRACE);

    Statements* const body = factory_->template NewVector<Statement*>();
    Scope* const scope = factory_->NewScope(decl_type);
    const ScopeSwitcher scope_switcher(this, scope);
    const TargetSwitcher target_switcher(this);
    const bool function_is_strict =
        ParseSourceElements(Token::RBRACE, body, CHECK);
    if (strict_ || function_is_strict) {
      // section 13.1
      // Strict Mode Restrictions
      switch (throw_error_if_strict_code) {
        case kDetectNone:
          break;
        case kDetectEvalName:
          RAISE_WITH_NUMBER(
              "function name \"eval\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectArgumentsName:
          RAISE_WITH_NUMBER(
              "function name \"arguments\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectEvalParameter:
          RAISE_WITH_NUMBER(
              "parameter \"eval\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectArgumentsParameter:
          RAISE_WITH_NUMBER(
              "parameter \"arguments\" not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectDuplicateParameter:
          RAISE_WITH_NUMBER(
              "duplicate parameter not allowed in strict code",
              throw_error_if_strict_code_line);
          break;
        case kDetectFutureReservedWords:
          RAISE_WITH_NUMBER(
              "FutureReservedWords is found in strict code",
              throw_error_if_strict_code_line);
          break;
      }
    }
    Next();
    const std::size_t end_block_position = lexer_.previous_end_position();
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
                                        end_block_position);
  }

  Identifier* ParseIdentifierNumber(bool* res) {
    if (strict_ && lexer_.NumericType() == lexer_type::OCTAL) {
      RAISE("octal integer literal not allowed in strict code");
    }
    const double val = lexer_.Numeric();
    std::tr1::array<char, 80> buf;
    DoubleToCString(val, buf.data(), 80);
    Identifier* const ident = factory_->NewIdentifier(
        Token::NUMBER,
        core::StringPiece(buf.data(), std::strlen(buf.data())),
        lexer_.begin_position(),
        lexer_.end_position());
    Next();
    return ident;
  }


  Identifier* ParseIdentifierString(bool* res) {
    if (strict_ && lexer_.StringEscapeType() == lexer_type::OCTAL) {
      RAISE("octal excape sequence not allowed in strict code");
    }
    Identifier* const ident = factory_->NewIdentifier(Token::STRING,
                                                      lexer_.Buffer(),
                                                      lexer_.begin_position(),
                                                      lexer_.end_position());
    Next();
    return ident;
  }

  template<typename Range>
  Identifier* ParseIdentifier(const Range& range) {
    Identifier* const ident = factory_->NewIdentifier(Token::IDENTIFIER,
                                                      range,
                                                      lexer_.begin_position(),
                                                      lexer_.end_position());
    Next();
    return ident;
  }

  template<typename Range>
  Identifier* ParseIdentifierWithPosition(const Range& range,
                                          std::size_t begin,
                                          std::size_t end) {
    Identifier* const ident = factory_->NewIdentifier(Token::IDENTIFIER,
                                                      range,
                                                      begin,
                                                      end);
    Next();
    return ident;
  }

  bool ContainsLabel(const Identifiers* const labels,
                     const Identifier * const label) const {
    assert(label != NULL);
    if (labels) {
      const typename Identifier::value_type& value = label->value();
      for (typename Identifiers::const_iterator it = labels->begin(),
           last = labels->end();
           it != last; ++it) {
        if ((*it)->value() == value) {
          return true;
        }
      }
    }
    return false;
  }

  bool TargetsContainsLabel(const Identifier* const label) const {
    assert(label != NULL);
    for (const Target* target = target_;
         target != NULL;
         target = target->previous()) {
      if (ContainsLabel(target->labels(), label)) {
        return true;
      }
    }
    return false;
  }

  BreakableStatement** LookupBreakableTarget(
      const Identifier* const label) const {
    assert(label != NULL);
    for (Target* target = target_;
         target != NULL;
         target = target->previous()) {
      if (ContainsLabel(target->labels(), label)) {
        return target->node();
      }
    }
    return NULL;
  }

  BreakableStatement** LookupBreakableTarget() const {
    for (Target* target = target_;
         target != NULL;
         target = target->previous()) {
      if (target->IsAnonymous()) {
        return target->node();
      }
    }
    return NULL;
  }

  IterationStatement** LookupContinuableTarget(
      const Identifier* const label) const {
    assert(label != NULL);
    for (Target* target = target_;
         target != NULL;
         target = target->previous()) {
      if (target->IsIteration() && ContainsLabel(target->labels(), label)) {
        return reinterpret_cast<IterationStatement**>(target->node());
      }
    }
    return NULL;
  }

  IterationStatement** LookupContinuableTarget() const {
    for (Target* target = target_;
         target != NULL;
         target = target->previous()) {
      if (target->IsIteration()) {
        return reinterpret_cast<IterationStatement**>(target->node());
      }
    }
    return NULL;
  }

  inline void SetErrorHeader() {
    SetErrorHeader(lexer_.line_number());
  }

  void SetErrorHeader(std::size_t line) {
    std::tr1::array<char, 40> buf;
    error_.append(lexer_.filename());
    const int num = std::snprintf(buf.data(), buf.size(),
                                  ":%lu: SyntaxError: ",
                                  static_cast<unsigned long>(line));  // NOLINT
    error_.append(buf.data(), num);
  }

  void ReportUnexpectedToken(Token::Type expected_token) {
    SetErrorHeader();
    switch (token_) {
      case Token::STRING:
        error_.append("unexpected string");
        break;
      case Token::NUMBER:
        error_.append("unexpected number");
        break;
      case Token::IDENTIFIER:
        error_.append("unexpected identifier");
        break;
      case Token::EOS:
        error_.append("unexpected EOS");
        break;
      case Token::ILLEGAL: {
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

  bool ExpectSemicolon(bool *res) {
    if (token_ == Token::SEMICOLON) {
      Next();
      return true;
    }
    if (lexer_.has_line_terminator_before_next() ||
        token_ == Token::RBRACE ||
        token_ == Token::EOS ) {
      return true;
    }
    UNEXPECT(token_);
  }

  inline lexer_type* lexer() const {
    return &lexer_;
  }
  template<typename LexType>
  inline Token::Type Next() {
    return token_ = lexer_.Next<LexType>(strict_);
  }
  inline Token::Type Next() {
    return token_ = lexer_.Next<IdentifyReservedWords>(strict_);
  }
  inline Token::Type Next(bool strict) {
    return token_ = lexer_.Next<IdentifyReservedWords>(strict);
  }
  inline Token::Type Peek() const {
    return token_;
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
  inline Identifiers* labels() const {
    return labels_;
  }
  inline void set_labels(Identifiers* labels) {
    labels_ = labels;
  }
  inline bool strict() const {
    return strict_;
  }
  inline void set_strict(bool strict) {
    strict_ = strict;
  }
  inline bool RecoverableError() const {
    return (!(error_state_ & kNotRecoverable)) && token_ == Token::EOS;
  }

 protected:
  class ScopeSwitcher : private Noncopyable<ScopeSwitcher>::type {
   public:
    ScopeSwitcher(parser_type* parser, Scope* scope)
      : parser_(parser) {
      scope->SetUpperScope(parser_->scope());
      parser_->set_scope(scope);
    }
    ~ScopeSwitcher() {
      assert(parser_->scope() != NULL);
      parser_->set_scope(parser_->scope()->GetUpperScope());
    }
   private:
    parser_type* parser_;
  };

  class LabelSwitcher : private Noncopyable<LabelSwitcher>::type {
   public:
    LabelSwitcher(parser_type* parser,
                  Identifiers* labels, bool exist_labels)
      : parser_(parser),
        exist_labels_(exist_labels) {
      parser_->set_labels(labels);
    }
    ~LabelSwitcher() {
      if (!exist_labels_) {
        parser_->set_labels(NULL);
      }
    }
   private:
    parser_type* parser_;
    bool exist_labels_;
  };

  class StrictSwitcher : private Noncopyable<StrictSwitcher>::type {
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

  enum EvalOrArguments {
    kNone = 0,
    kEval = 1,
    kArguments = 2
  };

  static EvalOrArguments IsEvalOrArguments(const Identifier* ident) {
    const SpaceUString& str = ident->value();
    if (str.compare(ParserData::kEval.data()) == 0) {
      return kEval;
    } else if (str.compare(ParserData::kArguments.data()) == 0) {
      return kArguments;
    } else {
      return kNone;
    }
  }

  lexer_type lexer_;
  Token::Type token_;
  std::string error_;
  bool strict_;
  int error_state_;
  Factory* factory_;
  Scope* scope_;
  Target* target_;
  Identifiers* labels_;
};
#undef IS
#undef EXPECT
#undef UNEXPECT
#undef RAISE
#undef RAISE_WITH_NUMBER
#undef CHECK
} }  // namespace iv::core
#endif  // _IV_PARSER_H_
