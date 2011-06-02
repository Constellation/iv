#ifndef _IV_DETAIL_TR1_MEMORY_H_
#define _IV_DETAIL_TR1_MEMORY_H_
#include "platform.h"
#if defined(IV_USE_BOOST_TR1)
#include <boost/tr1/memory.hpp>
#elif defined(IV_COMPILER_MSVC) && IV_COMPILER_MSVC_10
#include <memory>
#else
#include <tr1/memory>
#endif
#endif  // _IV_DETAIL_TR1_MEMORY_H_
