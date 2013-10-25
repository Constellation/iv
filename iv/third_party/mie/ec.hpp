#pragma once
/**
	@file
	@brief elliptic curve
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <sstream>
#include <cybozu/exception.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>

namespace mie {

#define MIE_EC_USE_JACOBI
/*
	elliptic curve
	y^2 = x^3 + ax + b (affine)
	y^2 = x^3 + az^4 + bz^6 (Jacobi) x = X/Z^2, y = Y/Z^3
*/
template<class _Fp>
class EcT : public ope::addsub<EcT<_Fp>,
	ope::comparable<EcT<_Fp>,
	ope::hasNegative<EcT<_Fp> > > > {
	enum {
		zero,
		minus3,
		generic
	};
public:
	typedef _Fp Fp;
#ifdef MIE_EC_USE_JACOBI
	mutable Fp x, y, z;
#else
	Fp x, y;
	bool inf_;
#endif
	static Fp a_;
	static Fp b_;
	static int specialA_;
#ifdef MIE_EC_USE_JACOBI
	EcT() { z.clear(); }
#else
	EcT() : inf_(true) {}
#endif
	EcT(const Fp& _x, const Fp& _y)
	{
		set(_x, _y);
	}
	void normalize() const
	{
#ifdef MIE_EC_USE_JACOBI
		if (isZero() || z == 1) return;
		Fp rz, rz2;
		Fp::inv(rz, z);
		rz2 = rz * rz;
		x *= rz2;
		y *= rz2 * rz;
		z = 1;
#endif
	}

	static inline void setParam(const std::string& astr, const std::string& bstr)
	{
		a_.fromStr(astr);
		b_.fromStr(bstr);
		if (a_.isZero()) {
			specialA_ = zero;
		} else if (a_ == -3) {
			specialA_ = minus3;
		} else {
			specialA_ = generic;
		}
		printf("specialA_=%d\n", specialA_);
	}
	static inline bool isValid(const Fp& _x, const Fp& _y)
	{
		return _y * _y == (_x * _x + a_) * _x + b_;
	}
	void set(const Fp& _x, const Fp& _y, bool verify = true)
	{
		if (verify && !isValid(_x, _y)) throw cybozu::Exception("ec:EcT:set") << _x << _y;
		x = _x; y = _y;
#ifdef MIE_EC_USE_JACOBI
		z = 1;
#else
		inf_ = false;
#endif
	}
	void clear()
	{
#ifdef MIE_EC_USE_JACOBI
		z = 0;
#else
		inf_ = true;
#endif
		x.clear();
		y.clear();
	}

	static inline void dbl(EcT& R, const EcT& P, bool verifyInf = true)
	{
		if (verifyInf) {
			if (P.isZero()) {
				R.clear(); return;
			}
		}
#ifdef MIE_EC_USE_JACOBI
		Fp S, M, t, y2;
		Fp::square(y2, P.y);
		Fp::mul(S, P.x, y2);
		S += S;
		S += S;
		Fp::square(M, P.x);
		switch (specialA_) {
		case zero:
			Fp::add(t, M, M);
			M += t;
			break;
		case minus3:
			Fp::square(t, P.z);
			Fp::square(t, t);
			M -= t;
			Fp::add(t, M, M);
			M += t;
			break;
		case generic:
		default:
			Fp::square(t, P.z);
			Fp::square(t, t);
			t *= a_;
			t += M;
			M += M;
			M += t;
			break;
		}
		Fp::square(R.x, M);
		R.x -= S;
		R.x -= S;
		Fp::mul(R.z, P.y, P.z);
		R.z += R.z;
		Fp::square(y2, y2);
		y2 += y2;
		y2 += y2;
		y2 += y2;
		Fp::sub(R.y, S, R.x);
		R.y *= M;
		R.y -= y2;
#else
		Fp t, s;
		Fp::square(t, P.x);
		Fp::add(s, t, t);
		t += s;
		t += a_;
		Fp::add(s, P.y, P.y);
		t /= s;
		Fp::square(s, t);
		s -= P.x;
		Fp x3;
		Fp::sub(x3, s, P.x);
		Fp::sub(s, P.x, x3);
		s *= t;
		Fp::sub(R.y, s, P.y);
		R.x = x3;
		R.inf_ = false;
#endif
	}
	static inline void add(EcT& R, const EcT& P, const EcT& Q)
	{
		if (P.isZero()) { R = Q; return; }
		if (Q.isZero()) { R = P; return; }
#ifdef MIE_EC_USE_JACOBI
		Fp r, U1, S1, H, H3;
		Fp::square(r, P.z);
		Fp::square(S1, Q.z);
		Fp::mul(U1, P.x, S1);
		Fp::mul(H, Q.x, r);
		H -= U1;
		r *= P.z;
		S1 *= Q.z;
		S1 *= P.y;
		Fp::mul(r, Q.y, r);
		r -= S1;
		if (H.isZero()) {
			if (r.isZero()) {
				dbl(R, P, false);
			} else {
				R.clear();
			}
			return;
		}
		Fp::mul(R.z, P.z, Q.z);
		R.z *= H;
		Fp::square(H3, H); // H^2
		Fp::square(R.y, r); // r^2
		U1 *= H3; // U1 H^2
		H3 *= H; // H^3
		R.y -= U1;
		R.y -= U1;
		Fp::sub(R.x, R.y, H3);
		U1 -= R.x;
		U1 *= r;
		H3 *= S1;
		Fp::sub(R.y, U1, H3);
#else
		Fp t;
		Fp::neg(t, Q.y);
		if (P.y == t) { R.clear(); return; }
		Fp::sub(t, Q.x, P.x);
		if (t.isZero()) {
			dbl(R, P, false);
			return;
		}
		Fp s;
		Fp::sub(s, Q.y, P.y);
		Fp::div(t, s, t);
		R.inf_ = false;
		Fp x3;
		Fp::square(x3, t);
		x3 -= P.x;
		x3 -= Q.x;
		Fp::sub(s, P.x, x3);
		s *= t;
		Fp::sub(R.y, s, P.y);
		R.x = x3;
#endif
	}
	static inline void sub(EcT& R, const EcT& P, const EcT& Q)
	{
#if 0
		if (P.inf_) { neg(R, Q); return; }
		if (Q.inf_) { R = P; return; }
		if (P.y == Q.y) { R.clear(); return; }
		Fp t;
		Fp::sub(t, Q.x, P.x);
		if (t.isZero()) {
			dbl(R, P, false);
			return;
		}
		Fp s;
		Fp::add(s, Q.y, P.y);
		Fp::neg(s, s);
		Fp::div(t, s, t);
		R.inf_ = false;
		Fp x3;
		Fp::mul(x3, t, t);
		x3 -= P.x;
		x3 -= Q.x;
		Fp::sub(s, P.x, x3);
		s *= t;
		Fp::sub(R.y, s, P.y);
		R.x = x3;
#else
		EcT nQ;
		neg(nQ, Q);
		add(R, P, nQ);
#endif
	}
	static inline void neg(EcT& R, const EcT& P)
	{
#ifdef MIE_EC_USE_JACOBI
		R.x = P.x;
		Fp::neg(R.y, P.y);
		R.z = P.z;
#else
		if (P.isZero()) {
			R.clear();
			return;
		}
		R.inf_ = false;
		R.x = P.x;
		Fp::neg(R.y, P.y);
#endif
	}
	template<class N>
	static inline void power(EcT& z, const EcT& x, const N& y)
	{
		power_impl::power(z, x, y);
	}
	/*
		0 <= P for any P
		(Px, Py) <= (P'x, P'y) iff Px < P'x or Px == P'x and Py <= P'y
	*/
	static inline int compare(const EcT& P, const EcT& Q)
	{
		P.normalize();
		Q.normalize();
		if (P.isZero()) {
			if (Q.isZero()) return 0;
			return -1;
		}
		if (Q.isZero()) return 1;
		int c = _Fp::compare(P.x, Q.x);
		if (c > 0) return 1;
		if (c < 0) return -1;
		return _Fp::compare(P.y, Q.y);
	}
	bool isZero() const
	{
#ifdef MIE_EC_USE_JACOBI
		return z.isZero();
#else
		return inf_;
#endif
	}
	friend inline std::ostream& operator<<(std::ostream& os, const EcT& self)
	{
		if (self.isZero()) {
			return os << '0';
		} else {
			self.normalize();
			return os << self.x.toStr(16) << '_' << self.y.toStr(16);
		}
	}
	friend inline std::istream& operator>>(std::istream& is, EcT& self)
	{
		std::string str;
		is >> str;
		if (str == "0") {
			self.clear();
		} else {
#ifdef MIE_EC_USE_JACOBI
			self.z = 1;
#else
			self.inf_ = false;
#endif
			size_t pos = str.find('_');
			if (pos == std::string::npos) throw cybozu::Exception("EcT:bad format") << str;
			str[pos] = '\0';
			self.x.fromStr(&str[0], 16);
			self.y.fromStr(&str[pos + 1], 16);
		}
		return is;
	}
};

