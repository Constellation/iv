#ifndef _IV_DETAIL_TR1_ARRAY_H_
#define _IV_DETAIL_TR1_ARRAY_H_
#include "platform.h"
#if defined(IV_USE_BOOST_TR1)
#include <boost/tr1/array.hpp>
#elif defined(IV_COMPILER_MSVC) && IV_COMPILER_MSVC_10
#include <array>
#else
#include <tr1/array>
#endif
#endif  // _IV_DETAIL_TR1_ARRAY_H_
