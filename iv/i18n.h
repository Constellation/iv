// utility implementation of i18n
//
// See ECMAScript Internationalization API
//   http://wiki.ecmascript.org/doku.php?id=globalization:globalization
//   http://wiki.ecmascript.org/doku.php?id=globalization:specification_drafts
//
#ifndef IV_I18N_H_
#define IV_I18N_H_
#include <iv/character.h>
#include <iv/i18n_language_tag_verifier.h>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
namespace iv {
namespace core {
namespace i18n {

// Only mapping ASCII characters
// 6.1 Case Sensitivity and Case Mapping
inline uint16_t ToLocaleIdentifierUpperCase(uint16_t ch) {
  if ('a' <= ch && ch <= 'z') {
    return ch - 'a' + 'A';
  }
  return ch;
}

// 6.2.2 IsWellFormedLanguageTag(locale)
// BCP 47 language tag as specified in RFC 5646 section 2.1
//
// See iv/i18n_language_tag_verifier.h
template<typename Iter>
inline bool IsWellFormedLanguageTag(Iter it, Iter last) {
  LanguageTagVerifier<Iter> verifier(it, last);
  return verifier.Verify();
}

inline bool IsWellFormedLanguageTag(const StringPiece& piece) {
  typedef StringPiece::const_iterator Iter;
  LanguageTagVerifier<Iter> verifier(piece.cbegin(), piece.cend());
  return verifier.Verify();
}

inline bool IsWellFormedLanguageTag(const UStringPiece& piece) {
  typedef UStringPiece::const_iterator Iter;
  LanguageTagVerifier<Iter> verifier(piece.cbegin(), piece.cend());
  return verifier.Verify();
}

// 6.2.3 CanonicalizeLanguageTag(locale)
// Returns the canonical and case-regularized form of the locale argument (which
// must be a String value that is a well-formed BCP 47 language tag as verified
// by the IsWellFormedLanguageTag abstract operation).
// Specified in RFC 5646 section 4.5
template<typename Iter>
inline character::locale::Locale CanonicalizeLanguageTag(Iter it, Iter last) {
  // TODO(Constellation) not implemented yet
  return character::locale::EN;
}

// Currency
// based on
//   http://en.wikipedia.org/wiki/ISO_4217
//   CLDR, tools/java/org/unicode/cldr/util/data/ISO4217.txt

// 6.3.1 IsWellFormedCurrencyCode(currency)
template<typename Iter>
inline bool IsWellFormedCurrencyCode(Iter it, Iter last) {
  const std::size_t len = std::distance(it, last);
  if (len != 3) {
    return false;
  }
  for (; it != last; ++it) {
    const uint16_t ch = ToLocaleIdentifierLowerCase(*it);
    if (ch < 'A' || 'Z' < ch) {
      return false;
    }
  }
  return true;
}

class I18N {
 public:
  I18N()
    : default_locale_(character::locale::EN),
      current_locale_(character::locale::EN) {
  }

  // 6.2.4 DefaultLocale()
  // the well-formed (6.2.2) and canonicalized (6.2.3) BCP 47 language tag
  character::locale::Locale DefaultLocale() const {
    return default_locale_;
  }

 private:
  character::locale::Locale default_locale_;
  character::locale::Locale current_locale_;
};

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_H_
