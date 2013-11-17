#ifndef IV_LV5_FACTORY_H_
#define IV_LV5_FACTORY_H_
#include <vector>
#include <iv/alloc.h>
#include <iv/ast_factory.h>
#include <iv/location.h>
#include <iv/ustringpiece.h>
#include <iv/lv5/symbol.h>
#include <iv/lv5/specialized_ast.h>

namespace iv {
namespace lv5 {
class Context;

class AstFactory
  : public core::Space,
    public core::ast::BasicAstFactory<AstFactory> {
 public:
  typedef core::SpaceVector<AstFactory, RegExpLiteral*>::type DestReqs;
  explicit AstFactory(Context* ctx)
    : ctx_(ctx),
      regexps_(DestReqs::allocator_type(this)) { }

  ~AstFactory() {
    DestroyRegExpLiteral();
  }

  void Clear() {
    core::Space::Clear();
    DestroyRegExpLiteral();
  }

  inline RegExpLiteral* NewRegExpLiteral(
      const std::vector<uint16_t>& content,
      const std::vector<uint16_t>& flags,
      std::size_t begin,
      std::size_t end,
      std::size_t line_number) {
    RegExpLiteral* expr =
        new(this)RegExpLiteral(NewString(content), NewString(flags));
    expr->Initialize(ctx_);
    if (expr->IsValid()) {
      regexps_.push_back(expr);
      return Location(expr, begin, end, line_number);
    } else {
      return nullptr;
    }
  }
 private:
  void DestroyRegExpLiteral() {
    for (DestReqs::const_iterator it = regexps_.begin(),
         last = regexps_.end(); it != last; ++it) {
      (*it)->~RegExpLiteral();
    }
  }

  Context* ctx_;
  DestReqs regexps_;
};
} }  // namespace iv::lv5
#endif  // IV_LV5_FACTORY_H_
