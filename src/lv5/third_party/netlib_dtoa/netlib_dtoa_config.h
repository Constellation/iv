#ifndef NETLIB_DTOA_CONFIG_H_
#define NETLIB_DTOA_CONFIG_H_
#include "byteorder.h"

#ifdef IV_IS_LITTLE_ENDIAN
#define IEEE_8087
#else
#define IEEE_MC68k
#endif

#endif  // NETLIB_DTOA_CONFIG_H_
