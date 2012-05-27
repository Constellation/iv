// utility implementation of i18n
//
// See ECMAScript Internationalization API
//   http://wiki.ecmascript.org/doku.php?id=globalization:globalization
//   http://wiki.ecmascript.org/doku.php?id=globalization:specification_drafts
//
#ifndef IV_I18N_H_
#define IV_I18N_H_
#include <iv/character.h>
#include <iv/stringpiece.h>
#include <iv/ustringpiece.h>
#include <iv/notfound.h>
#include <iv/i18n_language_tag_scanner.h>
#include <iv/i18n_number_format.h>
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

inline uint16_t ToLocaleIdentifierLowerCase(uint16_t ch) {
  if ('A' <= ch && ch <= 'Z') {
    return ch + ('a' - 'A');
  }
  return ch;
}

// 6.2.2 IsStructurallyValidLanguageTag(locale)
// BCP 47 language tag as specified in RFC 5646 section 2.1
//
// See iv/i18n_language_tag_verifier.h
template<typename Iter>
inline bool IsStructurallyValidLanguageTag(Iter it, Iter last) {
  LanguageTagScanner verifier(it, last);
  return verifier.IsStructurallyValid();
}

inline bool IsStructurallyValidLanguageTag(const StringPiece& piece) {
  typedef StringPiece::const_iterator Iter;
  LanguageTagScanner verifier(piece.cbegin(), piece.cend());
  return verifier.IsStructurallyValid();
}

inline bool IsStructurallyValidLanguageTag(const UStringPiece& piece) {
  typedef UStringPiece::const_iterator Iter;
  LanguageTagScanner verifier(piece.cbegin(), piece.cend());
  return verifier.IsStructurallyValid();
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
    const uint16_t ch = ToLocaleIdentifierUpperCase(*it);
    if (ch < 'A' || 'Z' < ch) {
      return false;
    }
  }
  return true;
}

// 10.2.2 LookupMatch(availableLocales, requestedLocales)
class LookupResult {
 public:
  typedef std::vector<std::string> UnicodeExtensions;

  LookupResult() : locale_(), extensions_() { }

  explicit LookupResult(const std::string& l)
    : locale_(l), extensions_() {
  }

  LookupResult(const std::string& l, Locale loc)
    : locale_(l), extensions_() {
    Locale::Map::const_iterator it = loc.extensions().find('u');
    if (it != loc.extensions().end()) {
      for (Locale::Map::const_iterator last = loc.extensions().end();
           it != last && it->first == 'u'; ++it) {
        extensions_.push_back(it->second);
      }
    }
  }

  const std::string& locale() const { return locale_; }
  const UnicodeExtensions& extensions() const { return extensions_; }
 private:
  std::string locale_;
  UnicodeExtensions extensions_;
};

class I18N {
 public:
  I18N() { }

  // 6.2.4 DefaultLocale()
  // the well-formed (6.2.2) and canonicalized (6.2.3) BCP 47 language tag
  std::string DefaultLocale() const {
    // TODO(Constellation) not implemented properly yet
    return "en";
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

  template<typename AvailIter, typename ReqIter>
  inline LookupResult LookupMatch(AvailIter ait, AvailIter alast,
                                  ReqIter rit, ReqIter rlast) {
    AvailIter pos = alast;
    Locale locale;
    for (ReqIter it = rit; it != rlast && pos == alast; ++it) {
      LanguageTagScanner scanner(it->begin(), it->end());
      if (!scanner.IsStructurallyValid()) {
        continue;
      }
      const std::string no_extensions_locale = scanner.Canonicalize(true);
      locale = scanner.locale();
      pos = IndexOfMatch(ait, alast, no_extensions_locale);
    }
    if (pos != alast) {
      return LookupResult(*pos, locale);
    }
    return LookupResult(DefaultLocale());
  }

  template<typename AvailIter, typename ReqIter>
  inline LookupResult BestFitMatch(AvailIter ait, AvailIter alast,
                                   ReqIter rit, ReqIter rlast) {
    // TODO(Constellation) implement BestFitMatch
    return LookupMatch(ait, alast, rit, rlast);
  }
};

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_H_
