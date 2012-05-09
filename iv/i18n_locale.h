#ifndef IV_I18N_LOCALE_H_
#define IV_I18N_LOCALE_H_
#include <iv/ustringpiece.h>
namespace iv {
namespace core {
namespace i18n {

class LanguageTagScanner;

template<std::size_t MAX>
class FixedString {
 public:
  FixedString(const StringPiece& str)
    : size_(str.size()),
      data_() {
    assert(size() <= MAX);
    std::copy(str.begin(), str.end(), data());
    data()[size()] = '\0';
  }

  operator StringPiece() {
    return StringPiece(data(), size());
  }

  std::size_t size() const { return size_; }

  char* data() { return data_; }

  const char* data() const { return data_; }

 private:
  std::size_t size_;
  char data_[MAX + 1];
};

class Locale {
 public:
  friend class LanguageTagScanner;

  typedef std::vector<std::string> Vector;
  typedef std::multimap<char, std::string> Map;

  const std::string& all() const { return all_; }

  const std::string& language() const { return language_; }

  const Vector& extlang() const { return extlang_; }

  const std::string& script() const { return script_; }

  const std::string& region() const { return region_; }

  const Vector& variants() const { return variants_; }

  const Map& extensions() const { return extensions_; }

  const std::string& privateuse() const { return privateuse_; }

 private:
  std::string all_;
  std::string language_;
  Vector extlang_;
  std::string script_;
  std::string region_;
  Vector variants_;
  Map extensions_;
  std::string privateuse_;
};

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_LOCALE_H_
