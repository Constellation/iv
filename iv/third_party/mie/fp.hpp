#pragma once
/**
	@file
	@brief Fp
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <sstream>
#include <vector>
#include <cybozu/hash.hpp>
#include <cybozu/itoa.hpp>
#include <cybozu/atoi.hpp>
#include <mie/operator.hpp>
#include <mie/power.hpp>

namespace mie {

namespace fp {

/*
	convert x[0..n) to hex string
	start "0x" if withPrefix
*/
template<class T>
void toStr16(std::string& str, const T *x, size_t n, bool withPrefix = false)
{
	size_t fullN = 0;
	if (n > 1) {
		size_t pos = n - 1;
		while (pos > 0) {
			if (x[pos]) break;
			pos--;
		}
		if (pos > 0) fullN = pos;
	}
	const T v = n == 0 ? 0 : x[fullN];
	const size_t topLen = cybozu::getHexLength(v);
	const size_t startPos = withPrefix ? 2 : 0;
	const size_t lenT = sizeof(T) * 2;
	str.resize(startPos + fullN * lenT + topLen);
	if (withPrefix) {
		str[0] = '0';
		str[1] = 'x';
	}
	cybozu::itohex(&str[startPos], topLen, v, false);
	for (size_t i = 0; i < fullN; i++) {
		cybozu::itohex(&str[startPos + topLen + i * lenT], lenT, x[fullN - 1 - i], false);
	}
}

/*
	convert x[0..n) to bin string
	start "0b" if withPrefix
*/
template<class T>
void toStr2(std::string& str, const T *x, size_t n, bool withPrefix)
{
	size_t fullN = 0;
	if (n > 1) {
		size_t pos = n - 1;
		while (pos > 0) {
			if (x[pos]) break;
			pos--;
		}
		if (pos > 0) fullN = pos;
	}
	const T v = n == 0 ? 0 : x[fullN];
	const size_t topLen = cybozu::getBinLength(v);
	const size_t startPos = withPrefix ? 2 : 0;
	const size_t lenT = sizeof(T) * 8;
	str.resize(startPos + fullN * lenT + topLen);
	if (withPrefix) {
		str[0] = '0';
		str[1] = 'b';
	}
	cybozu::itobin(&str[startPos], topLen, v);
	for (size_t i = 0; i < fullN; i++) {
		cybozu::itobin(&str[startPos + topLen + i * lenT], lenT, x[fullN - 1 - i]);
	}
}

/*
	convert hex string to x[0..xn)
	hex string = [0-9a-fA-F]+
*/
template<class T>
void fromStr16(T *x, size_t xn, const char *str, size_t strLen)
{
	if (strLen == 0) throw cybozu::Exception("fp:fromStr16:strLen is zero");
	const size_t unitLen = sizeof(T) * 2;
	const size_t q = strLen / unitLen;
	const size_t r = strLen % unitLen;
	const size_t requireSize = q + (r ? 1 : 0);
	if (xn < requireSize) throw cybozu::Exception("fp:fromStr16:short size") << xn << requireSize;
	for (size_t i = 0; i < q; i++) {
		bool b;
		x[i] = cybozu::hextoi(&b, &str[r + (q - 1 - i) * unitLen], unitLen);
		if (!b) throw cybozu::Exception("fp:fromStr16:bad char") << cybozu::exception::makeString(str, strLen);
	}
	if (r) {
		bool b;
		x[q] = cybozu::hextoi(&b, str, r);
		if (!b) throw cybozu::Exception("fp:fromStr16:bad char") << cybozu::exception::makeString(str, strLen);
	}
	for (size_t i = requireSize; i < xn; i++) x[i] = 0;
}

/*
	@param base [inout]
*/
inline const char *verifyStr(bool *isMinus, int *base, const std::string& str)
{
	const char *p = str.c_str();
	if (*p == '-') {
		*isMinus = true;
		p++;
	} else {
		*isMinus = false;
	}
	if (p[0] == '0') {
		if (p[1] == 'x') {
			if (*base != 0 && *base != 16) {
				throw cybozu::Exception("fp:verifyStr:bad base") << *base << str;
			}
			*base = 16;
			p += 2;
		} else if (p[1] == 'b') {
			if (*base != 0 && *base != 2) {
				throw cybozu::Exception("fp:verifyStr:bad base") << *base << str;
			}
			*base = 2;
			p += 2;
		}
	}
	if (*base == 0) *base = 10;
	if (*p == '\0') throw cybozu::Exception("fp:verifyStr:str is empty");
	return p;
}

inline size_t getRoundNum(size_t x, size_t size)
{
	return (x + size - 1) / size;
}

