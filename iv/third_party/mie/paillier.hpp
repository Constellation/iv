#pragma once
/**
    @file
    @brief paillier encryption
    @author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://www.opensource.org/licenses/bsd-license.php
*/
#include <fstream>
#include <vector>
#include <mie/gmp_util.hpp>
#include <cybozu/random_generator.hpp>

namespace mie { namespace paillier {

template<class RG>
void getRandomInt(mpz_class& z, size_t bitLen, RG& rg)
{
	const size_t rem = bitLen & 31;
	const size_t n = (bitLen + 31) / 32;
	std::vector<unsigned int> buf(n);
	for (size_t i = 0; i < n; i++) {
		buf[i] = rg();
	}
	if (rem > 0) buf[n - 1] &= (1U << rem) - 1;
	buf[n - 1] |= 1U << rem;
	Gmp::setRaw(z, &buf[0], n);
}

template<class RG>
void getRandomPrime(mpz_class& z, size_t bitLen, RG& rg)
{
	do {
		getRandomInt(z, bitLen, rg);
	} while (!Gmp::isPrime(z));
}

template<class T>
struct LoadSave {
	void load(const std::string& fileName)
	{
		std::ifstream ifs(fileName.c_str(), std::ios::binary);
		if (!ifs) throw cybozu::Exception("load:can't open") << fileName;
		ifs >> static_cast<T&>(*this);
	}
	void save(const std::string& fileName) const
	{
		std::ofstream ofs(fileName.c_str(), std::ios::binary);
		if (!ofs) throw cybozu::Exception("save:can't open") << fileName;
		ofs << static_cast<const T&>(*this);
	}
};

class PublicKey : public LoadSave<PublicKey> {
	mpz_class n;
	size_t nLen;
	mpz_class nn; // n^2
	mpz_class g; // n + 1
	// call finish after setting n
	void finish()
	{
		nLen = Gmp::getBitLen(n);
		nn = n * n;
		g = n + 1;
	}
	friend class PrivateKey;
public:
	const mpz_class& getN() const { return n; }
	friend inline std::istream& operator>>(std::istream& is, PublicKey& self)
	{
		is >> std::hex >> self.n;
		self.finish();
		return is;
	}
	friend inline std::ostream& operator<<(std::ostream& os, const PublicKey& self)
	{
		os << std::hex << self.n;
		return os;
	}
	void L(mpz_class& y, const mpz_class& x) const
	{
		y = x - 1;
		y /= n;
	}
	void mul(mpz_class& z, const mpz_class& x, const mpz_class &y) const
	{
		z = x * y;
		z %= nn;
	}
	void pow(mpz_class& z, const mpz_class& x, const mpz_class& y) const
	{
		Gmp::powMod(z, x, y, nn);
	}
	/*
		encMsg = (g^msg) (r^n) mod n^2
	*/
	template<class RG>
	void enc(mpz_class& encMsg, const mpz_class& msg, RG& rg) const
	{
		if (msg >= n) throw cybozu::Exception("too large msg");
		mpz_class r;
		getRandomInt(r, nLen * 2 - 2, rg);
		Gmp::powMod(r, r, n, nn);

		Gmp::powMod(encMsg, g, msg, nn);
		mul(encMsg, encMsg, r);
	}
	void enc(mpz_class& encMsg, const mpz_class& msg) const
	{
		cybozu::RandomGenerator rg;
		enc(encMsg, msg, rg);
	}
};

class PrivateKey : public LoadSave<PrivateKey> {
	mpz_class p;
	mpz_class q;
	mpz_class lambda; // lcm(p-1, q-1)
	PublicKey pub;
	mpz_class x; // 1 / L(g^lambda mod n^2) mod n

	// call finish after setting p, q
	void finish()
	{
		pub.n = p * q;
		pub.finish();
		Gmp::lcm(lambda, p - 1, q - 1);
		// x = (n + 1)^lambda mod n^2
		Gmp::powMod(x, pub.g, lambda, pub.nn);
		pub.L(x, x);
		// 1 / L() mod n
		Gmp::invMod(x, x, pub.n);
	}
public:
	template<class RG>
	void init(size_t keyLen, RG& rg)
	{
		do {
			getRandomPrime(p, (keyLen + 1) / 2, rg);
			getRandomPrime(q, (keyLen + 1) / 2, rg);
			pub.n = p * q;
		} while (Gmp::getBitLen(pub.n) < keyLen);
		finish();
		pub.finish();
	}
	void init(size_t keyLen)
	{
		cybozu::RandomGenerator rg;
		init(keyLen, rg);
	}
	const PublicKey& getPublicKey() const { return pub; }
	friend inline std::istream& operator>>(std::istream& is, PrivateKey& self)
	{
		is >> std::hex >> self.p >> self.q;
		self.finish();
		return is;
	}
	friend inline std::ostream& operator<<(std::ostream& os, const PrivateKey& self)
	{
		os << std::hex << self.p << " " << self.q;
		return os;
	}
	/*
		decMsg = L(encMsg^lambda mod n^2) / L(g^lambda mod n^2) mod n
	*/
	void dec(mpz_class& decMsg, const mpz_class& encMsg) const
	{
		Gmp::powMod(decMsg, encMsg, lambda, pub.nn);
		pub.L(decMsg, decMsg);
		decMsg *= x;
		decMsg %= pub.n;
	}
};

} } // mie::paillier

