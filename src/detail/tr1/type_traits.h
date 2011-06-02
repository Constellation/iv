#ifndef _IV_DETAIL_TR1_TYPE_TRAITS_H_
#define _IV_DETAIL_TR1_TYPE_TRAITS_H_
#include "platform.h"
#if defined(IV_USE_BOOST_TR1)
#include <boost/tr1/type_traits.hpp>
#elif defined(IV_COMPILER_MSVC) && IV_COMPILER_MSVC_10
#include <type_traits>
#else
#include <tr1/type_traits>
#endif
#endif  // _IV_DETAIL_TR1_TYPE_TRAITS_H_
