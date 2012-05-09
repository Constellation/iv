#ifndef IV_I18N_LOCALE_H_
#define IV_I18N_LOCALE_H_
#include <iv/ustringpiece.h>
#include <iv/fixed_string.h>
namespace iv {
namespace core {
namespace i18n {

class LanguageTagScanner;

class Locale {
 public:
  friend class LanguageTagScanner;

  typedef std::vector<std::string> Vector;
  typedef std::multimap<char, std::string> Map;

  // currently not used
  typedef BasicFixedString<char, 3> ExtLang;
  typedef BasicFixedString<char, 8> Language;
  typedef BasicFixedString<char, 4> Script;
  typedef BasicFixedString<char, 3> Region;
  typedef BasicFixedString<char, 8> Variant;
  typedef BasicFixedString<char, 8> Extension;

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
