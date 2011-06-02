#ifndef _IV_DETAIL_TR1_UNORDERED_SET_H_
#define _IV_DETAIL_TR1_UNORDERED_SET_H_
#include "platform.h"
#if defined(IV_USE_BOOST_TR1)
#include <boost/tr1/unordered_set.hpp>
#elif defined(IV_COMPILER_MSVC) && IV_COMPILER_MSVC_10
#include <unordered_set>
#else
#include <tr1/unordered_set>
#endif
#endif  // _IV_DETAIL_TR1_UNORDERED_SET_H_
