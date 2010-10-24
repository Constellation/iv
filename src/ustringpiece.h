#ifndef IV_USTRINGPIECE_H_
#define IV_USTRINGPIECE_H_
#include "uchar.h"
#include "stringpiece.h"
namespace iv {
namespace core {
typedef BasicStringPiece<UChar, std::char_traits<UChar> > UStringPiece;
} }  // namespace iv::core
#endif  // IV_USTRINGPIECE_H_
