#ifndef _IV_BYTEORDER_H_
#define _IV_BYTEORDER_H_

#include <sys/types.h>

#ifdef __GNUC__
#include <endian.h>
#endif

#if !defined(__BYTE_ORDER) || (__BYTE_ORDER != __LITTLE_ENDIAN && __BYTE_ORDER != __BIG_ENDIAN)
#error BYTE_ORDER not defined. you shoud define __LITTLE_ENDIAN or __BIG_ENDIAN
#endif

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define IV_IS_LITTLE_ENDIAN
#else
#define IV_IS_BIG_ENDIAN
#endif

#endif  // _IV_BYTEORDER_H_
