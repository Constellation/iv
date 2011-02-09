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
#include "netlib-dtoa.h"

// clean up netlib defines...
#undef CONST
#undef P
#undef D
#undef d0
#undef d1
#undef Long
#undef Llong
#undef ULLong
#undef Kmax
#undef MALLOC
#undef PRIVATE_MEM
#undef PRIVATE_mem
#undef word0
#undef word1
#undef dval
#undef Storeinc
#undef Flt_Rounds
#undef Exp_shift
#undef Exp_shift1
#undef Exp_msk1
#undef Exp_msk11
#undef Exp_mask
#undef P
#undef Nbits
#undef Bias
#undef Emax
#undef Emin
#undef Exp_1
#undef Exp_11
#undef Ebits
#undef Frac_mask
#undef Frac_mask1
#undef Ten_pmax
#undef Bletch
#undef Bndry_mask
#undef Bndry_mask1
#undef LSB
#undef Sign_bit
#undef Log2P
#undef Tiny0
#undef Tiny1
#undef Quick_max
#undef Int_max
#undef rounded_product
#undef rounded_quotient
#undef Big0
#undef Big1
#undef Bcopy
#undef Pack_32
#undef Scale_Bit
#undef n_bigtens
#undef Need_Hexdig
#undef USC
#undef NAN_WORD0
#undef NAN_WORD1
#undef ULbits
#undef kshift
#undef kmask
#undef Bug
#undef Avoid_Underflow
#undef Sudden_Underflow

} } }  // iv::core::netlib

using iv::core::netlib::dtoa;
using iv::core::netlib::freedtoa;

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
