#ifndef _IV_DETAIL_TR1_CSTDINT_H_
#define _IV_DETAIL_TR1_CSTDINT_H_
#include "platform.h"
#if defined(IV_USE_BOOST_TR1)
#include <boost/cstdint.hpp>
#elif defined(IV_COMPILER_MSVC) && IV_COMPILER_MSVC_10
#include <cstdint>
#else
#include <tr1/cstdint>
#endif
#endif  // _IV_DETAIL_TR1_CSTDINT_H_
