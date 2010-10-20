#include <algorithm>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <tr1/functional>
#include "utils.h"
#ifdef DEBUG
#include <cstdio>
#include <iostream>  // NOLINT
#endif  // DEBUG

#include "parser.h"
#include "utils.h"

#ifdef DEBUG

#define REPORT\
  std::printf("error line: %d\n", __LINE__);\
  std::printf("error source: %d\n", lexer_.line_number());

#define TRACE\
  std::printf("check: %d\n", __LINE__);

#else

#define REPORT
#define TRACE

#endif  // DEBUG

#define IS(token)\
  do {\
    if (token_ != token) {\
      *res = false;\
      ReportUnexpectedToken();\
      REPORT\
      return NULL;\
    }\
  } while (0)

#define EXPECT(token)\
  do {\
    if (token_ != token) {\
      *res = false;\
      ReportUnexpectedToken();\
      REPORT\
      return NULL;\
    }\
    Next();\
  } while (0)

#define FAIL() \
  do {\
    *res = false;\
    REPORT\
    return NULL;\
  } while (0)

#define CHECK  res);\
  if (!*res) {\
    TRACE\
    return NULL;\
  }\
  ((void)0
#define DUMMY )  // to make indentation work
#undef DUMMY

#define NEW(a) (new (space_) a)

namespace iv {
namespace core {
namespace {
const char * const use_strict_prefix = "use strict";
const char * const arguments_prefix = "arguments";
const char * const eval_prefix = "eval";
const UString use_strict_string(
    use_strict_prefix, use_strict_prefix+std::strlen(use_strict_prefix));
const UString arguments_string(
    arguments_prefix, arguments_prefix+std::strlen(arguments_prefix));
const UString eval_string(
    eval_prefix, eval_prefix+std::strlen(eval_prefix));
}  // namespace

Parser::Parser(Source* source, AstFactory* space)
  : lexer_(source),
    error_(),
    strict_(false),
    space_(space),
    scope_(NULL),
    target_(NULL),
    labels_(NULL),
    resolved_check_stack_() {
}

// Program
//   : SourceElements
FunctionLiteral* Parser::ParseProgram() {
  FunctionLiteral* global = space_->NewFunctionLiteral(FunctionLiteral::GLOBAL);
  assert(target_ == NULL);
  bool error_flag = true;
  bool *res = &error_flag;
  {
    const ScopeSwitcher switcher(this, global->scope());
    Next();
    ParseSourceElements(Token::EOS, global, CHECK);
  }
  if (!resolved_check_stack_.empty()) {
    return NULL;
  }
  return (error_flag) ? global : NULL;
}

// SourceElements
//   : SourceElement
//   | SourceElement SourceElements
//
// SourceElement
//   : Statements
//   | FunctionDeclaration
bool Parser::ParseSourceElements(Token::Type end,
                                 FunctionLiteral* function, bool *res) {
  Statement *stmt;
  bool recognize_use_strict_directive = true;
  const StrictSwitcher switcher(this);
  while (token_ != end) {
    if (token_ == Token::FUNCTION) {
      // FunctionDeclaration
      stmt = ParseFunctionDeclaration(CHECK);
      function->AddStatement(stmt);
    } else {
      stmt = ParseStatement(CHECK);
      // use strict directive check
      if (recognize_use_strict_directive &&
          !strict_ &&
          stmt->AsExpressionStatement()) {
        Expression* const expr = stmt->AsExpressionStatement()->expr();
        if (expr->AsLiteral()) {
          Literal* const literal = expr->AsLiteral();
          if (literal->AsStringLiteral()) {
            if (lexer_.StringEscapeType() == Lexer::NONE &&
                literal->AsStringLiteral()->value().compare(
                    use_strict_string.data()) == 0) {
              switcher.SwitchStrictMode();
              function->AddStatement(stmt);
              function->set_strict(true);
              continue;
            }
          }
        }
      }
      function->AddStatement(stmt);
    }
    recognize_use_strict_directive = false;
  }
  return true;
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
Statement* Parser::ParseStatement(bool *res) {
  Statement *result = NULL;
  switch (token_) {
    case Token::LBRACE:
      // Block
      result = ParseBlock(CHECK);
      break;

    case Token::CONST:
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
      result = ParseFunctionStatement(CHECK);
      break;

    case Token::ILLEGAL:
      FAIL();
      break;

    case Token::IDENTIFIER:
      // LabelledStatement or ExpressionStatement
      result = ParseExpressionOrLabelledStatement(CHECK);
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
Statement* Parser::ParseFunctionDeclaration(bool *res) {
  FunctionLiteral *expr;
  Next();
  IS(Token::IDENTIFIER);
  expr = ParseFunctionLiteral(FunctionLiteral::DECLARATION,
                              FunctionLiteral::GENERAL, true, CHECK);
  // define named function as FunctionDeclaration
  scope_->AddFunctionDeclaration(expr);
  return NEW(FunctionStatement(expr));
}

Statement* Parser::ParseFunctionStatement(bool *res) {
  FunctionLiteral *expr;
  Next();
  IS(Token::IDENTIFIER);
  expr = ParseFunctionLiteral(FunctionLiteral::STATEMENT,
                              FunctionLiteral::GENERAL, true, CHECK);
  // define named function as variable declaration
  scope_->AddUnresolved(expr->name(), false);
  return NEW(FunctionStatement(expr));
}

//  Block
//    : '{' '}'
//    | '{' StatementList '}'
//
//  StatementList
//    : Statement
//    | StatementList Statement
Block* Parser::ParseBlock(bool *res) {
  Block *block = NEW(Block(space_));
  Statement *stmt;
  Target target(this, block);

  Next();
  while (token_ != Token::RBRACE) {
    stmt = ParseStatement(CHECK);
    block->AddStatement(stmt);
  }
  Next();
  return block;
}

//  VariableStatement
//    : VAR VariableDeclarationList ';'
//    : CONST VariableDeclarationList ';'
Statement* Parser::ParseVariableStatement(bool *res) {
  VariableStatement* stmt = NEW(VariableStatement(token_, space_));
  ParseVariableDeclarations(stmt, true, CHECK);
  ExpectSemicolon(CHECK);
  return stmt;
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
Statement* Parser::ParseVariableDeclarations(VariableStatement* stmt,
                                             bool contains_in,
                                             bool *res) {
  Identifier *name;
  Expression *expr;
  Declaration *decl;

  do {
    Next();
    IS(Token::IDENTIFIER);
    name = space_->NewIdentifier(lexer_.Buffer());
    // section 12.2.1
    // within the strict code, Identifier must not be "eval" or "arguments"
    if (strict_ && IsEvalOrArguments(name)) {
      FAIL();
    }
    Next();

    if (token_ == Token::ASSIGN) {
      Next();
      // AssignmentExpression
      expr = ParseAssignmentExpression(contains_in, CHECK);
      decl = NEW(Declaration(name, expr));
    } else {
      // Undefined Expression
      decl = NEW(Declaration(name, space_->NewUndefined()));
    }
    stmt->AddDeclaration(decl);
    scope_->AddUnresolved(name, stmt->IsConst());
  } while (token_ == Token::COMMA);

  return stmt;
}

bool Parser::ExpectSemicolon(bool *res) {
  if (token_ == Token::SEMICOLON) {
    Next();
    return true;
  }
  if (lexer_.has_line_terminator_before_next() ||
      token_ == Token::RBRACE ||
      token_ == Token::EOS ) {
    return true;
  }
  FAIL();
}

//  EmptyStatement
//    : ';'
Statement* Parser::ParseEmptyStatement() {
  Next();
  return space_->NewEmptyStatement();
}

//  IfStatement
//    : IF '(' Expression ')' Statement ELSE Statement
//    | IF '(' Expression ')' Statement
Statement* Parser::ParseIfStatement(bool *res) {
  IfStatement *if_stmt = NULL;
  Statement* stmt;
  Next();

  EXPECT(Token::LPAREN);

  Expression *expr = ParseExpression(true, CHECK);

  EXPECT(Token::RPAREN);

  stmt = ParseStatement(CHECK);
  if_stmt = NEW(IfStatement(expr, stmt));
  if (token_ == Token::ELSE) {
    Next();
    stmt = ParseStatement(CHECK);
    if_stmt->SetElse(stmt);
  }
  return if_stmt;
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
Statement* Parser::ParseDoWhileStatement(bool *res) {
  //  DO Statement WHILE '(' Expression ')' ';'
  DoWhileStatement* dowhile = NEW(DoWhileStatement());
  Target target(this, dowhile);
  Next();

  Statement *stmt = ParseStatement(CHECK);
  dowhile->set_body(stmt);

  EXPECT(Token::WHILE);

  EXPECT(Token::LPAREN);

  Expression *expr = ParseExpression(true, CHECK);
  dowhile->set_cond(expr);

  EXPECT(Token::RPAREN);

  ExpectSemicolon(CHECK);
  return dowhile;
}

Statement* Parser::ParseWhileStatement(bool *res) {
  //  WHILE '(' Expression ')' Statement
  Next();

  EXPECT(Token::LPAREN);

  Expression *expr = ParseExpression(true, CHECK);
  WhileStatement* whilestmt = NEW(WhileStatement(expr));
  Target target(this, whilestmt);

  EXPECT(Token::RPAREN);

  Statement* stmt = ParseStatement(CHECK);
  whilestmt->set_body(stmt);

  return whilestmt;
}

Statement* Parser::ParseForStatement(bool *res) {
  //  FOR '(' ExpressionNoIn_opt ';' Expression_opt ';' Expression_opt ')'
  //  Statement
  //  FOR '(' VAR VariableDeclarationListNoIn ';'
  //          Expression_opt ';'
  //          Expression_opt ')'
  //          Statement
  //  FOR '(' LeftHandSideExpression IN Expression ')' Statement
  //  FOR '(' VAR VariableDeclarationNoIn IN Expression ')' Statement
  Next();

  EXPECT(Token::LPAREN);

  Statement *init = NULL;

  if (token_ != Token::SEMICOLON) {
    if (token_ == Token::VAR || token_ == Token::CONST) {
      VariableStatement *var = NEW(VariableStatement(token_, space_));
      ParseVariableDeclarations(var, false, CHECK);
      init = var;
      if (token_ == Token::IN) {
        // for in loop
        Next();
        const AstNode::Declarations& decls = var->decls();
        if (decls.size() != 1) {
          // ForInStatement requests VaraibleDeclarationNoIn (not List),
          // so check declarations' size is 1.
          FAIL();
        }
        Expression *enumerable = ParseExpression(true, CHECK);
        EXPECT(Token::RPAREN);
        ForInStatement* forstmt = NEW(ForInStatement(init, enumerable));
        Target target(this, forstmt);
        Statement *body = ParseStatement(CHECK);
        forstmt->set_body(body);
        return forstmt;
      }
    } else {
      Expression *init_expr = ParseExpression(false, CHECK);
      init = NEW(ExpressionStatement(init_expr));
      if (token_ == Token::IN) {
        // for in loop
        if (init_expr == NULL || !init_expr->IsValidLeftHandSide()) {
          FAIL();
        }
        Next();
        Expression *enumerable = ParseExpression(true, CHECK);
        EXPECT(Token::RPAREN);
        ForInStatement* forstmt = NEW(ForInStatement(init, enumerable));
        Target target(this, forstmt);
        Statement *body = ParseStatement(CHECK);
        forstmt->set_body(body);
        return forstmt;
      }
    }
  }

  // ordinary for loop
  EXPECT(Token::SEMICOLON);

  Expression *cond = NULL;
  if (token_ == Token::SEMICOLON) {
    // no cond expr
    Next();
  } else {
    cond = ParseExpression(true, CHECK);
    EXPECT(Token::SEMICOLON);
  }

  ExpressionStatement *next = NULL;
  if (token_ == Token::RPAREN) {
    Next();
  } else {
    Expression *next_expr = ParseExpression(true, CHECK);
    next = NEW(ExpressionStatement(next_expr));
    EXPECT(Token::RPAREN);
  }

  ForStatement *for_stmt = NEW(ForStatement());
  Target target(this, for_stmt);
  Statement *body = ParseStatement(CHECK);
  for_stmt->set_body(body);
  if (init) {
    for_stmt->SetInit(init);
  }
  if (cond) {
    for_stmt->SetCondition(cond);
  }
  if (next) {
    for_stmt->SetNext(next);
  }

  return for_stmt;
}

//  ContinueStatement
//    : CONTINUE Identifier_opt ';'
Statement* Parser::ParseContinueStatement(bool *res) {
  ContinueStatement *continue_stmt = NEW(ContinueStatement());
  Next();
  if (!lexer_.has_line_terminator_before_next() &&
      token_ != Token::SEMICOLON &&
      token_ != Token::RBRACE &&
      token_ != Token::EOS) {
    IS(Token::IDENTIFIER);
    Identifier* label = space_->NewIdentifier(lexer_.Buffer());
    continue_stmt->SetLabel(label);
    IterationStatement* target = LookupContinuableTarget(label);
    if (target) {
      continue_stmt->SetTarget(target);
    } else {
      FAIL();
    }
    Next();
  } else {
    IterationStatement* target = LookupContinuableTarget();
    if (target) {
      continue_stmt->SetTarget(target);
    } else {
      FAIL();
    }
  }
  ExpectSemicolon(CHECK);
  return continue_stmt;
}

//  BreakStatement
//    : BREAK Identifier_opt ';'
Statement* Parser::ParseBreakStatement(bool *res) {
  BreakStatement *break_stmt = NEW(BreakStatement());
  Next();
  if (!lexer_.has_line_terminator_before_next() &&
      token_ != Token::SEMICOLON &&
      token_ != Token::RBRACE &&
      token_ != Token::EOS) {
    // label
    IS(Token::IDENTIFIER);
    Identifier* label = space_->NewIdentifier(lexer_.Buffer());
    break_stmt->SetLabel(label);
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
      BreakableStatement* target = LookupBreakableTarget(label);
      if (target) {
        break_stmt->SetTarget(target);
      } else {
        FAIL();
      }
    }
    Next();
  } else {
    BreakableStatement* target = LookupBreakableTarget();
    if (target) {
      break_stmt->SetTarget(target);
    } else {
      FAIL();
    }
  }
  ExpectSemicolon(CHECK);
  return break_stmt;
}

//  ReturnStatement
//    : RETURN Expression_opt ';'
Statement* Parser::ParseReturnStatement(bool *res) {
  Next();
  if (lexer_.has_line_terminator_before_next() ||
      token_ == Token::SEMICOLON ||
      token_ == Token::RBRACE ||
      token_ == Token::EOS) {
    ExpectSemicolon(CHECK);
    return NEW(ReturnStatement(space_->NewUndefined()));
  }
  Expression *expr = ParseExpression(true, CHECK);
  ExpectSemicolon(CHECK);
  return NEW(ReturnStatement(expr));
}

//  WithStatement
//    : WITH '(' Expression ')' Statement
Statement* Parser::ParseWithStatement(bool *res) {
  Next();

  // section 12.10.1
  // when in strict mode code, WithStatement is not allowed.
  if (strict_) {
    FAIL();
  }

  EXPECT(Token::LPAREN);

  Expression *expr = ParseExpression(true, CHECK);

  EXPECT(Token::RPAREN);

  Statement *stmt = ParseStatement(CHECK);
  return NEW(WithStatement(expr, stmt));
}

//  SwitchStatement
//    : SWITCH '(' Expression ')' CaseBlock
//
//  CaseBlock
//    : '{' CaseClauses_opt '}'
//    | '{' CaseClauses_opt DefaultClause CaseClauses_opt '}'
Statement* Parser::ParseSwitchStatement(bool *res) {
  CaseClause *case_clause;
  Next();

  EXPECT(Token::LPAREN);

  Expression *expr = ParseExpression(true, CHECK);
  SwitchStatement *switch_stmt = NEW(SwitchStatement(expr, space_));
  Target target(this, switch_stmt);

  EXPECT(Token::RPAREN);

  EXPECT(Token::LBRACE);

  while (token_ != Token::RBRACE) {
    case_clause = ParseCaseClause(CHECK);
    switch_stmt->AddCaseClause(case_clause);
  }
  Next();

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
CaseClause* Parser::ParseCaseClause(bool *res) {
  CaseClause *clause = NEW(CaseClause(space_));
  Statement *stmt;

  if (token_ == Token::CASE) {
    Next();
    Expression *expr = ParseExpression(true, CHECK);
    clause->SetExpression(expr);
  } else  {
    EXPECT(Token::DEFAULT);
    clause->SetDefault();
  }

  EXPECT(Token::COLON);

  while (token_ != Token::RBRACE &&
         token_ != Token::CASE   &&
         token_ != Token::DEFAULT) {
    stmt = ParseStatement(CHECK);
    clause->AddStatement(stmt);
  }

  return clause;
}

//  ThrowStatement
//    : THROW Expression ';'
Statement* Parser::ParseThrowStatement(bool *res) {
  Expression *expr;
  Next();
  // Throw requires Expression
  if (lexer_.has_line_terminator_before_next()) {
    FAIL();
  }
  expr = ParseExpression(true, CHECK);
  ExpectSemicolon(CHECK);
  return NEW(ThrowStatement(expr));
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
Statement* Parser::ParseTryStatement(bool *res) {
  Identifier *name;
  Block *block;
  bool has_catch_or_finally = false;

  Next();

  block = ParseBlock(CHECK);
  TryStatement *try_stmt = NEW(TryStatement(block));

  if (token_ == Token::CATCH) {
    // Catch
    has_catch_or_finally = true;
    Next();
    EXPECT(Token::LPAREN);
    IS(Token::IDENTIFIER);
    name = space_->NewIdentifier(lexer_.Buffer());
    // section 12.14.1
    // within the strict code, Identifier must not be "eval" or "arguments"
    if (strict_ && IsEvalOrArguments(name)) {
      FAIL();
    }
    Next();
    EXPECT(Token::RPAREN);
    block = ParseBlock(CHECK);
    try_stmt->SetCatch(name, block);
  }

  if (token_ == Token::FINALLY) {
    // Finally
    has_catch_or_finally= true;
    Next();
    block = ParseBlock(CHECK);
    try_stmt->SetFinally(block);
  }

  if (!has_catch_or_finally) {
    FAIL();
  }

  return try_stmt;
}


//  DebuggerStatement
//    : DEBUGGER ';'
Statement* Parser::ParseDebuggerStatement(bool *res) {
  Next();
  ExpectSemicolon(CHECK);
  return space_->NewDebuggerStatement();
}

//  LabelledStatement
//    : IDENTIFIER ':' Statement
//
//  ExpressionStatement
//    : Expression ';'
Statement* Parser::ParseExpressionOrLabelledStatement(bool *res) {
  assert(token_ == Token::IDENTIFIER);
  Expression* expr = ParseExpression(true, CHECK);
  if (token_ == Token::COLON &&
      expr->AsLiteral() &&
      expr->AsLiteral()->AsIdentifier()) {
    // LabelledStatement
    Next();

    AstNode::Identifiers* labels = labels_;
    Identifier* const label = expr->AsLiteral()->AsIdentifier();
    const bool exist_labels = labels;
    if (!exist_labels) {
      labels = space_->NewLabels();
    }
    if (ContainsLabel(labels, label) || TargetsContainsLabel(label)) {
      // duplicate label
      FAIL();
    }
    labels->push_back(label);
    LabelScope scope(this, labels, exist_labels);

    Statement* stmt = ParseStatement(CHECK);
    return NEW(LabelledStatement(expr, stmt));
  }
  ExpectSemicolon(CHECK);
  return NEW(ExpressionStatement(expr));
}

Statement* Parser::ParseExpressionStatement(bool *res) {
  Expression* expr = ParseExpression(true, CHECK);
  ExpectSemicolon(CHECK);
  return NEW(ExpressionStatement(expr));
}

//  Expression
//    : AssignmentExpression
//    | Expression ',' AssignmentExpression
Expression* Parser::ParseExpression(bool contains_in, bool *res) {
  Expression *right;
  Expression *result = ParseAssignmentExpression(contains_in, CHECK);
  while (token_ == Token::COMMA) {
    Next();
    right = ParseAssignmentExpression(contains_in, CHECK);
    result = NEW(BinaryOperation(Token::COMMA, result, right));
  }
  return result;
}

//  AssignmentExpression
//    : ConditionalExpression
//    | LeftHandSideExpression AssignmentOperator AssignmentExpression
Expression* Parser::ParseAssignmentExpression(bool contains_in, bool *res) {
  Expression *result = ParseConditionalExpression(contains_in, CHECK);
  if (!Token::IsAssignOp(token_)) {
    return result;
  }
  if (result == NULL || !result->IsValidLeftHandSide()) {
    FAIL();
  }
  const Token::Type op = token_;
  Next();
  Expression *right = ParseAssignmentExpression(contains_in, CHECK);
  if (right == NULL) {
    FAIL();
  }
  return NEW(Assignment(op, result, right));
}

//  ConditionalExpression
//    : LogicalOrExpression
//    | LogicalOrExpression '?' AssignmentExpression ':' AssignmentExpression
Expression* Parser::ParseConditionalExpression(bool contains_in, bool *res) {
  Expression *result;
  result = ParseBinaryExpression(contains_in, 9, CHECK);
  if (token_ == Token::CONDITIONAL) {
    Next();
    // see ECMA-262 section 11.12
    Expression *left = ParseAssignmentExpression(true, CHECK);
    EXPECT(Token::COLON);
    Expression *right = ParseAssignmentExpression(contains_in, CHECK);
    result = NEW(ConditionalExpression(result, left, right));
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
Expression* Parser::ParseBinaryExpression(bool contains_in,
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
    left = ReduceBinaryOperation(op, left, right);
  }
  if (prec < 1) return left;

  // AdditiveExpression
  while (token_ == Token::ADD ||
      token_ == Token::SUB) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 0, CHECK);
    left = ReduceBinaryOperation(op, left, right);
  }
  if (prec < 2) return left;

  // ShiftExpression
  while (token_ == Token::SHL ||
      token_ == Token::SAR ||
      token_ == Token::SHR) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 1, CHECK);
    left = ReduceBinaryOperation(op, left, right);
  }
  if (prec < 3) return left;

  // RelationalExpression
  while ((Token::REL_FIRST < token_ &&
      token_ < Token::REL_LAST) ||
      (contains_in && token_ == Token::IN)) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 2, CHECK);
    left = NEW(BinaryOperation(op, left, right));
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
    left = NEW(BinaryOperation(op, left, right));
  }
  if (prec < 5) return left;

  // BitwiseAndExpression
  while (token_ == Token::BIT_AND) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 4, CHECK);
    left = ReduceBinaryOperation(op, left, right);
  }
  if (prec < 6) return left;

  // BitwiseXorExpression
  while (token_ == Token::BIT_XOR) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 5, CHECK);
    left = ReduceBinaryOperation(op, left, right);
  }
  if (prec < 7) return left;

  // BitwiseOrExpression
  while (token_ == Token::BIT_OR) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 6, CHECK);
    left = ReduceBinaryOperation(op, left, right);
  }
  if (prec < 8) return left;

  // LogicalAndExpression
  while (token_ == Token::LOGICAL_AND) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 7, CHECK);
    left = NEW(BinaryOperation(op, left, right));
  }
  if (prec < 9) return left;

  // LogicalOrExpression
  while (token_ == Token::LOGICAL_OR) {
    op = token_;
    Next();
    right = ParseBinaryExpression(contains_in, 8, CHECK);
    left = NEW(BinaryOperation(op, left, right));
  }
  return left;
}

