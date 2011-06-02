#ifndef _IV_DETAIL_TR1_TUPLE_H_
#define _IV_DETAIL_TR1_TUPLE_H_
#include "platform.h"
#if defined(IV_USE_BOOST_TR1)
#include <boost/tr1/tuple.hpp>
#elif defined(IV_COMPILER_MSVC) && IV_COMPILER_MSVC_10
#include <tuple>
#else
#include <tr1/tuple>
#endif
#endif  // _IV_DETAIL_TR1_TUPLE_H_
