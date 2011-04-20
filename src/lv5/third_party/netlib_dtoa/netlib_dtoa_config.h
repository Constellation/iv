#ifndef _IV_BYTEORDER_H_
#define _IV_BYTEORDER_H_

#include <sys/types.h>

#if defined(__FreeBSD__) || defined(__APPLE__)

#if defined(__FreeBSD__)
#include <sys/endian.h>
#else
#include <machine/endian.h>
#endif  // defined(__FreeBSD__)

#define __BYTE_ORDER BYTE_ORDER
#define __LITTLE_ENDIAN LITTLE_ENDIAN
#define __BIG_ENDIAN BIG_ENDIAN

#elif defined(__GNUC__)

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

#ifdef IV_IS_LITTLE_ENDIAN
#define IEEE_8087
#else
#define IEEE_MC68k
#endif

#endif  // _IV_BYTEORDER_H_
