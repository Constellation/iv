#ifndef _IV_REGEXP_CREATOR_H_
#define _IV_REGEXP_CREATOR_H_
#include <vector>
#include "uchar.h"
namespace iv {
namespace core {
class Space;
class RegExpLiteral;
class RegExpCreator {
 public:
  static RegExpLiteral* Create(Space* space,
                               const std::vector<UChar>& content,
                               const std::vector<UChar>& flags) {
    return new (space) RegExpLiteral(content, flags, space);
  }
};
} }  // namespace iv::core
#endif  // _IV_REGEXP_CREATOR_H_
