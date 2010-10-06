#ifndef IV_LV5_DTOA_H_
#define IV_LV5_DTOA_H_
// Double to String

// David M Gay's algorithm and V8 fast-dtoa

namespace v8 {
namespace internal {
// Printing floating-point numbers quickly and accurately with integers. 
// Florian Loitsch, PLDI 2010.
extern char* DoubleToCString(double v, char* buffer, int buflen);
} }  // namespace v8::internal

namespace iv {
namespace core {

using v8::internal::DoubleToCString;

} }  // namespace iv::lv5
#endif  // IV_LV5_DTOA_H_
