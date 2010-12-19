#ifndef _IV_USTRINGPIECE_H_
#define _IV_USTRINGPIECE_H_
#include "uchar.h"
#include "stringpiece.h"
namespace iv {
namespace core {
typedef BasicStringPiece<uc16, std::char_traits<uc16> > UStringPiece;
} }  // namespace iv::core
#endif  // _IV_USTRINGPIECE_H_
