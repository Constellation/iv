// utility implementation of i18n
//
// See ECMAScript Internationalization API
//   http://wiki.ecmascript.org/doku.php?id=globalization:globalization
//   http://wiki.ecmascript.org/doku.php?id=globalization:specification_drafts
//
#ifndef IV_I18N_H_
#define IV_I18N_H_
#include <iv/character.h>
#include <iv/i18n_language_tag_scanner.h>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/notfound.h>
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
  LanguageTagScanner verifier(it, last);
  return verifier.IsWellFormed();
}

inline bool IsWellFormedLanguageTag(const StringPiece& piece) {
  typedef StringPiece::const_iterator Iter;
  LanguageTagScanner verifier(piece.cbegin(), piece.cend());
  return verifier.IsWellFormed();
}

inline bool IsWellFormedLanguageTag(const UStringPiece& piece) {
  typedef UStringPiece::const_iterator Iter;
  LanguageTagScanner verifier(piece.cbegin(), piece.cend());
  return verifier.IsWellFormed();
}

// 10.2.1 IndexOfMatch(availableLocales, locale)
// use kNotFound instead of -1
template<typename AvailIter>
inline AvailIter IndexOfMatch(AvailIter ait,
                              AvailIter alast, const std::string& locale) {
  std::string candidate = locale;
  while (!candidate.empty()) {
    const AvailIter it = std::find(ait, alast, candidate);
    if (it != alast) {
      return it;
    }
    std::size_t pos = locale.rfind('-');
    if (pos == std::string::npos) {
      return alast;
    }
    if (pos >= 2 && candidate[pos - 2] == '-') {
      pos -= 2;
    }
    candidate.resize(pos);
  }
  return alast;
}

// 10.2.2 LookupMatch(availableLocales, requestedLocales)
class LookupResult {
 public:
  LookupResult(const std::string& l,
               Locale loc, std::size_t p)
    : locale(l), parsed(loc), position(p) { }
  LookupResult(const std::string& l)
    : locale(l), parsed(), position(kNotFound) { }

  bool empty() { return position == kNotFound; }

 private:
  std::string locale;
  Locale parsed;
  std::size_t position;
};

template<typename AvailIter, typename ReqIter>
inline LookupResult LookupMatch(AvailIter ait, AvailIter alast,
                                ReqIter rit, ReqIter rlast) {
  AvailIter pos = alast;
  Locale locale;
  for (ReqIter it = rit; it != rlast && pos == alast; ++it) {
    LanguageTagScanner scanner(it->begin(), it->end());
    if (!scanner.IsWellFormed()) {
      continue;
    }
    const std::string no_extensions_locale = scanner.Canonicalize(true);
    locale = scanner.locale();
    pos = IndexOfMatch(ait, alast, no_extensions_locale);
  }
  if (pos != alast) {
    return LookupResult(*pos, locale, std::distance(ait, pos));
  }
  // TODO(Constellation) fix default locale
  return LookupResult("en-US");
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