template<class T>
struct TagMultiGr<EcT<T> > {
	static void square(EcT<T>& z, const EcT<T>& x)
	{
		EcT<T>::dbl(z, x);
	}
	static void mul(EcT<T>& z, const EcT<T>& x, const EcT<T>& y)
	{
		EcT<T>::add(z, x, y);
	}
	static void inv(EcT<T>& z, const EcT<T>& x)
	{
		EcT<T>::neg(z, x);
	}
	static void div(EcT<T>& z, const EcT<T>& x, const EcT<T>& y)
	{
		EcT<T>::sub(z, x, y);
	}
	static void init(EcT<T>& x)
	{
		x.clear();
	}
};

template<class _Fp> _Fp EcT<_Fp>::a_;
template<class _Fp> _Fp EcT<_Fp>::b_;
template<class _Fp> int EcT<_Fp>::specialA_;

struct EcParam {
	const char *name;
	const char *p;
	const char *a;
	const char *b;
	const char *gx;
	const char *gy;
	const char *n;
	size_t bitLen; // bit length of p
};

} // mie

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN

template<class T> struct hash;

template<class _Fp>
struct hash<mie::EcT<_Fp> > : public std::unary_function<mie::EcT<_Fp>, size_t> {
	size_t operator()(const mie::EcT<_Fp>& P) const
	{
		if (P.isZero()) return 0;
		P.normalize();
		uint64_t v = hash<_Fp>()(P.x);
		v = hash<_Fp>()(P.y, v);
		return static_cast<size_t>(v);
	}
};

CYBOZU_NAMESPACE_TR1_END } // std

