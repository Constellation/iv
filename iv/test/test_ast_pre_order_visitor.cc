#include <string>
#include <iterator>
#include <algorithm>
#include <memory>
#include <gtest/gtest.h>
#include <iv/detail/cstdint.h>
#include <iv/ast_pre_order_visitor.h>
#include <iv/ast_factory.h>

namespace {

class Factory : public iv::core::ast::AstFactory<Factory> {
};

}  // namespace annonymous

TEST(AstPreOrderVisitor, NormalCompileTest) {
  using namespace iv::core;
  std::unique_ptr<Factory> factory(new Factory);
  ast::AstPreOrderVisitor<Factory>::type visitor;
}

TEST(AstPreOrderVisitor, ConstCompileTest) {
  using namespace iv::core;
  std::unique_ptr<Factory> factory(new Factory);
  ast::AstPreOrderVisitor<Factory>::const_type visitor;
}
