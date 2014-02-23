#ifndef IV_UTILS_H_
#define IV_UTILS_H_
#include <cstddef>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <iv/debug.h>
#include <iv/arith.h>
#include <iv/platform.h>
#include <iv/detail/cstdint.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/mie/string.hpp>
#endif
namespace iv {
namespace core {

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define IV_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

// alignment hack
// make struct using char (alignment 1 byte) + T (unknown alignment)
// shift 1 byte using char and get struct offset (offsetof returns T alignment)
template <typename T>
class AlignOfImpl {
 private:
  struct Helper {
    char a_;
    T b_;
  };
 public:
  static const std::size_t value = offsetof(Helper, b_);
};

#define IV_ALIGN_OF(type) ::iv::core::AlignOfImpl<type>::value

#define IV_ALIGN_OFFSET(offset, alignment) \
    ((size_t)((offset) + ((alignment) - 1)) & ~(size_t)((alignment) - 1))

#define IV_ALIGN_TYPE(offset, type) IV_ALIGNED_OFFSET(offset, IV_ALIGN_OF(type))

// see http://www5d.biglobe.ne.jp/~noocyte/Programming/BigAlignmentBlock.html
#define IV_ALIGNED_SIZE(size, alignment) ((size) + (alignment) - 1)
#define IV_ALIGNED_ADDRESS(address, alignment)\
  ((address + (alignment - 1)) & ~(alignment - 1))

// only 2^n and unsigned
#define IV_ROUNDUP(x, y) (((x) + (y - 1)) & ~(y - 1))

// only 2^n and unsinged
#define IV_ROUNDDOWN(x, y) ((x) & (-(y)))

// OFFSETOF for C++ classes (non-POD)
// http://www.kijineko.co.jp/tech/cpptempls/struct/offsetof.html
// This macro purges '&' operator override,
// and return offset value of non-POD class.
// IV_DUMMY provides dummy address, that is not used in other places.
#define IV_OFFSETOF(type, member) (reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<type*>(0x2000)->member)) - 0x2000) /* NOLINT */

#define IV_CAST_OFFSET(from, to)\
/* NOLINT */  (reinterpret_cast<uintptr_t>(static_cast<to>((reinterpret_cast<from>(0x2000)))) - 0x2000)

#define IV_TO_STRING_IMPL(s) #s
#define IV_TO_STRING(s) IV_TO_STRING_IMPL(s)

template<class T>
T LowestOneBit(T value) {
  return value & (~value + 1u);
}

template <typename T>
std::size_t PtrAlignOf(T* value) {
  return static_cast<std::size_t>(
      LowestOneBit(reinterpret_cast<uintptr_t>(value)));
}

// ptr returnd by malloc guaranteed that any type can be set
// so ptr (void*) is the most biggest alignment of all
class Size {
 public:
  static const std::size_t KB = 1 << 10;
  static const std::size_t MB = KB << 10;
  static const std::size_t GB = MB << 10;

  static const int kCharSize     = sizeof(char);      // NOLINT
  static const int kShortSize    = sizeof(short);     // NOLINT
  static const int kIntSize      = sizeof(int);       // NOLINT
  static const int kDoubleSize   = sizeof(double);    // NOLINT
  static const int kPointerSize  = sizeof(void*);     // NOLINT
  static const int kIntptrSize   = sizeof(intptr_t);  // NOLINT

  static const int kCharAlign    = IV_ALIGN_OF(char);      // NOLINT
  static const int kShortAlign   = IV_ALIGN_OF(short);     // NOLINT
  static const int kIntAlign     = IV_ALIGN_OF(int);       // NOLINT
  static const int kDoubleAlign  = IV_ALIGN_OF(double);    // NOLINT
  static const int kPointerAlign = IV_ALIGN_OF(void*);     // NOLINT
  static const int kIntptrAlign  = IV_ALIGN_OF(intptr_t);  // NOLINT
};

#define UNREACHABLE() assert(!"UNREACHABLE")

template<typename LIter, typename RIter>
inline int CompareIterators(LIter lit, LIter llast, RIter rit, RIter rlast) {
  while (lit != llast && rit != rlast) {
    if (*lit != *rit) {
      return (*lit < *rit) ? -1 : 1;
    }
    ++lit;
    ++rit;
  }

  if (lit == llast) {
    if (rit == rlast) {
      return 0;
    } else {
      return -1;
    }
  } else {
    return 1;
  }
}

inline std::size_t NextCapacity(std::size_t capacity) {
  if (capacity == 0) {
    return 0;
  }
  if (capacity < 8) {
    return 8;
  }
  return math::CLP2(capacity);
}

template<class Vector>
inline void ShrinkToFit(Vector& vec) {  // NOLINT
#if defined(IV_CXX11)
  vec.shrink_to_fit();
#else
  Vector(vec).swap(vec);
#endif
}

template<typename Iter, typename Iter2>
inline Iter Search(Iter i, Iter iz, Iter2 j, Iter2 jz) {
  return std::search(i, iz, j, jz);
}

#if defined(IV_ENABLE_JIT)
template<>
inline const char* Search<const char*, const char*>(
    const char* i, const char* iz,
    const char* j, const char* jz) {
  static const bool enabled = mie::isAvailableSSE42();
  return enabled ? mie::findStr(i, iz, j, jz - j) : std::search(i, iz, j, jz);
}
#endif

} }  // namespace iv::core
#endif  // IV_UTILS_H_