Expression* Parser::ReduceBinaryOperation(Token::Type op,
                                           Expression* left,
                                           Expression* right) {
  if (left->AsLiteral() &&
      right->AsLiteral() &&
      left->AsLiteral()->AsNumberLiteral() &&
      right->AsLiteral()->AsNumberLiteral()) {
    const double l_val = left->AsLiteral()->AsNumberLiteral()->value();
    const double r_val = right->AsLiteral()->AsNumberLiteral()->value();
    Expression* res;
    switch (op) {
      case Token::ADD:
        res = NumberLiteral::New(space_, l_val + r_val);
        break;

      case Token::SUB:
        res = NumberLiteral::New(space_, l_val - r_val);
        break;

      case Token::MUL:
        res = NumberLiteral::New(space_, l_val * r_val);
        break;

      case Token::DIV:
        res = NumberLiteral::New(space_, l_val / r_val);
        break;

      case Token::BIT_OR:
        res = NumberLiteral::New(space_,
                      DoubleToInt32(l_val) | DoubleToInt32(r_val));
        break;

      case Token::BIT_AND:
        res = NumberLiteral::New(space_,
                      DoubleToInt32(l_val) & DoubleToInt32(r_val));
        break;

      case Token::BIT_XOR:
        res = NumberLiteral::New(space_,
                      DoubleToInt32(l_val) ^ DoubleToInt32(r_val));
        break;

      // section 11.7 Bitwise Shift Operators
      case Token::SHL: {
        const int32_t value = DoubleToInt32(l_val)
            << (DoubleToInt32(r_val) & 0x1f);
        res = NumberLiteral::New(space_, value);
        break;
      }

      case Token::SHR: {
        const uint32_t shift = DoubleToInt32(r_val) & 0x1f;
        const uint32_t value = DoubleToUInt32(l_val) >> shift;
        res = NumberLiteral::New(space_, value);
        break;
      }

      case Token::SAR: {
        uint32_t shift = DoubleToInt32(r_val) & 0x1f;
        int32_t value = DoubleToInt32(l_val) >> shift;
        res = NumberLiteral::New(space_, value);
        break;
      }

      default:
        res = NEW(BinaryOperation(op, left, right));
        break;
    }
    return res;
  } else {
    return NEW(BinaryOperation(op, left, right));
  }
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
Expression* Parser::ParseUnaryExpression(bool *res) {
  Expression *result, *expr;
  const Token::Type op = token_;
  switch (token_) {
    case Token::VOID:
    case Token::NOT:
    case Token::TYPEOF:
    case Token::DELETE:
      Next();
      expr = ParseUnaryExpression(CHECK);
      result = NEW(UnaryOperation(op, expr));
      break;

    case Token::BIT_NOT:
      Next();
      expr = ParseUnaryExpression(CHECK);
      if (expr->AsLiteral() && expr->AsLiteral()->AsNumberLiteral()) {
        result = NumberLiteral::New(
           space_,
           ~DoubleToInt32(expr->AsLiteral()->AsNumberLiteral()->value()));
      } else {
        result = NEW(UnaryOperation(op, expr));
      }
      break;

    case Token::ADD:
      Next();
      expr = ParseUnaryExpression(CHECK);
      if (expr->AsLiteral() && expr->AsLiteral()->AsNumberLiteral()) {
        result = expr;
      } else {
        result = NEW(UnaryOperation(op, expr));
      }
      break;

    case Token::SUB:
      Next();
      expr = ParseUnaryExpression(CHECK);
      if (expr->AsLiteral() && expr->AsLiteral()->AsNumberLiteral()) {
        result = NumberLiteral::New(space_,
                              -(expr->AsLiteral()->AsNumberLiteral()->value()));
      } else {
        result = NEW(UnaryOperation(op, expr));
      }
      break;

    case Token::INC:
    case Token::DEC:
      Next();
      expr = ParseMemberExpression(true, CHECK);
      if (expr == NULL || !expr->IsValidLeftHandSide()) {
        FAIL();
      }
      result = NEW(UnaryOperation(op, expr));
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
Expression* Parser::ParsePostfixExpression(bool *res) {
  Expression *expr;
  expr = ParseMemberExpression(true, CHECK);
  if (!lexer_.has_line_terminator_before_next() &&
      (token_ == Token::INC || token_ == Token::DEC)) {
    if (expr == NULL || !expr->IsValidLeftHandSide()) {
      FAIL();
    }
    expr = NEW(PostfixExpression(token_, expr));
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
Expression* Parser::ParseMemberExpression(bool allow_call, bool *res) {
  Expression *expr;
  if (token_ != Token::NEW) {
    if (token_ == Token::FUNCTION) {
      // FunctionExpression
      Next();
      expr = ParseFunctionLiteral(FunctionLiteral::EXPRESSION,
                                  FunctionLiteral::GENERAL, true, CHECK);
    } else {
      expr = ParsePrimaryExpression(CHECK);
    }
  } else {
    Next();
    Expression *target = ParseMemberExpression(false, CHECK);
    ConstructorCall *con = NEW(ConstructorCall(target, space_));
    if (token_ == Token::LPAREN) {
      ParseArguments(con, CHECK);
    }
    expr = con;
  }
  FunctionCall *funcall;
  while (true) {
    switch (token_) {
      case Token::LBRACK: {
        Next();
        Expression* index = ParseExpression(true, CHECK);
        expr = NEW(IndexAccess(expr, index));
        EXPECT(Token::RBRACK);
        break;
      }

      case Token::PERIOD: {
        Next(Lexer::kIgnoreReservedWords);  // IDENTIFIERNAME
        IS(Token::IDENTIFIER);
        Identifier* ident = space_->NewIdentifier(lexer_.Buffer());
        Next();
        expr = NEW(IdentifierAccess(expr, ident));
        break;
      }

      case Token::LPAREN:
        if (allow_call) {
          funcall = NEW(FunctionCall(expr, space_));
          ParseArguments(funcall, CHECK);
          expr = funcall;
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
Expression* Parser::ParsePrimaryExpression(bool *res) {
  Expression *result = NULL;
  switch (token_) {
    case Token::THIS:
      result = space_->NewThisLiteral();
      Next();
      break;

    case Token::IDENTIFIER:
      result = space_->NewIdentifier(lexer_.Buffer());
      Next();
      break;

    case Token::NULL_LITERAL:
      result = space_->NewNullLiteral();
      Next();
      break;

    case Token::TRUE_LITERAL:
      result = space_->NewTrueLiteral();
      Next();
      break;

    case Token::FALSE_LITERAL:
      result = space_->NewFalseLiteral();
      Next();
      break;

    case Token::NUMBER:
      // section 7.8.3
      // strict mode forbids Octal Digits Literal
      if (strict_ && lexer_.NumericType() == Lexer::OCTAL) {
        FAIL();
      }
      result = NumberLiteral::New(space_, lexer_.Numeric());
      Next();
      break;

    case Token::STRING:
      if (strict_ && lexer_.StringEscapeType() == Lexer::OCTAL) {
        FAIL();
      }
      result = space_->NewStringLiteral(lexer_.Buffer());
      Next();
      break;

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
      FAIL();
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
Call* Parser::ParseArguments(Call* func, bool *res) {
  Expression *expr;
  Next();
  while (token_ != Token::RPAREN) {
    expr = ParseAssignmentExpression(true, CHECK);
    func->AddArgument(expr);
    if (token_ != Token::RPAREN) {
      EXPECT(Token::COMMA);
    }
  }
  Next();
  return func;
}

Expression* Parser::ParseRegExpLiteral(bool contains_eq, bool *res) {
  if (lexer_.ScanRegExpLiteral(contains_eq)) {
    RegExpLiteral *expr = space_->NewRegExpLiteral(lexer_.Buffer());
    if (!lexer_.ScanRegExpFlags()) {
      FAIL();
    } else {
      expr->SetFlags(lexer_.Buffer());
    }
    Next();
    return expr;
  } else {
    FAIL();
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
Expression* Parser::ParseArrayLiteral(bool *res) {
  ArrayLiteral *array = space_->NewArrayLiteral();
  Expression *expr;
  Next();
  while (token_ != Token::RBRACK) {
    if (token_ == Token::COMMA) {
      // when Token::COMMA, only increment length
      array->AddItem(NULL);
    } else {
      expr = ParseAssignmentExpression(true, CHECK);
      array->AddItem(expr);
    }
    if (token_ != Token::RBRACK) {
      EXPECT(Token::COMMA);
    }
  }
  Next();
  return array;
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
Expression* Parser::ParseObjectLiteral(bool *res) {
  typedef std::tr1::unordered_map<IdentifierKey, int> ObjectMap;
  ObjectLiteral *object = space_->NewObjectLiteral();
  ObjectMap map;
  Expression *expr;
  Identifier *ident;

  // IDENTIFIERNAME
  Next(Lexer::kIgnoreReservedWordsAndIdentifyGetterOrSetter);
  while (token_ != Token::RBRACE) {
    if (token_ == Token::GET || token_ == Token::SET) {
      const bool is_get = token_ == Token::GET;
      // this is getter or setter or usual prop
      Next(Lexer::kIgnoreReservedWords);  // IDENTIFIERNAME
      if (token_ == Token::COLON) {
        // prop
        ident = space_->NewIdentifier(is_get ? "get" : "set");
        Next();
        expr = ParseAssignmentExpression(true, CHECK);
        object->AddDataProperty(ident, expr);
        ObjectMap::iterator it = map.find(ident);
        if (it == map.end()) {
          map.insert(ObjectMap::value_type(ident, ObjectLiteral::DATA));
        } else {
          if (it->second != ObjectLiteral::DATA) {
            FAIL();
          } else {
            if (strict_) {
              FAIL();
            }
          }
        }
      } else {
        // getter or setter
        if (token_ == Token::IDENTIFIER ||
            token_ == Token::STRING ||
            token_ == Token::NUMBER) {
          if (token_ == Token::NUMBER) {
            ident = space_->NewIdentifier(lexer_.Buffer8());
          } else {
            ident = space_->NewIdentifier(lexer_.Buffer());
          }
          Next();
          ObjectLiteral::PropertyDescriptorType type =
              (is_get) ? ObjectLiteral::GET : ObjectLiteral::SET;
          expr = ParseFunctionLiteral(
              FunctionLiteral::EXPRESSION,
              (is_get) ? FunctionLiteral::GETTER : FunctionLiteral::SETTER,
              false, CHECK);
          object->AddAccessor(type, ident, expr);
          ObjectMap::iterator it = map.find(ident);
          if (it == map.end()) {
            map.insert(ObjectMap::value_type(ident, type));
          } else if (it->second & (ObjectLiteral::DATA | type)) {
            FAIL();
          } else {
            it->second |= type;
          }
        } else {
          FAIL();
        }
      }
    } else if (token_ == Token::IDENTIFIER ||
               token_ == Token::STRING ||
               token_ == Token::NUMBER) {
      ident = space_->NewIdentifier(lexer_.Buffer());
      Next();
      EXPECT(Token::COLON);
      expr = ParseAssignmentExpression(true, CHECK);
      object->AddDataProperty(ident, expr);
      ObjectMap::iterator it = map.find(ident);
      if (it == map.end()) {
        map.insert(ObjectMap::value_type(ident, ObjectLiteral::DATA));
      } else {
        if (it->second != ObjectLiteral::DATA) {
          FAIL();
        } else {
          if (strict_) {
            FAIL();
          }
        }
      }
    } else {
      FAIL();
    }

    if (token_ != Token::RBRACE) {
      IS(Token::COMMA);
      // IDENTIFIERNAME
      Next(Lexer::kIgnoreReservedWordsAndIdentifyGetterOrSetter);
    }
  }
  Next();
  return object;
}

FunctionLiteral* Parser::ParseFunctionLiteral(
    FunctionLiteral::DeclType decl_type,
    FunctionLiteral::ArgType arg_type,
    bool allow_identifier,
    bool *res) {
  // IDENTIFIER
  // IDENTIFIER_opt
  FunctionLiteral *literal = space_->NewFunctionLiteral(decl_type);
  literal->set_strict(strict_);
  literal->set_source(lexer_.source());
  if (allow_identifier && token_ == Token::IDENTIFIER) {
    literal->SetName(space_->NewIdentifier(lexer_.Buffer()));
    Next();
  }
  const ScopeSwitcher switcher(this, literal->scope());
  literal->set_start_position(lexer_.pos() - 2);

  //  '(' FormalParameterList_opt ')'
  EXPECT(Token::LPAREN);

  if (arg_type == FunctionLiteral::GETTER) {
    // if getter, arguments count is 0
    EXPECT(Token::RPAREN);
  } else if (arg_type == FunctionLiteral::SETTER) {
    // if setter, arguments count is 1
    IS(Token::IDENTIFIER);
    literal->AddParameter(space_->NewIdentifier(lexer_.Buffer()));
    Next();
    EXPECT(Token::RPAREN);
  } else {
    while (token_ != Token::RPAREN) {
      IS(Token::IDENTIFIER);
      literal->AddParameter(space_->NewIdentifier(lexer_.Buffer()));
      Next();
      if (token_ != Token::RPAREN) {
        EXPECT(Token::COMMA);
      }
    }
    Next();
  }

  //  '{' FunctionBody '}'
  //
  //  FunctionBody
  //    :
  //    | SourceElements
  EXPECT(Token::LBRACE);

  ParseSourceElements(Token::RBRACE, literal, CHECK);
  if (strict_ || literal->strict()) {
    // section 13.1
    // Strict Mode Restrictions
    if (literal->name() && IsEvalOrArguments(literal->name())) {
      FAIL();
    }
    for (AstNode::Identifiers::const_iterator it = literal->params().begin(),
         last = literal->params().end();
         it != last; ++it) {
      if (IsEvalOrArguments(*it)) {
        FAIL();
      }
      AstNode::Identifiers::const_iterator searcher = it;
      ++searcher;
      for (;searcher != last; ++searcher) {
        if ((*it)->value() == (*searcher)->value()) {
          FAIL();
        }
      }
    }
  }
  literal->set_end_position(lexer_.pos() - 2);
  Next();
  return literal;
}

bool Parser::ContainsLabel(const AstNode::Identifiers* const labels,
                           const Identifier * const label) const {
  assert(label != NULL);
  if (labels) {
    const SpaceUString& value = label->value();
    for (AstNode::Identifiers::const_iterator it = labels->begin(),
                                              last = labels->end();
         it != last; ++it) {
      if ((*it)->value() == value) {
        return true;
      }
    }
  }
  return false;
}

bool Parser::TargetsContainsLabel(const Identifier* const label) const {
  assert(label != NULL);
  for (Target* target = target_;
       target != NULL;
       target = target->previous()) {
    if (ContainsLabel(target->node()->labels(), label)) {
      return true;
    }
  }
  return false;
}

BreakableStatement* Parser::LookupBreakableTarget(
    const Identifier* const label) const {
  assert(label != NULL);
  for (const Target* target = target_;
       target != NULL;
       target = target->previous()) {
    if (ContainsLabel(target->node()->labels(), label)) {
      return target->node();
    }
  }
  return NULL;
}

BreakableStatement* Parser::LookupBreakableTarget() const {
  for (const Target* target = target_;
       target != NULL;
       target = target->previous()) {
    if (target->node()->AsAnonymousBreakableStatement()) {
      return target->node();
    }
  }
  return NULL;
}

IterationStatement* Parser::LookupContinuableTarget(
    const Identifier* const label) const {
  assert(label != NULL);
  for (const Target* target = target_;
       target != NULL;
       target = target->previous()) {
    IterationStatement* const iter = target->node()->AsIterationStatement();
    if (iter && ContainsLabel(iter->labels(), label)) {
      return iter;
    }
  }
  return NULL;
}

IterationStatement* Parser::LookupContinuableTarget() const {
  for (const Target* target = target_;
       target != NULL;
       target = target->previous()) {
    IterationStatement* const iter = target->node()->AsIterationStatement();
    if (iter) {
      return iter;
    }
  }
  return NULL;
}

void Parser::ReportUnexpectedToken() {
  switch (token_) {
    case Token::STRING:
      break;
    case Token::NUMBER:
      break;
    case Token::IDENTIFIER:
      break;
    case Token::EOS:
      break;
    default:
      break;
  }
}

void Parser::UnresolvedCheck() {
  std::tr1::unordered_set<IdentifierKey> idents;
  for (Scope::Variables::const_iterator
       it = scope_->variables().begin(),
       last = scope_->variables().end(); it != last; ++it) {
    idents.insert(it->first);
  }
  for (Scope::FunctionLiterals::const_iterator
       it = scope_->function_declarations().begin(),
       last = scope_->function_declarations().begin(); it != last; ++it) {
    idents.insert((*it)->name());
  }
  resolved_check_stack_.erase(
      std::remove_if(resolved_check_stack_.begin(),
                     resolved_check_stack_.end(),
                     std::tr1::bind(RemoveResolved,
                                    idents,
                                    std::tr1::placeholders::_1)),
      resolved_check_stack_.end());
}

bool Parser::RemoveResolved(
    const std::tr1::unordered_set<IdentifierKey>& idents,
    const Unresolveds::value_type& target) {
  return idents.find(target.first) != idents.end();
}

bool Parser::IsEvalOrArguments(const Identifier* ident) {
  const SpaceUString& str = ident->value();
  return (str.compare(eval_string.data()) == 0 ||
          str.compare(arguments_string.data()) == 0);
}

#undef REPORT
#undef TRACE
#undef CHECK
#undef FAIL
#undef EXPECT
#undef IS
#undef NEW

} }  // namespace iv::core
