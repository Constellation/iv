#ifndef IV_CANONICALIZED_NAN_H_
#define IV_CANONICALIZED_NAN_H_
#include "detail/cinttypes.h"
#include "bit_cast.h"
namespace iv {
namespace core {

// 111111111111000000000000000000000000000000000000000000000000000
static const double kNaN = BitCast<double, uint64_t>(UINT64_C(0x7FF8000000000000));

} }  // namespace iv::core
#endif  // IV_CANONICALIZED_NAN_H_
