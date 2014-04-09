#pragma once
/**
	@file
	@brief functions like C99 for VC
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <math.h>
#include <float.h>

#if defined(_MSC_VER) && (_MSC_VER < 1800)
const int FP_NAN = 0;
const int FP_INFINITE = 1;
const int FP_ZERO = 2;
const int FP_SUBNORMAL = 3;
const int FP_NORMAL = 4;

inline int fpclassify(double x)
{
	int c = _fpclass(x);
	if (c & (_FPCLASS_NN | _FPCLASS_PN)) return FP_NORMAL;
	if (c & (_FPCLASS_NZ | _FPCLASS_PZ)) return FP_ZERO;
	if (c & (_FPCLASS_ND | _FPCLASS_PD)) return FP_SUBNORMAL;
	if (c & (_FPCLASS_NINF | _FPCLASS_PINF)) return FP_INFINITE;
	// _FPCLASS_SNAN, _FPCLASS_QNAN
	return FP_NAN;
}

inline int isfinite(double x)
{
	int c = _fpclass(x);
	return !(c & (_FPCLASS_SNAN | _FPCLASS_QNAN | _FPCLASS_NINF | _FPCLASS_PINF));
}

inline int isnormal(double x)
{
	int c = _fpclass(x);
	return c & (_FPCLASS_NN | _FPCLASS_PN);
}
inline int isnan(double x) { return _isnan(x); }
inline int isinf(double x)
{
	int c = _fpclass(x);
	if (c & _FPCLASS_NINF) return -1;
	if (c & _FPCLASS_PINF) return 1;
	return 0;
}
#endif
