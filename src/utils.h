#ifndef _IV_UTILS_H_
#define _IV_UTILS_H_
#include <cstddef>
#include <cassert>
#include <tr1/cstdint>

#ifdef NDEBUG
#undef DEBUG
#else
#define DEBUG
#endif

namespace iv {
namespace core {

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

// alignment hack
// make struct using char (alignment 1 byte) + T (unknown alignment)
// shift 1 byte using char and get struct offset (offsetof returns T alignment)
template <typename T>
class AlignOf {
 private:
  struct Helper {
    char a_;
    T b_;
  };
 public:
  static const std::size_t value = offsetof(Helper, b_);
};

#define AlignOf(type) AlignOf<type>::value

#define AlignOffset(offset, alignment) \
    ((size_t)((offset) + ((alignment) - 1)) & ~(size_t)((alignment) - 1))

#define AlignType(offset, type) AlignOffset(offset, AlignOf(type))

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
  static const int KB = 1 << 10;
  static const int MB = KB << 10;
  static const int GB = MB << 10;
  static const int TB = GB << 10;

  static const int kCharSize     = sizeof(char);      // NOLINT
  static const int kShortSize    = sizeof(short);     // NOLINT
  static const int kIntSize      = sizeof(int);       // NOLINT
  static const int kDoubleSize   = sizeof(double);    // NOLINT
  static const int kPointerSize  = sizeof(void*);     // NOLINT
  static const int kIntptrSize   = sizeof(intptr_t);  // NOLINT

  static const int kCharAlign    = AlignOf(char);      // NOLINT
  static const int kShortAlign   = AlignOf(short);     // NOLINT
  static const int kIntAlign     = AlignOf(int);       // NOLINT
  static const int kDoubleAlign  = AlignOf(double);    // NOLINT
  static const int kPointerAlign = AlignOf(void*);     // NOLINT
  static const int kIntptrAlign  = AlignOf(intptr_t);  // NOLINT
};

#define VOID_POINTER void*
#define POINTERSIZE (sizeof(POINTER))
#define UNREACHABLE() assert(!"UNREACHABLE")

} }  // namespace iv::core
#endif  // _IV_UTILS_H_
