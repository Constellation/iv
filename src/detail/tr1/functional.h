#ifndef _IV_DETAIL_TR1__H_
#define _IV_DETAIL_TR1__H_
#include "platform.h"
#if defined(IV_USE_BOOST_TR1)
#include <boost/tr1/functional.hpp>
#elif defined(IV_COMPILER_MSVC) && IV_COMPILER_MSVC_10
#include <functional>
#else
#include <tr1/functional>
#endif
#endif  // _IV_DETAIL_TR1_ARRAY_H_
