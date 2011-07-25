// Incremental Parser
// Incremental Parser requires actions and completer.
// When Incremental Parser found code complete point (EOS), callback completer's
// handler. And, when Incremental Parser found Statement, Expression and so on,
// callback actions' handler.
// When this found comment, callback actions' comment handler.
//
// If you like, you can build AST at actions handler, and Incremental Parser
// provides error recovery state for SyntaxError. This is designed by Compiler
// Book, recovery section.
#ifndef _IV_INCREMENTAL_PARSER_H_
#define _IV_INCREMENTAL_PARSER_H_
#include "debug.h"
#include "ast.h"
#include "lexer.h"
#include "noncopyable.h"
namespace iv {
namespace core {

#define CHECK(expr)\
  do {\
    if (!expr) {\
      return false;\
    }\
  } while (0)

#define IS(token)\
  do {\
    if (token_ != token) {\
      CHECK(actions_->UnexpectedToken(this));\
      return false;\
    }\
  } while (0)

#define EXPECT(token)\
  do {\
    if (token_ != token) {\
      CHECK(actions_->UnexpectedToken(this));\
      return false;\
    }\
    Next();\
  } while (0)

#define UNEXPECT(token)\
  CHECK(actions_->UnexpectedToken(token))

template<typename Actions,
         typename Completer,
         typename Source>
class IncrementalParser
  : private Noncopyable<IncrementalParser<Actions, Completer, Source> > {
 public:
  typedef IncrementalParser<Actions, Completer, Source> this_type;
  typedef Lexer<Source, true> lexer_type;

  IncrementalParser(Actions* actions,
                    Completer* completer, const Source& source)
    : lexer_(source),
      actions_(actions),
      completer_(completer) {
  }

  bool ParseProgram() {
    Next();
    CHECK(actions_->ProgramBegin(this));
    CHECK(ParseSourceElements(Token::EOS));
    CHECK(actions_->ProgramEnd(this));
    return true;
  }

  bool ParseSourceElements(Token::Type end) {
    CHECK(actions_->SourceElementsBegin(this));
    while (token_ != end) {
      if (token_ == Token::FUNCTION) {
        CHECK(ParseFunctionDeclaration());
      } else {
        CHECK(ParseStatement());
      }
    }
    CHECK(actions_->SourceElementsEnd(this));
    return true;
  }

  bool ParseFunctionDeclaration() {
    CHECK(actions_->FunctionDeclarationBegin());
    CHECK(ParseFunctionLiteral(true));
    CHECK(actions_->FunctionDeclarationEnd());
    return true;
  }

  bool ParseStatement() {
    switch (token_) {
      case Token::LBRACE:
        // Block
        CHECK(ParseBlock());
        break;

      case Token::CONST:
      case Token::VAR:
        // VariableStatement
        CHECK(ParseVariableStatement());
        break;

      case Token::SEMICOLON:
        // EmptyStatement
        CHECK(ParseEmptyStatement());
        break;

      case Token::IF:
        // IfStatement
        CHECK(ParseIfStatement());
        break;

      case Token::DO:
        // IterationStatement
        // do while
        CHECK(ParseDoWhileStatement());
        break;

      case Token::WHILE:
        // IterationStatement
        // while
        CHECK(ParseWhileStatement());
        break;

      case Token::FOR:
        // IterationStatement
        // for
        CHECK(ParseForStatement());
        break;

      case Token::CONTINUE:
        // ContinueStatement
        CHECK(ParseContinueStatement());
        break;

      case Token::BREAK:
        // BreakStatement
        CHECK(ParseBreakStatement());
        break;

      case Token::RETURN:
        // ReturnStatement
        CHECK(ParseReturnStatement());
        break;

      case Token::WITH:
        // WithStatement
        CHECK(ParseWithStatement());
        break;

      case Token::SWITCH:
        // SwitchStatement
        CHECK(ParseSwitchStatement());
        break;

      case Token::THROW:
        // ThrowStatement
        CHECK(ParseThrowStatement());
        break;

      case Token::TRY:
        // TryStatement
        CHECK(ParseTryStatement());
        break;

      case Token::DEBUGGER:
        // DebuggerStatement
        CHECK(ParseDebuggerStatement());
        break;

      case Token::FUNCTION:
        // FunctionStatement (not in ECMA-262 5th)
        // FunctionExpression
        CHECK(ParseFunctionStatement());
        break;

      case Token::IDENTIFIER:
        // LabelledStatement or ExpressionStatement
        CHECK(ParseExpressionOrLabelledStatement());
        break;

      case Token::ILLEGAL:
        UNEXPECT(token_);
        break;

      default:
        // ExpressionStatement or ILLEGAL
        CHECK(ParseExpressionStatement());
        break;
    }
    return true;
  }

  bool ParseBlock() {
    CHECK(actions_->BlockBegin(this));
    Next();
    while (token_ != Token::RBRACE) {
      CHECK(ParseStatement());
    }
    Next();
    CHECK(actions_->BlockEnd(this));
    return true;
  }

  bool ParseVariableStatement() {
    assert(token_ == Token::VAR || token_ == Token::CONST);
    CHECK(actions_->VariableStatementBegin(this));
    CHECK(ParseVariableDeclarations(token_ == Token::CONST, true, true))
    CHECK(ExpectSemicolon());
    CHECK(actions_->VariableStatementEnd(this));
  }

  bool ParseVariableDeclarations(bool is_const,
                                 bool contains_in, bool is_list) {
    do {
      Next();
      IS(Token::IDENTIFIER);
      CHECK(actions_->VariableDeclarationIdentifier(this));
      Next();
      if (token_ == Token::ASSIGN) {
        CHECK(actions_->VariableDeclarationAssignBegin(this));
        Next();
        CHECK(ParseAssignmentExpression(contains_in));
        CHECK(actions_->VariableDeclarationAssignEnd(this));
      }
    } while (is_list && token_ == Token::COMMA);
    return true;
  }

  bool ParseEmptyStatement() {
    CHECK(actions_->EmtpyStatement());
    return true;
  }

  bool ParseIfStatement() {
    assert(token_ == Token::IF);
    CHECK(actions_->IfStatementBegin(this));
    Next();
    EXPECT(Token::LPAREN);
    CHECK(ParseExpression(true));
    EXPECT(Token::RPAREN);
    CHECK(ParseStatement());
    if (token_ == Token::ELSE) {
      Next();
      CHECK(ParseStatement());
    }
    CHECK(actions_->IfStatementEnd(this));
    return true;
  }

  bool ParseDoWhileStatement() {
    assert(token_ == Token::DO);
    CHECK(actions_->DoWhileStatementBegin(this));
    Next();
    CHECK(ParseStatement());
    EXPECT(Token::WHILE);
    EXPECT(Token::LPAREN);
    CHECK(ParseExpression(true));
    EXPECT(Token::RPAREN);
    if (token_ == Token::PERIOD) {
      Next();
    }
    CHECK(actions_->DoWhileStatementEnd(this));
    return true;
  }

  bool ParseWhileStatement() {
    assert(token_ == Token::WHILE);
    CHECK(actions_->WhileStatementBegin(this));
    Next();
    EXPECT(Token::LPAREN);
    CHECK(ParseExpression(true));
    EXPECT(Token::RPAREN);
    CHECK(ParseStatement());
    CHECK(actions_->WhileStatementEnd(this));
    return true;
  }

  bool ParseForStatement() {
    assert(token_ == Token::FOR);
    CHECK(actions_->ForStatementBegin(this));
    Next();
    EXPECT(Token::LPAREN);
    if (token_ != Token::SEMICOLON) {
      // ForInStatement pattern
      bool variables = false;
      const Token::Type t = token_;
      if (token_ == Token::VAR || token_ == Token::CONST) {
        variables = true;
        CHECK(ParseVariableDeclarations(t == Token::CONST, false, false));
      } else {
        CHECK(ParseExpression());
      }
      if (token_ == Token::IN) {
        CHECK(actions_->ForIn(this));
        CHECK(ParseExpression(true));
        EXPECT(Token::RPAREN);
        CHECK(ParseStatement());
        CHECK(actions_->ForStatementEnd(this));
        return true;
      } else if (variables && token_ == Token::COMMA) {
        // continue variables
        CHECK(ParseVariableDeclarations(t == Token::CONST, false, true));
      }
    }

    EXPECT(Token::SEMICOLON);
    if (token_ == Token::SEMICOLON) {
      Next();
    } else {
      CHECK(ParseExpression(true));
      EXPECT(Token::SEMICOLON);
    }

    if (token_ == Token::RPAREN) {
      Next();
    } else {
      CHECK(ParseExpression(true));
      EXPECT(Token::RPAREN);
    }

    CHECK(ParseStatement());
    CHECK(actions_->ForStatementEnd(this));
    return true;
  }

  bool ParseContinueStatement() {
    assert(token_ == Token::CONTINUE);
    CHECK(actions_->ContinueStatementBegin(this));
    Next();
    if (!lexer_.has_line_terminator_before_next() &&
        token_ != Token::SEMICOLON &&
        token_ != Token::RBRACE &&
        token_ != Token::EOS) {
      IS(Token::IDENTIFIER);
      CHECK(actions_->ContinueStatementIdentifier(this));
      Next();
    }
    CHECK(ExpectSemicolon());
    CHECK(actions_->ContinueStatementEnd(this));
    return true;
  }

  bool ParseBreakStatement() {
    assert(token_ == Token::BREAK);
    CHECK(actions_->BreakStatementBegin(this));
    Next();
    if (!lexer_.has_line_terminator_before_next() &&
        token_ != Token::SEMICOLON &&
        token_ != Token::RBRACE &&
        token_ != Token::EOS) {
      IS(Token::IDENTIFIER);
      CHECK(actions_->BreakStatementIdentifier(this));
      Next();
    }
    CHECK(ExpectSemicolon());
    CHECK(actions_->BreakStatementEnd(this));
    return true;
  }

  bool ParseReturnStatement() {
    assert(token_ == Token::RETURN);
    CHECK(actions_->ReturnStatementBegin(this));
    Next();
    if (token_ != Token::SEMICOLON && token_ == Token::EOS) {
      // code complete
      if (completer_) {
        completer_->CompleteReturn(this);
      }
    }
    if (lexer_.has_line_terminator_before_next() ||
        token_ == Token::SEMICOLON ||
        token_ == Token::RBRACE ||
        token_ == Token::EOS) {
      CHECK(ExpectSemicolon());
    } else {
      CHECK(ParseExpression(true));
      CHECK(ExpectSemicolon());
    }
    CHECK(actions_->ReturnStatementEnd(this));
    return true;
  }

  bool ParseWithStatement() {
    assert(token_ == Token::WITH);
    CHECK(actions_->WithStatementBegin(this));
    Next();
    CHECK(ParseExpression(true));
    EXPECT(Token::LPAREN);
    EXPECT(Token::RPAREN);
    CHECK(ParseStatement());
    CHECK(actions_->WithStatementEnd(this));
    return true;
  }

  bool ParseSwitchStatement() {
    assert(token_ == Token::SWITCH);
    CHECK(actions_->SwitchStatementBegin(this));
    Next();
    CHECK(ParseExpression(true));
    EXPECT(Token::RPAREN);
    EXPECT(Token::LBRACE);
    while (token_ != Token::RBRACE) {
      if (token_ == Token::CASE ||
          token_ == Token::DEFAULT) {
        const bool clause_is_case = token_ == Token::CASE;
        CHECK(ParseCaseClause(clause_is_case));
      } else {
        UNEXPECT(token_);
      }
    }
    Next();
    CHECK(actions_->SwitchStatementEnd(this));
    return true;
  }

  bool ParseCaseClause(bool clause_is_case) {
    assert(token_ == Token::CASE || token_ == Token::DEFAULT);
    if (clause_is_case) {
      CHECK(actions_->SwitchStatementCaseClauseBegin(this));
      Next();
      CHECK(ParseExpression(true));
    } else {
      CHECK(actions_->SwitchStatementDefaultClauseBegin(this));
      Next();
    }
    EXPECT(Token::COLON);
    while (token_ != Token::RBRACE &&
           token_ != Token::CASE   &&
           token_ != Token::DEFAULT) {
      CHECK(ParseStatement());
    }
    if (clause_is_case) {
      CHECK(actions_->SwitchStatementCaseClauseEnd(this));
    } else {
      CHECK(actions_->SwitchStatementDefaultClauseEnd(this));
    }
    return true;
  }

  bool ParseThrowStatement() {
    assert(token_ == Token::THROW);
    CHECK(actions_->ThrowStatementBegin(this));
    Next();
    if (lexer_.has_line_terminator_before_next()) {
      UNEXPECT(Token::ILLEGAL);
    }
    CHECK(ParseStatement());
    CHECK(ExpectSemicolon());
    CHECK(actions_->ThrowStatementEnd(this));
    return true;
  }

  bool ParseTryStatement() {
    assert(token_ == Token::TRY);
    CHECK(actions_->TryStatementBegin(this));
    Next();
    IS(Token::LBRACE);
    CHECK(ParseBlock());
    if (token_ == Token::CATCH) {
      CHECK(actions_->TryStatementCatchBegin(this));
      Next();
      EXPECT(Token::LPAREN);
      IS(Token::IDENTIFIER);
      CHECK(actions_->TryStatementCatchIdentifier(this));
      EXPECT(Token::RPAREN);
      IS(Token::LBRACE);
      CHECK(ParseBlock());
      CHECK(actions_->TryStatementCatchEnd(this));
    }

    if (token_ == Token::FINALLY) {
      CHECK(actions_->TryStatementFinallyBegin(this));
      Next();
      IS(Token::LBRACE);
      CHECK(ParseBlock());
      CHECK(actions_->TryStatementFinallyEnd(this));
    }
    CHECK(actions_->TryStatementEnd(this));
    return true;
  }

  bool ParseDebuggerStatement() {
    assert(token_ == Token::DEBUGGER);
    CHECK(actions_->DebuggerStatementBegin(this));
    Next();
    CHECK(ExpectSemicolon());
    CHECK(actions_->DebuggerStatementEnd(this));
    return true;
  }

  bool ParseExpressionStatement() {
    CHECK(actions_->ExpressionStatementBegin(this));
    CHECK(ParseExpression(true));
    CHECK(ExpectSemicolon());
    CHECK(actions_->ExpressionStatementEnd(this));
    return true;
  }


  bool ParseExpressionOrLabelledStatement() {
    assert(token_ == Token::IDENTIFIER);
    // Token get and return
    CHECK(ParseExpressionStatement());
    return true;
  }

  bool ParseFunctionStatement() {
    assert(token_ == Token::FUNCTION);
    CHECK(actions_->FunctionStatementBegin());
    CHECK(ParseFunctionLiteral(true));
    CHECK(actions_->FunctionStatementEnd());
    return true;
  }

  bool ParseExpression(bool contains_in) {
    CHECK(ParseAssignmentExpression(contains_in));
    while (token_ == Token::COMMA) {
      CHECK(actions_->CommaExpr(this));
      Next();
      CHECK(ParseAssignmentExpression(contains_in));
    }
    CHECK(actions_->CommanExprEnd(this));
    return true;
  }

  bool ParseAssignmentExpression(bool contains_in) {
  }

  template<typename LexType>
  inline Token::Type Next() {
    do {
      token_ = lexer_.Next<LexType>(actions_->IsStrict());
      if (token_ == Token::SINGLE_LINE_COMMENT) {
        if (!actions_->SingleLineComment(this)) {
          token_ = Token::ILLEGAL;
          return token_;
        }
        continue;
      }
      if (token_ == Token::MULTI_LINE_COMMENT) {
        if (!actions_->MultiLineComment(this)) {
          token_ = Token::ILLEGAL;
          return token_;
        }
        continue;
      }
    } while (false);
    return token_;
  }

  inline Token::Type Next() {
    do {
      token_ = lexer_.Next<IdentifyReservedWords>(actions_->IsStrict());
      if (token_ == Token::SINGLE_LINE_COMMENT) {
        if (!actions_->SingleLineComment(this)) {
          token_ = Token::ILLEGAL;
          return token_;
        }
        continue;
      }
      if (token_ == Token::MULTI_LINE_COMMENT) {
        if (!actions_->MultiLineComment(this)) {
          token_ = Token::ILLEGAL;
          return token_;
        }
        continue;
      }
    } while (false);
    return token_;
  }

  Token::Type token() const {
    return token_;
  }

  const lexer_type& lexer() const {
    return lexer_;
  }

  Actions* actions() {
    return actions_;
  }

  Completer* completer() {
    return completer_;
  }

 private:
  lexer_type lexer_;
  Token::Type token_;
  Actions* actions_;
  Completer* completer_;
};

#undef CHECK
#undef IS
#undef EXPECT
#undef UNEXPECT

} }  // namespace iv::core
#endif  // _IV_INCREMENTAL_PARSER_H_
