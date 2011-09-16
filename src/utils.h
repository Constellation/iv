#ifndef IV_UTILS_H_
#define IV_UTILS_H_
#include <cstddef>
#include <cstdio>
#include <vector>
#include <string>
#include "debug.h"
#include "detail/cstdint.h"
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
class AlignOfImpl {
 private:
  struct Helper {
    char a_;
    T b_;
  };
 public:
  static const std::size_t value = offsetof(Helper, b_);
};

#define AlignOf(type) ::iv::core::AlignOfImpl<type>::value

#define AlignOffset(offset, alignment) \
    ((size_t)((offset) + ((alignment) - 1)) & ~(size_t)((alignment) - 1))

#define AlignType(offset, type) AlignOffset(offset, AlignOf(type))

#define IV_ROUNDUP(x, y)\
  (((x) + (y - 1)) & ~(y - 1))

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

  static const int kCharAlign    = AlignOf(char);      // NOLINT
  static const int kShortAlign   = AlignOf(short);     // NOLINT
  static const int kIntAlign     = AlignOf(int);       // NOLINT
  static const int kDoubleAlign  = AlignOf(double);    // NOLINT
  static const int kPointerAlign = AlignOf(void*);     // NOLINT
  static const int kIntptrAlign  = AlignOf(intptr_t);  // NOLINT
};

#define UNREACHABLE() assert(!"UNREACHABLE")

// utility functions

inline bool ReadFile(const std::string& filename,
                     std::vector<char>* out, bool output_error = true) {
  if (std::FILE* fp = std::fopen(filename.c_str(), "rb")) {
    std::fseek(fp, 0L, SEEK_END);
    const std::size_t filesize = std::ftell(fp);
    if (filesize) {
      std::rewind(fp);
      const std::size_t offset = out->size();
      out->resize(offset + filesize);
      if (std::fread(out->data() + offset, filesize, 1, fp) < 1) {
        const std::string err = "lv5 can't read \"" + filename + "\"";
        std::perror(err.c_str());
        std::fclose(fp);
        return false;
      }
    }
    std::fclose(fp);
    return true;
  } else {
    if (output_error) {
      const std::string err = "lv5 can't open \"" + filename + "\"";
      std::perror(err.c_str());
    }
    return false;
  }
}

} }  // namespace iv::core
#endif  // IV_UTILS_H_
