// RFC 5646 : Tags for Identifying Languages
//   http://tools.ietf.org/html/rfc5646
// RFC 5234 : Augmented BNF for Syntax Specifications: ABNF
//   http://tools.ietf.org/html/rfc5234
//
// =========================== Language Tag ABNF ===============================
//
// Language-Tag  = langtag             ; normal language tags
//               / privateuse          ; private use tag
//               / grandfathered       ; grandfathered tags
//
// langtag       = language
//                 ["-" script]
//                 ["-" region]
//                 *("-" variant)
//                 *("-" extension)
//                 ["-" privateuse]
//
// language      = 2*3ALPHA            ; shortest ISO 639 code
//                 ["-" extlang]       ; sometimes followed by
//                                     ; extended language subtags
//               / 4ALPHA              ; or reserved for future use
//               / 5*8ALPHA            ; or registered language subtag
//
// extlang       = 3ALPHA              ; selected ISO 639 codes
//                 *2("-" 3ALPHA)      ; permanently reserved
//
// script        = 4ALPHA              ; ISO 15924 code
//
// region        = 2ALPHA              ; ISO 3166-1 code
//               / 3DIGIT              ; UN M.49 code
//
// variant       = 5*8alphanum         ; registered variants
//               / (DIGIT 3alphanum)
//
// extension     = singleton 1*("-" (2*8alphanum))
//
//                                     ; Single alphanumerics
//                                     ; "x" reserved for private use
// singleton     = DIGIT               ; 0 - 9
//               / %x41-57             ; A - W
//               / %x59-5A             ; Y - Z
//               / %x61-77             ; a - w
//               / %x79-7A             ; y - z
//
// privateuse    = "x" 1*("-" (1*8alphanum))
//
// grandfathered = irregular           ; non-redundant tags registered
//               / regular             ; during the RFC 3066 era
//
// irregular     = "en-GB-oed"         ; irregular tags do not match
//               / "i-ami"             ; the 'langtag' production and
//               / "i-bnn"             ; would not otherwise be
//               / "i-default"         ; considered 'well-formed'
//               / "i-enochian"        ; These tags are all valid,
//               / "i-hak"             ; but most are deprecated
//               / "i-klingon"         ; in favor of more modern
//               / "i-lux"             ; subtags or subtag
//               / "i-mingo"           ; combination
//               / "i-navajo"
//               / "i-pwn"
//               / "i-tao"
//               / "i-tay"
//               / "i-tsu"
//               / "sgn-BE-FR"
//               / "sgn-BE-NL"
//               / "sgn-CH-DE"
//
// regular       = "art-lojban"        ; these tags match the 'langtag'
//               / "cel-gaulish"       ; production, but their subtags
//               / "no-bok"            ; are not extended language
//               / "no-nyn"            ; or variant subtags: their meaning
//               / "zh-guoyu"          ; is defined by their registration
//               / "zh-hakka"          ; and all of these are deprecated
//               / "zh-min"            ; in favor of a more modern
//               / "zh-min-nan"        ; subtag or sequence of subtags
//               / "zh-xiang"
//
// alphanum      = (ALPHA / DIGIT)     ; letters and numbers
//
#ifndef IV_I18N_LANGUAGE_TAG_SCANNER_H_
#define IV_I18N_LANGUAGE_TAG_SCANNER_H_
#include <bitset>
#include <map>
#include <iv/detail/array.h>
#include <iv/character.h>
#include <iv/stringpiece.h>
#include <iv/ustring.h>
#include <iv/i18n_locale.h>
#include <iv/i18n_language_tag.h>

#ifdef IV_ENABLE_I18N
#include <unicode/uloc.h>
#endif  // IV_ENABLE_I18N

namespace iv {
namespace core {
namespace i18n {

#define IV_EXPECT_NEXT_TAG()\
  do {\
    if (c_ != '-') {\
      if (IsEOS()) {\
        return true;\
      }\
      return false;\
    }\
    Advance();\
  } while (0)

template<typename Iter>
class LanguageTagScanner {
 public:
  typedef LanguageTagScanner<Iter> this_type;

