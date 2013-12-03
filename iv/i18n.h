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
#include <iv/symbol_fwd.h>
#include <iv/i18n_language_tag_scanner.h>
#include <iv/i18n_number_format.h>
namespace iv {
namespace core {
namespace i18n {

// Only mapping ASCII characters
// 6.1 Case Sensitivity and Case Mapping
inline char16_t ToLocaleIdentifierUpperCase(char16_t ch) {
  if ('a' <= ch && ch <= 'z') {
    return ch - 'a' + 'A';
  }
  return ch;
}

inline char16_t ToLocaleIdentifierLowerCase(char16_t ch) {
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
  LanguageTagScanner verifier(piece.cbegin(), piece.cend());
  return verifier.IsStructurallyValid();
}

inline bool IsStructurallyValidLanguageTag(const UStringPiece& piece) {
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
    const char16_t ch = ToLocaleIdentifierUpperCase(*it);
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

  bool IsFound() const { return !locale().empty(); }

  const std::string& locale() const { return locale_; }
  const UnicodeExtensions& extensions() const { return extensions_; }
 private:
  std::string locale_;
  UnicodeExtensions extensions_;
};

class I18N {
 public:
  I18N() : symbols_() { }

  // 6.2.4 DefaultLocale()
  // the well-formed (6.2.2) and canonicalized (6.2.3) BCP 47 language tag
  std::string DefaultLocale() const {
    // TODO(Constellation) not implemented properly yet
    return "en";
  }

  // 9.2.2 BestAvailableLocale(availableLocales, locale)
  // use kNotFound instead of -1
  template<typename AvailIter>
  inline AvailIter BestAvailableLocale(AvailIter ait,
                                       AvailIter alast,
                                       const std::string& locale) {
    std::size_t size = locale.size();
    StringPiece candidate(locale.data(), size);
    while (!candidate.empty()) {
      const AvailIter it = std::find(ait, alast, candidate);
      if (it != alast) {
        return it;
      }
      std::size_t pos = candidate.rfind('-');
      if (pos == std::string::npos) {
        return alast;
      }
      if (pos >= 2 && candidate[pos - 2] == '-') {
        pos -= 2;
      }
      candidate = StringPiece(locale.data(), pos - 1);
    }
    return alast;
  }

  // 9.2.3 LookupMatcher(availableLocales, requestedLocale)
  template<typename AvailIter, typename ReqIter>
  inline LookupResult LookupMatcher(AvailIter ait,
                                    AvailIter alast,
                                    ReqIter rit,
                                    ReqIter rlast) {
    AvailIter pos = alast;
    Locale locale;
    for (ReqIter it = rit; it != rlast && pos == alast; ++it) {
      LanguageTagScanner scanner(it->begin(), it->end());
      if (!scanner.IsStructurallyValid()) {
        continue;
      }
      const std::string no_extensions_locale = scanner.Canonicalize(true);
      locale = scanner.locale();
      pos = BestAvailableLocale(ait, alast, no_extensions_locale);
    }
    if (pos != alast) {
      return LookupResult(*pos, locale);
    }
    return LookupResult(DefaultLocale());
  }

  // 9.2.3 BestFitMatcher(availableLocales, requestedLocale)
  template<typename AvailIter, typename ReqIter>
  inline LookupResult BestFitMatcher(AvailIter ait,
                                     AvailIter alast,
                                     ReqIter rit,
                                     ReqIter rlast) {
    return LookupMatcher(ait, alast, rit, rlast);
  }

#define IV_I18N_LOCALE_SYMBOLS(V)\
  V(initializedIntlObject)\
  V(initializedNumberFormat)\
  V(initializedDateTimeFormat)\

  class Symbols {
   public:
    Symbols()
      :
#define IV_V(name) name##_(ToUString(#name)),
        IV_I18N_LOCALE_SYMBOLS(IV_V)
#undef IV_V
        last_order_() {
      last_order_ = 0;  // suppress warning
    }

#define IV_V(name) Symbol name() const {\
  return symbol::MakePrivateSymbol(reinterpret_cast<uintptr_t>(&name##_)); }
    IV_I18N_LOCALE_SYMBOLS(IV_V)
#undef IV_V

   private:
#define IV_V(name) const core::UString name##_;
    IV_I18N_LOCALE_SYMBOLS(IV_V)
#undef IV_V
    int last_order_;
  };

#undef IV_I18N_LOCALE_SYMBOLS

  const Symbols& symbols() const { return symbols_; }
 private:
  Symbols symbols_;
};

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_H_
