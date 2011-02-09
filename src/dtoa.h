#ifndef _IV_DTOA_H_
#define _IV_DTOA_H_
#include "byteorder.h"

extern "C" char* dtoa(double d, int mode, int ndigits,
                      int *decpt, int *sign, char **rve);
extern "C" void freedtoa(char *s);

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
