#include <ruby.h>
#include "rparser.h"

#define RBFUNC(func) (reinterpret_cast<VALUE(*)(...)>(func))

extern "C" {

void Init_phonic() {
  iv::phonic::Encoding::Init();
  // IV
  VALUE mIV = rb_define_module("IV");

  // IV::Phonic
  VALUE mIVPhonic = rb_define_module_under(mIV, "Phonic");
  rb_define_module_function(mIVPhonic, "parse",
                            RBFUNC(&iv::phonic::RParser::Parse), 1);

  iv::phonic::RParser::Init(mIVPhonic);

  VALUE cNode = rb_define_class_under(mIVPhonic, "Node", rb_cObject);
  VALUE cExpr = rb_define_class_under(mIVPhonic, "Expression", cNode);
  VALUE cStmt = rb_define_class_under(mIVPhonic, "Statement", cNode);

  // Expressions
  rb_define_class_under(mIVPhonic, "Identifier", cExpr);
  rb_define_class_under(mIVPhonic, "StringLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "NumberLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "RegExpLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "FunctionLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "ArrayLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "ObjectLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "NullLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "ThisLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "Undefined", cExpr);
  rb_define_class_under(mIVPhonic, "TrueLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "FalseLiteral", cExpr);
  rb_define_class_under(mIVPhonic, "ConditionalExpression", cExpr);
  rb_define_class_under(mIVPhonic, "PostfixExpression", cExpr);
  rb_define_class_under(mIVPhonic, "BinaryOperation", cExpr);
  rb_define_class_under(mIVPhonic, "UnaryOperation", cExpr);
  rb_define_class_under(mIVPhonic, "Assignment", cExpr);
  rb_define_class_under(mIVPhonic, "FunctionCall", cExpr);
  rb_define_class_under(mIVPhonic, "ConstructorCall", cExpr);
  rb_define_class_under(mIVPhonic, "IndexAccess", cExpr);
  rb_define_class_under(mIVPhonic, "IdentifierAccess", cExpr);

  // Statements
  rb_define_class_under(mIVPhonic, "Block", cStmt);
  rb_define_class_under(mIVPhonic, "VariableStatement", cStmt);
  rb_define_class_under(mIVPhonic, "IfStatement", cStmt);
  rb_define_class_under(mIVPhonic, "DoWhileStatement", cStmt);
  rb_define_class_under(mIVPhonic, "WhileStatement", cStmt);
  rb_define_class_under(mIVPhonic, "ForInStatement", cStmt);
  rb_define_class_under(mIVPhonic, "ForStatement", cStmt);
  rb_define_class_under(mIVPhonic, "ExpressionStatement", cStmt);
  rb_define_class_under(mIVPhonic, "ContinueStatement", cStmt);
  rb_define_class_under(mIVPhonic, "ReturnStatement", cStmt);
  rb_define_class_under(mIVPhonic, "BreakStatement", cStmt);
  rb_define_class_under(mIVPhonic, "WithStatement", cStmt);
  rb_define_class_under(mIVPhonic, "SwitchStatement", cStmt);
  rb_define_class_under(mIVPhonic, "ThrowStatement", cStmt);
  rb_define_class_under(mIVPhonic, "TryStatement", cStmt);
  rb_define_class_under(mIVPhonic, "LabelledStatement", cStmt);
  rb_define_class_under(mIVPhonic, "DebuggerStatement", cStmt);
}

}

#undef RBFUNC
