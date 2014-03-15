#ifndef IV_PHONIC_SOURCE_H_
#define IV_PHONIC_SOURCE_H_
#include <cstddef>
#include <cassert>
#include <string>
#include <tr1/cstdint>
#include <iv/string_view.h>
#include <iv/none.h>
namespace iv {
namespace phonic {
namespace detail {
template<typename T>
class FilenameData {
 public:
  static const std::string kFilename;
};

template<typename T>
const std::string FilenameData<T>::kFilename = "<anonymous>";

}  // namespace iv::phonic::detail

typedef detail::FilenameData<core::None> FilenameData;

class AsciiSource {
 public:
  static const int kEOS = -1;
  explicit AsciiSource(const char* str)
    : source_(str) {
  }
  ~AsciiSource() { }

  inline char16_t Get(std::size_t pos) const {
    assert(pos < size());
    return source_[pos];
  }
  inline std::size_t size() const {
    return source_.size();
  }
  inline const std::string& filename() const {
    return FilenameData::kFilename;
  }
  inline core::u16string_view SubString(
      std::size_t n, std::size_t len = std::string::npos) const {
    return core::u16string_view();
  }

 private:
  std::string source_;
};

class UTF16Source {
 public:
  static const int kEOS = -1;
  explicit UTF16Source(const char* str, std::size_t len)
    : buf_(reinterpret_cast<const uint16_t*>(str)),
      size_(len / 2) {
      assert(len % 2 == 0);
  }
  ~UTF16Source() { }

  inline char16_t Get(std::size_t pos) const {
    assert(pos < size());
    return *(buf_ + pos);
  }
  inline std::size_t size() const {
    return size_;
  }
  inline const std::string& filename() const {
    return FilenameData::kFilename;
  }
  inline core::u16string_view SubString(
      std::size_t n, std::size_t len = std::string::npos) const {
    return core::u16string_view();
  }

 private:
  const uint16_t* buf_;
  std::size_t size_;
};

class UTF16LESource {
 public:
  static const int kEOS = -1;
  explicit UTF16LESource(const char* str, std::size_t len)
    : buf_(str),
      size_(len / 2) {
      assert(len % 2 == 0);
  }
  ~UTF16LESource() { }

  inline char16_t Get(std::size_t pos) const {
    assert(pos < size());
    return (*(buf_ + pos*2)) | (*(buf_ + pos*2 + 1) << 8);
  }
  inline std::size_t size() const {
    return size_;
  }
  inline const std::string& filename() const {
    return FilenameData::kFilename;
  }
  inline core::u16string_view SubString(
      std::size_t n, std::size_t len = std::string::npos) const {
    return core::u16string_view();
  }

 private:
  const char* buf_;
  std::size_t size_;
};

class UTF16BESource {
 public:
  static const int kEOS = -1;
  explicit UTF16BESource(const char* str, std::size_t len)
    : buf_(str),
      size_(len / 2) {
      assert(len % 2 == 0);
  }
  ~UTF16BESource() { }

  inline char16_t Get(std::size_t pos) const {
    assert(pos < size());
    return (*(buf_ + pos*2) << 8) | (*(buf_ + pos*2 + 1));
  }
  inline std::size_t size() const {
    return size_;
  }
  inline const std::string& filename() const {
    return FilenameData::kFilename;
  }
  inline core::u16string_view SubString(
      std::size_t n, std::size_t len = std::string::npos) const {
    return core::u16string_view();
  }

 private:
  const char* buf_;
  std::size_t size_;
};

} }  // namespace iv::phonic
#endif  // IV_PHONIC_SOURCE_H_