  LanguageTagScanner(Iter it, Iter last)
    : start_(it),
      last_(last),
      pos_(it),
      c_(-1),
      valid_(),
      locale_(),
      unique_(0) {
    locale_.all_.assign(it, last);
    Restore(start_);
    valid_ = Scan();
  }

  bool IsWellFormed() const { return valid_; }

  const Locale& locale() const { return locale_; }

#ifdef IV_ENABLE_I18N
  // 6.2.3 CanonicalizeLanguageTag(locale)
  // Returns the canonical and case-regularized form of the locale argument
  // (which must be a String value that is
  // a well-formed BCP 47 language tag as verified
  // by the IsWellFormedLanguageTag abstract operation).
  // Specified in RFC 5646 section 4.5
  std::string Canonicalize() {
    std::array<char, ULOC_FULLNAME_CAPACITY> vec1;
    UErrorCode status = U_ZERO_ERROR;
    int32_t length = 0;
    uloc_forLanguageTag(locale_.all_.c_str(),
                        vec1.data(), vec1.size(), &length, &status);
    vec1[length] = '\0';
    std::array<char, ULOC_FULLNAME_CAPACITY> vec2;
    length = uloc_toLanguageTag(vec1.data(),
                                vec2.data(), length, true, &status);
    return std::string(vec2.begin(), vec2.begin() + length);
  }
#endif  // IV_ENABLE_I18N

  std::string CanonicalizedLanguageTag() {
    return "";
  }

  bool Scan() {
    // check grandfathered
    const TagMap::const_iterator it =
        Grandfathered().find(std::string(start_, last_));
    if (it != Grandfathered().end()) {
      LanguageTagScanner<std::string::const_iterator>
          scan2(it->second.begin(), it->second.end());
      const bool valid = scan2.IsWellFormed();
      if (valid) {
        locale_ = scan2.locale();
      }
      return valid;
    }
    return ScanRegular();
  }

  const Locale& locale() { return locale_; }

 private:
  void Clear() {
    locale_.language_.clear();
    locale_.extlang_.clear();
    locale_.script_.clear();
    locale_.region_.clear();
    locale_.variants_.clear();
    locale_.extensions_.clear();
    locale_.privateuse_.clear();
    unique_.reset();
  }

  bool ScanRegular() {
    if (ScanLangtag(start_)) {
      return true;
    }
    Clear();
    if (ScanPrivateUse(start_)) {
      return true;
    }
    return false;
  }

  bool ScanLangtag(Iter restore) {
    // langtag       = language
    //                 ["-" script]
    //                 ["-" region]
    //                 *("-" variant)
    //                 *("-" extension)
    //                 ["-" privateuse]
    if (!ScanLanguage(restore)) {
      Restore(restore);
      return false;
    }

    // script
    Iter restore2 = current();
    IV_EXPECT_NEXT_TAG();
    ScanScript(restore2);

    // region
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    ScanRegion(restore2);

    // variant
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    while (ScanVariant(restore2)) {
      restore2 = current();
      IV_EXPECT_NEXT_TAG();
    }

    // extension
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    while (ScanExtension(restore2)) {
      restore2 = current();
      IV_EXPECT_NEXT_TAG();
    }

    // privateuse
    restore2 = current();
    IV_EXPECT_NEXT_TAG();
    ScanPrivateUse(restore2);

    if (!IsEOS()) {
      Restore(restore);
      return false;
    }
    return true;
  }

  bool ScanScript(Iter restore) {
    // script        = 4ALPHA              ; ISO 15924 code
    const Iter s = current();
    if (!ExpectAlpha(4) || !MaybeValid()) {
      Restore(restore);
      return false;
    }
    locale_.script_.assign(s, current());
    return true;
  }