template<class T>
void maskBuffer(T* buf, size_t bufN, size_t bitLen)
{
	size_t bitSizeT = sizeof(T) * 8;
	if (bitLen >= bitSizeT * bufN) return;
	const size_t rem = bitLen & (bitSizeT - 1);
	const size_t n = getRoundNum(bitLen, bitSizeT);
	if (rem > 0) buf[n - 1] &= (T(1) << rem) - 1;
}

template<class RG>
void setRand(std::vector<uint32_t>& buf, RG& rg, size_t bitLen)
{
	const size_t n = getRoundNum(bitLen, 32);
	buf.resize(n);
	if (n == 0) return;
	rg.read(&buf[0], n);
}

template<class S>
void setMaskedRaw(std::vector<S>& buf, const S *inBuf, size_t n, size_t bitLen)
{
	bitLen = std::min(bitLen, sizeof(S) * 8 * n);
	buf.resize(getRoundNum(bitLen, sizeof(S) * 8));
	if (buf.empyt()) return;
	std::copy(inBuf, inBuf + buf.size(), &buf[0]);
	fp::maskBuffer(&buf[0], buf.size(), bitLen);
}

} // fp

namespace fp_local {
struct TagDefault;
} // fp_local

template<class T, class tag = fp_local::TagDefault>
class FpT : public ope::comparable<FpT<T, tag>,
	ope::addsub<FpT<T, tag>,
	ope::mulable<FpT<T, tag>,
	ope::invertible<FpT<T, tag>,
	ope::hasNegative<FpT<T, tag>,
	ope::hasIO<FpT<T, tag> > > > > > > {
public:
	typedef typename T::BlockType BlockType;
	typedef typename T::ImplType ImplType;
	FpT() {}
	FpT(int x) { operator=(x); }
	FpT(uint64_t x) { operator=(x); }
	explicit FpT(const std::string& str, int base = 0)
	{
		fromStr(str, base);
	}
	FpT(const FpT& x)
		: v(x.v)
	{
	}
	FpT& operator=(const FpT& x)
	{
		v = x.v; return *this;
	}
	FpT& operator=(int x)
	{
		if (x >= 0) {
			v = x;
		} else {
			assert(m_ >= -x);
			T::sub(v, m_, -x);
		}
		return *this;
	}
	FpT& operator=(uint64_t x)
	{
		T::set(v, x);
		return *this;
	}
	void fromStr(const std::string& str, int base = 0)
	{
		bool isMinus;
		inFromStr(v, &isMinus, str, base);
		if (v >= m_) throw cybozu::Exception("fp:FpT:fromStr:large str") << str;
		if (isMinus) {
			neg(*this, *this);
		}
	}
	void set(const std::string& str, int base = 10) { fromStr(str, base); }
	void toStr(std::string& str, int base = 10, bool withPrefix = false) const
	{
		switch (base) {
		case 10:
			T::toStr(str, v, base);
			return;
		case 16:
			mie::fp::toStr16(str, getBlock(*this), getBlockSize(*this), withPrefix);
			return;
		case 2:
			mie::fp::toStr2(str, getBlock(*this), getBlockSize(*this), withPrefix);
			return;
		default:
			throw cybozu::Exception("fp:FpT:toStr:bad base") << base;
		}
	}
	std::string toStr(int base = 10, bool withPrefix = false) const
	{
		std::string str;
		toStr(str, base, withPrefix);
		return str;
	}
	void clear()
	{
		T::clear(v);
	}
	template<class RG>
	void initRand(RG& rg, size_t)
	{
		std::vector<uint32_t> buf(fp::getRoundNum(modBitLen_, 32));
		assert(!buf.empty());
		rg.read(&buf[0], buf.size());;
		setMaskMod(buf);
	}
	/*
		ignore the value of inBuf over modulo
	*/
	template<class S>
	void setRaw(const S *inBuf, size_t n)
	{
		n = std::min(n, fp::getRoundNum(modBitLen_, sizeof(S) * 8));
		if (n == 0) {
			clear();
			return;
		}
		std::vector<S> buf(n);
		std::copy(inBuf, inBuf + buf.size(), &buf[0]);
		setMaskMod(buf);
	}
	static inline void setModulo(const std::string& mstr, int base = 0)
	{
		bool isMinus;
		inFromStr(m_, &isMinus, mstr, base);
		if (isMinus) throw cybozu::Exception("fp:FpT:setModulo:mstr is not minus") << mstr;
		modBitLen_ = T::getBitLen(m_);
		opt_.init(m_);
	}
	static inline void getModulo(std::string& mstr)
	{
		T::toStr(mstr, m_);
	}
	static inline void add(FpT& z, const FpT& x, const FpT& y) { T::addMod(z.v, x.v, y.v, m_); }
	static inline void sub(FpT& z, const FpT& x, const FpT& y) { T::subMod(z.v, x.v, y.v, m_); }
	static inline void mul(FpT& z, const FpT& x, const FpT& y)
	{
		if (opt_.hasMulMod()) {
			opt_.mulMod(z.v, x.v, y.v);
		} else {
			T::mulMod(z.v, x.v, y.v, m_);
		}
	}
	static inline void square(FpT& z, const FpT& x) { T::squareMod(z.v, x.v, m_); }

	static inline void add(FpT& z, const FpT& x, unsigned int y) { T::addMod(z.v, x.v, y, m_); }
	static inline void sub(FpT& z, const FpT& x, unsigned int y) { T::subMod(z.v, x.v, y, m_); }
	static inline void mul(FpT& z, const FpT& x, unsigned int y)
	{
		if (opt_.hasMulMod()) {
			opt_.mulMod(z.v, x.v, y);
		} else {
			T::mulMod(z.v, x.v, y, m_);
		}
	}

	static inline void inv(FpT& z, const FpT& x) { T::invMod(z.v, x.v, m_); }
	static inline void div(FpT& z, const FpT& x, const FpT& y)
	{
		ImplType rev;
		T::invMod(rev, y.v, m_);
		T::mulMod(z.v, x.v, rev, m_);
	}
	static inline void neg(FpT& z, const FpT& x)
	{
		if (x.isZero()) {
			z.clear();
		} else {
			T::sub(z.v, m_, x.v);
		}
	}
	static inline BlockType getBlock(const FpT& x, size_t i)
	{
		return T::getBlock(x.v, i);
	}
	static inline const BlockType *getBlock(const FpT& x)
	{
		return T::getBlock(x.v);
	}
	static inline size_t getBlockSize(const FpT& x)
	{
		return T::getBlockSize(x.v);
	}
	static inline int compare(const FpT& x, const FpT& y)
	{
		return T::compare(x.v, y.v);
	}
	static inline bool isZero(const FpT& x)
	{
		return T::isZero(x.v);
	}
	static inline size_t getBitLen(const FpT& x)
	{
		return T::getBitLen(x.v);
	}
	static inline void shr(FpT& z, const FpT& x, size_t n)
	{
		z.v = x.v >> n;
	}
	bool isZero() const { return isZero(*this); }
	size_t getBitLen() const { return getBitLen(*this); }
	template<class N>
	static void power(FpT& z, const FpT& x, const N& y)
	{
		power_impl::power(z, x, y);
	}
	const ImplType& getInnerValue() const { return v; }
private:
	static ImplType m_;
	static size_t modBitLen_;
	static mie::ope::Optimized<ImplType> opt_;
	ImplType v;
	static inline void inFromStr(ImplType& t, bool *isMinus, const std::string& str, int base)
	{
		const char *p = fp::verifyStr(isMinus, &base, str);
		if (!T::fromStr(t, p, base)) {
			throw cybozu::Exception("fp:FpT:fromStr") << str;
		}
	}
	template<class S>
	void setMaskMod(std::vector<S>& buf)
	{
		assert(buf.size() <= fp::getRoundNum(modBitLen_, sizeof(S) * 8));
		assert(!buf.empty());
		fp::maskBuffer(&buf[0], buf.size(), modBitLen_);
		T::setRaw(v, &buf[0], buf.size());
		if (v >= m_) {
			T::sub(v, v, m_);
		}
		assert(v < m_);
	}
};

template<class T, class tag>
typename T::ImplType FpT<T, tag>::m_;
template<class T, class tag>
size_t FpT<T, tag>::modBitLen_;

template<class T, class tag>
mie::ope::Optimized<typename T::ImplType> FpT<T, tag>::opt_;

} // mie

namespace std { CYBOZU_NAMESPACE_TR1_BEGIN
template<class T> struct hash;

template<class T, class tag>
struct hash<mie::FpT<T, tag> > : public std::unary_function<mie::FpT<T, tag>, size_t> {
	size_t operator()(const mie::FpT<T, tag>& x, uint64_t v = 0) const
	{
		typedef mie::FpT<T, tag> Fp;
		size_t n = Fp::getBlockSize(x);
		const typename Fp::BlockType *p = Fp::getBlock(x);
		return static_cast<size_t>(cybozu::hash64(p, n, v));
	}
};

CYBOZU_NAMESPACE_TR1_END } // std::tr1

