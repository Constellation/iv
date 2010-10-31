#include "regexp-creator.h"
#include "alloc-inl.h"
#include "ast.h"
namespace iv {
namespace core {

RegExpLiteral* RegExpCreator::Create(Space* space,
                                     const std::vector<UChar>& content,
                                     const std::vector<UChar>& flags) {
  RegExpLiteral* expr = new (space) RegExpLiteral(content, space);
  expr->SetFlags(flags);
  return expr;
}

} }  // namespace iv::core