  bool ScanRegion(Iter restore) {
    // region        = 2ALPHA              ; ISO 3166-1 code
    //               / 3DIGIT              ; UN M.49 code
    const Iter restore2 = current();
    if (ExpectAlpha(2) && MaybeValid()) {
      locale_.region_.assign(restore2, current());
      return true;
    }

    Restore(restore2);
    for (std::size_t i = 0; i < 3; ++i) {
      if (IsEOS() || !character::IsDecimalDigit(c_)) {
        Restore(restore);
        return false;
      }
      Advance();
    }
    if (!MaybeValid()) {
      Restore(restore);
      return false;
    }
    locale_.region_.assign(restore2, current());
    return true;
  }

  bool ScanVariant(Iter restore) {
    // variant       = 5*8alphanum         ; registered variants
    //               / (DIGIT 3alphanum)
    const Iter restore2 = current();
    if (ExpectAlphanum(5)) {
      for (std::size_t i = 0; i < 3; ++i) {
        if (IsEOS() || !character::IsASCIIAlphanumeric(c_)) {
          break;
        }
        Advance();
      }
      if (MaybeValid()) {
        locale_.variants_.push_back(std::string(restore2, current()));
        return true;
      }
    }

    Restore(restore2);
    if (IsEOS() || !character::IsDigit(c_)) {
      Restore(restore);
      return false;
    }
    Advance();
    if (!ExpectAlphanum(3) || !MaybeValid()) {
      Restore(restore);
      return false;
    }
    locale_.variants_.push_back(std::string(restore2, current()));
    return true;
  }

  // 0 to 61
  static int SingletonID(char ch) {
    assert(character::IsASCIIAlphanumeric(ch));
    // 0 to 9 is assigned to '0' to '9'
    if ('0' <= ch && ch <= '9') {
      return ch - '0';
    }
    if ('A' <= ch && ch <= 'Z') {
      return (ch - 'A') + 10;
    }
    return (ch - 'a') + 10 + 26;
  }

  bool ScanExtension(Iter restore) {
    // extension     = singleton 1*("-" (2*8alphanum))
    //
    //                                     ; Single alphanumerics
    //                                     ; "x" reserved for private use
    // singleton     = DIGIT               ; 0 - 9
    //               / %x41-57             ; A - W
    //               / %x59-5A             ; Y - Z
    //               / %x61-77             ; a - w
    //               / %x79-7A             ; y - z
    if (IsEOS() || !character::IsASCIIAlphanumeric(c_) || c_ == 'x') {
      Restore(restore);
      return false;
    }
    const char target = c_;
    const int ID = SingletonID(target);
    if (unique_.test(ID)) {
      Restore(restore);
      return false;
    }
    Advance();

    Iter s = pos_;
    if (!ExpectExtensionOrPrivateFollowing(2)) {
      Restore(restore);
      return false;
    }

    unique_.set(ID);
    locale_.extensions_.insert(
        std::make_pair(target, std::string(s, current())));
    while (true) {
      Iter restore2 = current();
      s = pos_;
      if (!ExpectExtensionOrPrivateFollowing(2)) {
        Restore(restore2);
        return true;
      }
      locale_.extensions_.insert(
          std::make_pair(target, std::string(s, current())));
    }
    return true;
  }

  bool ScanPrivateUse(Iter restore) {
    // privateuse    = "x" 1*("-" (1*8alphanum))
    if (c_ != 'x') {
      Restore(restore);
      return false;
    }
    Advance();

    Iter s = pos_;
    if (!ExpectExtensionOrPrivateFollowing(1)) {
      Restore(restore);
      return false;
    }

    locale_.privateuse_.append(std::string(s, current()));
    while (true) {
      Iter restore2 = current();
      if (!ExpectExtensionOrPrivateFollowing(1)) {
        Restore(restore2);
        return true;
      }
      locale_.privateuse_.append(std::string(restore2, current()));
    }
    return true;
  }

