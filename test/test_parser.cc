#include <gtest/gtest.h>
#include <string>
#include "space.h"
#include "ast_factory.h"
#include "parser.h"

class AstFactory
  : public iv::core::Space<1>,
    public iv::core::ast::BasicAstFactory<AstFactory> { };

TEST(ParserCase, StringTest) {
  AstFactory factory;
  std::string src("var i = 20;");
  iv::core::Parser<AstFactory, std::string> parser(&factory, src);
}
