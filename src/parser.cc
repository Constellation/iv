#include "parser.h"

namespace iv {
namespace core {

#ifdef DEBUG

#define REPORT\
  std::printf("error line: %d\n", __LINE__);\
  std::printf("error source: %d\n", lexer_.line_number());

#define TRACE\
  std::printf("check: %d\n", __LINE__);

#else

#define REPORT
#define TRACE

#endif

#define IS(token)\
  do {\
    if (token_ != token) {\
      *res = false;\
      REPORT\
      return NULL;\
    }\
  } while (0)

#define EXPECT(token)\
  do {\
    if (token_ != token) {\
      *res = false;\
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

#define NEW(a) (new (factory_) a)

Parser::Parser(const char* source)
  : lexer_(source),
    stack_(),
    in_source_element_(false),
    space_(),
    factory_(&space_),
    empty_statement_instance_(),
    undefined_instance_(),
    this_instance_(),
    null_instance_() {
}

// Program
//   : SourceElements
FunctionLiteral* Parser::ParseProgram() {
  FunctionLiteral* global = space_.NewFunctionLiteral(FunctionLiteral::GLOBAL);
  bool error_flag = true;
  bool *res = &error_flag;
  Next();
  ParseSourceElements(Token::EOS, global, CHECK);
  return global;
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
  while (token_ != end) {
    if (token_ == Token::FUNCTION) {
      // FunctionDeclaration
      // FunctionExpression
      stmt = ParseFunctionDeclaration(CHECK);
      function->AddStatement(stmt);
    } else {
      stmt = ParseStatement(CHECK);
      function->AddStatement(stmt);
    }
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

    default:
      // LabelledStatement or ExpressionStatement or ILLEGAL
      result = ParseExpressionOrLabelledStatement(CHECK);
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
  if (token_ == Token::IDENTIFIER) {
    // FunctionDeclaration
    expr = ParseFunctionLiteral(FunctionLiteral::DECLARATION, CHECK);
  } else {
    // FunctionExpression
    // expr = ParseFunctionLiteral(FunctionLiteral::EXPRESSION, CHECK);
    FAIL();
  }
  return NEW(FunctionStatement(expr));
}

Statement* Parser::ParseFunctionStatement(bool *res) {
  FunctionLiteral *expr;
  Next();
  if (token_ == Token::IDENTIFIER) {
    // FunctionStatement
    expr = ParseFunctionLiteral(FunctionLiteral::STATEMENT, CHECK);
  } else {
    // FunctionExpression
    // expr = ParseFunctionLiteral(FunctionLiteral::EXPRESSION, CHECK);
    FAIL();
  }
  return NEW(FunctionStatement(expr));
}

bool Parser::ParseIdentifier() {
  Next();
  return true;
}

//  Block
//    : '{' '}'
//    | '{' StatementList '}'
//
//  StatementList
//    : Statement
//    | StatementList Statement
Block* Parser::ParseBlock(bool *res) {
  Block *block = NEW(Block(factory_));
  Statement *stmt;
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
  VariableStatement* stmt = NEW(VariableStatement(token_, factory_));
  ParseVariableDeclarations(stmt, CHECK);
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
                                             bool *res) {
  Identifier *name;
  Expression *expr;
  Declaration *decl;

  do {
    Next();
    IS(Token::IDENTIFIER);
    name = space_.NewIdentifier(lexer_.Literal());
    Next();

    if (token_ == Token::ASSIGN) {
      Next();
      // AssignmentExpression
      expr = ParseAssignmentExpression(true, CHECK);
      decl = NEW(Declaration(name, expr));
    } else {
      // Undefined Expression
      decl = NEW(Declaration(name, NewUndefined()));
    }
    stmt->AddDeclaration(decl);
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
  return NewEmptyStatement();
}

//  IfStatement
//    : IF '(' Expression ')' Statement ELSE Statement
//    | IF '(' Expression ')' Statement
Statement* Parser::ParseIfStatement(bool *res) {
  IfStatement *if_stmt = NULL;
  Statement* stmt;
  Expression *expr;
  Next();

  EXPECT(Token::LPAREN);

  expr = ParseExpression(true, CHECK);

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
  Statement *stmt;
  Expression *expr;
  Next();

  stmt = ParseStatement(CHECK);

  EXPECT(Token::WHILE);

  EXPECT(Token::LPAREN);

  expr = ParseExpression(true, CHECK);

  EXPECT(Token::RPAREN);

  ExpectSemicolon(CHECK);
  return NEW(DoWhileStatement(stmt, expr));
}

Statement* Parser::ParseWhileStatement(bool *res) {
  //  WHILE '(' Expression ')' Statement
  Expression* expr;
  Statement* stmt;
  Next();

  EXPECT(Token::LPAREN);

  expr = ParseExpression(true, CHECK);

  EXPECT(Token::RPAREN);

  stmt = ParseStatement(CHECK);

  return NEW(WhileStatement(stmt, expr));
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
      VariableStatement *var = NEW(VariableStatement(token_, factory_));
      ParseVariableDeclarations(var, CHECK);
      init = var;
      if (token_ == Token::IN) {
        // for in loop
        Next();
        Expression *enumerable = ParseExpression(true, CHECK);
        EXPECT(Token::RPAREN);
        Statement *body = ParseStatement(CHECK);
        return NEW(ForInStatement(init, enumerable, body));
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
        Statement *body = ParseStatement(CHECK);
        return NEW(ForInStatement(init, enumerable, body));
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

  Statement *body = ParseStatement(CHECK);
  ForStatement *for_stmt = NEW(ForStatement(body));
  if (init != NULL) {
    for_stmt->SetInit(init);
  }
  if (cond != NULL) {
    for_stmt->SetCondition(cond);
  }
  if (next != NULL) {
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
    if (token_ == Token::IDENTIFIER) {
      Identifier* label = space_.NewIdentifier(lexer_.Literal());
      continue_stmt->SetLabel(label);
      // ParseIdentifier();
      Next();
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
    if (token_ == Token::IDENTIFIER) {
      Identifier* label = space_.NewIdentifier(lexer_.Literal());
      break_stmt->SetLabel(label);
      // ParseIdentifier();
      Next();
    }
  }
  ExpectSemicolon(CHECK);
  return break_stmt;
}

//  ReturnStatement
//    : RETURN Expression_opt ';'
Statement* Parser::ParseReturnStatement(bool *res) {
  Expression *expr;
  Next();
  if (lexer_.has_line_terminator_before_next() ||
      token_ == Token::SEMICOLON ||
      token_ == Token::RBRACE ||
      token_ == Token::EOS) {
    ExpectSemicolon(CHECK);
    return NEW(ReturnStatement(space_.NewUndefined()));
  }
  expr = ParseExpression(true, CHECK);
  ExpectSemicolon(CHECK);
  return NEW(ReturnStatement(expr));
}

//  WithStatement
//    : WITH '(' Expression ')' Statement
Statement* Parser::ParseWithStatement(bool *res) {
  Statement *stmt;
  Expression *expr;

  Next();

  EXPECT(Token::LPAREN);

  expr = ParseExpression(true, CHECK);

  EXPECT(Token::RPAREN);

  stmt = ParseStatement(CHECK);
  return NEW(WithStatement(expr, stmt));
}

//  SwitchStatement
//    : SWITCH '(' Expression ')' CaseBlock
//
//  CaseBlock
//    : '{' CaseClauses_opt '}'
//    | '{' CaseClauses_opt DefaultClause CaseClauses_opt '}'
Statement* Parser::ParseSwitchStatement(bool *res) {
  Expression *expr;
  CaseClause *case_clause;
  Next();

  EXPECT(Token::LPAREN);

  expr = ParseExpression(true, CHECK);
  SwitchStatement *switch_stmt = NEW(SwitchStatement(expr, factory_));

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
  CaseClause *clause = NEW(CaseClause());
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
    clause->SetStatement(stmt);
  }

  return clause;
}

//  ThrowStatement
//    : THROW Expression ';'
Statement* Parser::ParseThrowStatement(bool *res) {
  Expression *expr;
  Next();
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
    name = space_.NewIdentifier(lexer_.Literal());
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
  return space_.NewDebuggerStatement();
}

//  LabelledStatement
//    : IDENTIFIER ':' Statement
//
//  ExpressionStatement
//    : Expression ';'
Statement* Parser::ParseExpressionOrLabelledStatement(bool *res) {
  Expression* expr;
  Statement* stmt;
  if (token_ == Token::IDENTIFIER) {
    expr = ParseExpression(true, CHECK);
    if (token_ == Token::COLON &&
        expr->AsLiteral() &&
        expr->AsLiteral()->AsIdentifier()) {
      // LabelledStatement
      Next();
      stmt = ParseStatement(CHECK);
      return NEW(LabelledStatement(expr, stmt));
    }
  } else {
    expr = ParseExpression(true, CHECK);
  }
  ExpectSemicolon(CHECK);
  return NEW(ExpressionStatement(expr));
}

//  Expression
//    : AssignmentExpression
//    | Expression ',' AssignmentExpression
Expression* Parser::ParseExpression(bool contains_in, bool *res) {
  Expression *result;
  Expression *right;

  result = ParseAssignmentExpression(contains_in, CHECK);
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
  Expression *result;
  result = ParseConditionalExpression(contains_in, CHECK);
  if (!Token::IsAssignOp(token_)) {
    return result;
  }
  if (result == NULL || !result->IsValidLeftHandSide()) {
    FAIL();
  }
  const Token::Type op = token_;
  Expression *right = NULL;
  Next();
  right = ParseAssignmentExpression(contains_in, CHECK);
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
    Expression *right;
    Expression *left;
    Next();
    // see ECMA-262 section 11.12
    left = ParseAssignmentExpression(true, CHECK);
    EXPECT(Token::COLON);
    right = ParseAssignmentExpression(contains_in, CHECK);
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
    double l_val = left->AsLiteral()->AsNumberLiteral()->value();
    double r_val = right->AsLiteral()->AsNumberLiteral()->value();
    Expression* res;
    switch (op) {
      case Token::ADD:
        res = NumberLiteral::New(factory_, l_val + r_val);
        break;

      case Token::SUB:
        res = NumberLiteral::New(factory_, l_val - r_val);
        break;

      case Token::MUL:
        res = NumberLiteral::New(factory_, l_val * r_val);
        break;

      case Token::DIV:
        res = NumberLiteral::New(factory_, l_val / r_val);
        break;

      case Token::BIT_OR:
        res = NumberLiteral::New(factory_,
                      Conv::DoubleToInt32(l_val) | Conv::DoubleToInt32(r_val));
        break;

      case Token::BIT_AND:
        res = NumberLiteral::New(factory_,
                      Conv::DoubleToInt32(l_val) & Conv::DoubleToInt32(r_val));
        break;

      case Token::BIT_XOR:
        res = NumberLiteral::New(factory_,
                      Conv::DoubleToInt32(l_val) ^ Conv::DoubleToInt32(r_val));
        break;

      case Token::SHL: {
        int32_t value = Conv::DoubleToInt32(l_val) <<
                                    (Conv::DoubleToInt32(r_val) & 0x1f);
        res = NumberLiteral::New(factory_, value);
        break;
      }

      case Token::SHR: {
        uint32_t shift = Conv::DoubleToInt32(r_val) & 0x1f;
        uint32_t value = Conv::DoubleToUInt32(l_val) >> shift;
        res = NumberLiteral::New(factory_, value);
        break;
      }

      case Token::SAR: {
        uint32_t shift = Conv::DoubleToInt32(r_val) & 0x1f;
        int32_t value = Conv::DoubleToInt32(l_val) >> shift;
        res = NumberLiteral::New(factory_, value);
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
           factory_,
           ~Conv::DoubleToInt32(expr->AsLiteral()->AsNumberLiteral()->value()));
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
        result = NumberLiteral::New(factory_,
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
      expr = ParseFunctionLiteral(FunctionLiteral::EXPRESSION, CHECK);
    } else {
      expr = ParsePrimaryExpression(CHECK);
    }
  } else {
    Next();
    Expression *target = ParseMemberExpression(false, CHECK);
    ConstructorCall *con = NEW(ConstructorCall(target, factory_));
    if (token_ == Token::LPAREN) {
      ParseArguments(con, CHECK);
    }
    expr = con;
  }
  Expression *index;
  FunctionCall *funcall;
  while (true) {
    switch (token_) {
      case Token::LBRACK:
        Next();
        index = ParseExpression(true, CHECK);
        expr = NEW(Property(expr, index));
        EXPECT(Token::RBRACK);
        break;

      case Token::PERIOD:
        Next();
        IS(Token::IDENTIFIER);
        index = space_.NewIdentifier(lexer_.Literal());
        Next();
        expr = NEW(Property(expr, index));
        break;

      case Token::LPAREN:
        if (allow_call) {
          funcall = NEW(FunctionCall(expr, factory_));
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
      result = NewThisLiteral();
      Next();
      break;

    case Token::IDENTIFIER:
      result = space_.NewIdentifier(lexer_.Literal());
      Next();
      break;

    case Token::NULL_LITERAL:
      result = NewNullLiteral();
      Next();
      break;

    case Token::TRUE_LITERAL:
      result = TrueLiteral::New(factory_);
      Next();
      break;

    case Token::FALSE_LITERAL:
      result = FalseLiteral::New(factory_);
      Next();
      break;

    case Token::NUMBER:
      result = NumberLiteral::New(factory_, lexer_.Numeric());
      Next();
      break;

    case Token::STRING:
      result = space_.NewStringLiteral(lexer_.Literal());
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
  RegExpLiteral *expr;
  if (lexer_.ScanRegExpLiteral(contains_eq)) {
    expr = space_.NewRegExpLiteral(lexer_.Literal());
    if (!lexer_.ScanRegExpFlags()) {
      FAIL();
    } else {
      expr->SetFlags(lexer_.Literal());
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
  ArrayLiteral *array = space_.NewArrayLiteral();
  Expression *expr;
  Next();
  while (token_ != Token::RBRACK) {
    if (token_ == Token::COMMA) {
      // Undefined
      array->AddItem(NewUndefined());
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
  ObjectLiteral *object = space_.NewObjectLiteral();
  Expression *expr;
  Identifier *ident;
  Lexer::GetterOrSetter getter_or_setter;
  Next();
  while (token_ != Token::RBRACE) {
    if (token_ == Token::IDENTIFIER) {
      getter_or_setter = lexer_.IsGetterOrSetter();
      if (getter_or_setter == Lexer::kNotGetterOrSetter) {
        ident = space_.NewIdentifier(lexer_.Literal());
        Next();
        EXPECT(Token::COLON);
        expr = ParseAssignmentExpression(true, CHECK);
        object->AddProperty(ident, expr);
        if (token_ != Token::RBRACE) {
          EXPECT(Token::COMMA);
        }
      } else {
        // this is getter or setter or usual prop
        Next();
        if (token_ == Token::COLON) {
          // prop
          ident = space_.NewIdentifier(
              getter_or_setter == Lexer::kGetter ? "get" : "set");
          Next();
          expr = ParseAssignmentExpression(true, CHECK);
          object->AddProperty(ident, expr);
          if (token_ != Token::RBRACE) {
            EXPECT(Token::COMMA);
          }
        } else {
          // getter or setter
          if (token_ == Token::IDENTIFIER ||
              token_ == Token::STRING ||
              token_ == Token::NUMBER) {
            if (token_ == Token::NUMBER) {
              ident = space_.NewIdentifier(lexer_.Literal8());
            } else {
              ident = space_.NewIdentifier(lexer_.Literal());
            }
            Next();
            expr = ParseFunctionLiteral(
                FunctionLiteral::EXPRESSION, CHECK);
            object->AddProperty(ident, expr);
            if (token_ != Token::RBRACE) {
              EXPECT(Token::COMMA);
            }
          } else {
            FAIL();
          }
        }
      }
    } else if (token_ == Token::STRING) {
      ident = space_.NewIdentifier(lexer_.Literal());
      Next();
      EXPECT(Token::COLON);
      expr = ParseAssignmentExpression(true, CHECK);
      object->AddProperty(ident, expr);
      if (token_ != Token::RBRACE) {
        EXPECT(Token::COMMA);
      }
    } else if (token_ == Token::NUMBER) {
      ident = space_.NewIdentifier(lexer_.Literal8());
      Next();
      EXPECT(Token::COLON);
      expr = ParseAssignmentExpression(true, CHECK);
      object->AddProperty(ident, expr);
      if (token_ != Token::RBRACE) {
        EXPECT(Token::COMMA);
      }
    } else {
      FAIL();
    }
  }
  Next();
  return object;
}

FunctionLiteral* Parser::ParseFunctionLiteral(FunctionLiteral::Type type,
                                              bool *res) {
  // IDENTIFIER
  // IDENTIFIER_opt
  FunctionLiteral *literal = space_.NewFunctionLiteral(type);
  if (token_ == Token::IDENTIFIER) {
    // ParseIdentifier();
    literal->SetName(lexer_.Literal());
    Next();
  }

  //  '(' FormalParameterList_opt ')'
  EXPECT(Token::LPAREN);

  while (token_ != Token::RPAREN) {
    IS(Token::IDENTIFIER);
    literal->AddParameter(space_.NewIdentifier(lexer_.Literal()));
    Next();
    if (token_ != Token::RPAREN) {
      EXPECT(Token::COMMA);
    }
  }
  Next();

  //  '{' FunctionBody '}'
  //
  //  FunctionBody
  //    :
  //    | SourceElements
  EXPECT(Token::LBRACE);

  ParseSourceElements(Token::RBRACE, literal, CHECK);
  Next();
  return literal;
}

Undefined* Parser::NewUndefined() {
  return space_.NewUndefined();
}

EmptyStatement* Parser::NewEmptyStatement() {
  return space_.NewEmptyStatement();
}

ThisLiteral* Parser::NewThisLiteral() {
  return space_.NewThisLiteral();
}

NullLiteral* Parser::NewNullLiteral() {
  return space_.NewNullLiteral();
}


#undef REPORT
#undef TRACE
#undef CHECK
#undef FAIL
#undef EXPECT
#undef IS
#undef NEW

} }  // namespace iv::core