  bool ScanLanguage(Iter restore) {
    // language      = 2*3ALPHA            ; shortest ISO 639 code
    //                 ["-" extlang]       ; sometimes followed by
    //                                     ; extended language subtags
    //               / 4ALPHA              ; or reserved for future use
    //               / 5*8ALPHA            ; or registered language subtag
    //
    // We assume, after this, '-' or EOS are following is valid.
    Iter restore2 = current();
    if (ExpectLanguageFirst()) {
      return true;
    }

    Restore(restore2);
    if (ExpectAlpha(4) && MaybeValid()) {
      locale_.language_.assign(restore2, current());
      return true;
    }

    Restore(restore2);
    if (!ExpectAlpha(5)) {
      Restore(restore);
      return false;
    }
    for (std::size_t i = 0; i < 3; ++i) {
      if (IsEOS() || !character::IsASCIIAlpha(c_)) {
        break;
      }
      Advance();
    }
    if (!MaybeValid()) {
      Restore(restore);
      return false;
    }

    locale_.language_.assign(restore2, current());
    return true;
  }

  // not simple expect
  bool ExpectLanguageFirst() {
    // first case
    // 2*3ALPHA ["-" extlang]
    //
    // extlang       = 3ALPHA              ; selected ISO 639 codes
    //                 *2("-" 3ALPHA)      ; permanently reserved
    Iter s = current();
    if (!ExpectAlpha(2)) {
      return false;
    }
    if (!IsEOS() && character::IsASCIIAlpha(c_)) {
      Advance();
    }

    Iter restore = current();
    locale_.language_.assign(s, restore);

    // extlang check
    if (c_ != '-') {
      return IsEOS();  // maybe valid
    }
    Advance();

    {
      const Iter s = current();

      // extlang, this is optional
      if (!ExpectAlpha(3) || !MaybeValid()) {
        Restore(restore);
        return true;
      }
      assert(MaybeValid());

      restore = current();
      locale_.extlang_.push_back(std::string(s, restore));
    }

    for (std::size_t i = 0; i < 2; ++i) {
      if (c_ != '-') {
        assert(IsEOS());
        return true;  // maybe valid
      }
      Advance();
      const Iter s = current();
      if (!ExpectAlpha(3) || !MaybeValid()) {
        Restore(restore);
        return true;
      }
      assert(MaybeValid());
      restore = current();
      locale_.extlang_.push_back(std::string(s, restore));
    }
    assert(MaybeValid());
    return true;
  }

  // not simple expect
  bool ExpectExtensionOrPrivateFollowing(std::size_t n) {
    // extension's 1*("-" (2*8alphanum))
    // or
    // privateuse's 1*("-" (1*8alphanum))
    if (c_ != '-') {
      return false;
    }
    Advance();
    if (!ExpectAlphanum(n)) {
      return false;
    }
    for (std::size_t i = 0, len = 8 - n; i < len; ++i) {
      if (IsEOS() || !character::IsASCIIAlphanumeric(c_)) {
        break;
      }
      Advance();
    }
    return MaybeValid();
  }

  // simple expect
  bool ExpectAlphanum(std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (IsEOS() || !character::IsASCIIAlphanumeric(c_)) {
        return false;
      }
      Advance();
    }
    return true;
  }

  // simple expect
  bool ExpectAlpha(std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (IsEOS() || !character::IsASCIIAlpha(c_)) {
        return false;
      }
      Advance();
    }
    return true;
  }

  inline bool IsEOS() const {
    return c_ < 0;
  }

  inline bool MaybeValid() const {
    return IsEOS() || c_ == '-';
  }

  void Restore(Iter pos) {
    pos_ = pos;
    Advance();
  }

  inline void Advance() {
    if (pos_ == last_) {
      c_ = -1;
    } else {
      c_ = *pos_;
      ++pos_;
    }
  }

  inline Iter current() {
    if (pos_ == last_) {
      return last_;
    }
    return pos_ - 1;
  }

  Iter start_;
  Iter last_;
  Iter pos_;
  int c_;
  bool valid_;
  Locale locale_;
  std::bitset<64> unique_;
};

#undef IV_EXPECT_NEXT_TAG
} } }  // namespace iv::core::i18n
#endif  // IV_I18N_LANGUAGE_TAG_SCANNER_H_
