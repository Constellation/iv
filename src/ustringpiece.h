#ifndef _IV_USTRINGPIECE_H_
#define _IV_USTRINGPIECE_H_
#include "detail/cstdint.h"
#include "stringpiece.h"
namespace iv {
namespace core {
typedef BasicStringPiece<uint16_t, std::char_traits<uint16_t> > UStringPiece;
} }  // namespace iv::core
#endif  // _IV_USTRINGPIECE_H_
