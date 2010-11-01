#include "regexp-creator.h"
#include "alloc-inl.h"
#include "ast.h"
namespace iv {
namespace core {

RegExpLiteral* RegExpCreator::Create(Space* space,
                                     const std::vector<UChar>& content,
                                     const std::vector<UChar>& flags) {
  return new (space) RegExpLiteral(content, flags, space);
}

} }  // namespace iv::core
