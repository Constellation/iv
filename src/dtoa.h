#ifndef _IV_DTOA_H_
#define _IV_DTOA_H_
#include "byteorder.h"

#ifdef IV_IS_LITTLE_ENDIAN
#define IEEE_8087
#else
#define IEEE_MC68k
#endif

// Double to String
// David M Gay's algorithm and V8 fast-dtoa
namespace iv {
namespace core {
namespace netlib {
#ifndef _IV_THIRD_PARTY_NET_LIB_DTOA_DTOA_C_
#define _IV_THIRD_PARTY_NET_LIB_DTOA_DTOA_C_
#include "third_party/netlib-dtoa/dtoa.c"
#endif  // _IV_THIRD_PARTY_NET_LIB_DTOA_DTOA_C_
} } }  // iv::core::netlib

using iv::core::netlib::dtoa;
using iv::core::netlib::freedtoa;

#undef CONST

namespace v8 {
namespace internal {
// Printing floating-point numbers quickly and accurately with integers. 
// Florian Loitsch, PLDI 2010.
extern char* DoubleToCString(double v, char* buffer, int buflen);
} }  // namespace v8::internal

namespace iv {
namespace core {

using v8::internal::DoubleToCString;

} }  // namespace iv::core
#endif  // _IV_DTOA_H_
