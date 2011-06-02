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

#if !defined(IV_USE_BOOST_TR1) || defined(BOOST_HAS_TR1_HASH)
#define IV_TR1_HASH_NAMESPACE_START std { namespace tr1
#define IV_TR1_HASH_NAMESPACE_END }
#else
#define IV_TR1_HASH_NAMESPACE_START boost
#define IV_TR1_HASH_NAMESPACE_END
#endif  // ifdef BOOST_HAS_TR1_HASH

#endif  // _IV_DETAIL_TR1_ARRAY_H_
