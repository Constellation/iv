#include <gtest/gtest.h>
#include <iv/i18n.h>

// examples are described at
//   RFC 5646 Appendix A.  Examples of Language Tags (Informative)
TEST(I18NCase, IsWellFormedLanguageTagTest) {
  using iv::core::i18n::IsWellFormedLanguageTag;

  // Simple language subtag:
  EXPECT_TRUE(IsWellFormedLanguageTag("de")) << "German";
  EXPECT_TRUE(IsWellFormedLanguageTag("fr")) << "French";
  EXPECT_TRUE(IsWellFormedLanguageTag("ja")) << "Japanese";
  EXPECT_TRUE(IsWellFormedLanguageTag("i-enochian")) << "(example of a grandfathered tag)";

  // Language subtag plus Script subtag:
  EXPECT_TRUE(IsWellFormedLanguageTag("zh-Hant")) << "(Chinese written using the Traditional Chinese script)";
  EXPECT_TRUE(IsWellFormedLanguageTag("zh-Hans")) << "(Chinese written using the Simplified Chinese script)";
  EXPECT_TRUE(IsWellFormedLanguageTag("sr-Cyrl")) << "(Serbian written using the Cyrillic script)";
  EXPECT_TRUE(IsWellFormedLanguageTag("sr-Latn")) << "(Serbian written using the Latin script)";

  // Extended language subtags and their primary language subtag counterparts:
  EXPECT_TRUE(IsWellFormedLanguageTag("zh-cmn-Hans-CN")) << "(Chinese, Mandarin, Simplified script, as used in China)";
  EXPECT_TRUE(IsWellFormedLanguageTag("cmn-Hans-CN")) << "(Mandarin Chinese, Simplified script, as used in China)";
  EXPECT_TRUE(IsWellFormedLanguageTag("zh-yue-HK")) << "(Chinese, Cantonese, as used in Hong Kong SAR)";
  EXPECT_TRUE(IsWellFormedLanguageTag("yue-HK")) << "(Cantonese Chinese, as used in Hong Kong SAR)";

  // Language-Script-Region:
  EXPECT_TRUE(IsWellFormedLanguageTag("zh-Hans-CN")) << "(Chinese written using the Simplified script as used in mainland China)";
  EXPECT_TRUE(IsWellFormedLanguageTag("sr-Latn-RS")) << "(Serbian written using the Latin script as used in Serbia)";

  // Language-Variant:
  EXPECT_TRUE(IsWellFormedLanguageTag("sl-rozaj")) << "(Resian dialect of Slovenian)";
  EXPECT_TRUE(IsWellFormedLanguageTag("sl-rozaj-biske")) << "(San Giorgio dialect of Resian dialect of Slovenian)";
  EXPECT_TRUE(IsWellFormedLanguageTag("sl-nedis")) << "(Nadiza dialect of Slovenian)";

  // Language-Region-Variant:
  EXPECT_TRUE(IsWellFormedLanguageTag("de-CH-1901")) << "(German as used in Switzerland using the 1901 variant [orthography])";
  EXPECT_TRUE(IsWellFormedLanguageTag("sl-IT-nedis")) << "(Slovenian as used in Italy, Nadiza dialect)";

  // Language-Script-Region-Variant:
  EXPECT_TRUE(IsWellFormedLanguageTag("hy-Latn-IT-arevela")) << "(Eastern Armenian written in Latin script, as used in Italy)";

  // Language-Region:
  EXPECT_TRUE(IsWellFormedLanguageTag("de-DE")) << "(German for Germany)";
  EXPECT_TRUE(IsWellFormedLanguageTag("en-US")) << "(English as used in the United States)";
  EXPECT_TRUE(IsWellFormedLanguageTag("es-419")) << "(Spanish appropriate for the Latin America and Caribbean region using the UN region code)";

  // Private use subtags:
  EXPECT_TRUE(IsWellFormedLanguageTag("de-CH-x-phonebk"));
  EXPECT_TRUE(IsWellFormedLanguageTag("az-Arab-x-AZE-derbend"));

  // Private use registry values:
  EXPECT_TRUE(IsWellFormedLanguageTag("x-whatever")) << "(private use using the singleton 'x')";
  EXPECT_TRUE(IsWellFormedLanguageTag("qaa-Qaaa-QM-x-southern")) << "(all private tags)";
  EXPECT_TRUE(IsWellFormedLanguageTag("de-Qaaa")) << "(German, with a private script)";
  EXPECT_TRUE(IsWellFormedLanguageTag("sr-Latn-QM")) << "(Serbian, Latin script, private region)";
  EXPECT_TRUE(IsWellFormedLanguageTag("sr-Qaaa-RS")) << "(Serbian, private script, for Serbia)";

  // Tags that use extensions (examples ONLY -- extensions MUST be defined
  // by revision or update to this document, or by RFC):
  EXPECT_TRUE(IsWellFormedLanguageTag("en-US-u-islamcal"));
  EXPECT_TRUE(IsWellFormedLanguageTag("zh-CN-a-myext-x-private"));
  EXPECT_TRUE(IsWellFormedLanguageTag("en-a-myext-b-another"));

  // Some Invalid Tags:
  EXPECT_FALSE(IsWellFormedLanguageTag("de-419-DE")) << "(two region tags)";
  EXPECT_FALSE(IsWellFormedLanguageTag("a-DE")) << "(use of a single-character subtag in primary position; note that there are a few grandfathered tags that start with \"i-\" that are valid)";
  EXPECT_FALSE(IsWellFormedLanguageTag("ar-a-aaa-b-bbb-a-ccc")) << "(two extensions with same single-letter prefix)";
  EXPECT_FALSE(IsWellFormedLanguageTag("")) << "(empty string should be invalid)";
}

TEST(I18NCase, ScanTest) {
  using iv::core::i18n::LanguageTagScanner;
  {
    const std::string str("zh-cmn-Hans-CN");
    LanguageTagScanner scanner(str.begin(), str.end());
    iv::core::i18n::Locale locale = scanner.locale();
    EXPECT_EQ(locale.language(), "zh");
    EXPECT_EQ(locale.extlang()[0], "cmn");
    EXPECT_EQ(locale.script(), "Hans");
    EXPECT_EQ(locale.region(), "CN");
  }
  {
    const std::string str("zn-Hans");
    LanguageTagScanner scanner(str.begin(), str.end());
    iv::core::i18n::Locale locale = scanner.locale();
    EXPECT_EQ(locale.language(), "zn");
    EXPECT_EQ(locale.script(), "Hans");
  }
}

#ifdef IV_ENABLE_I18N
#include <unicode/locid.h>
TEST(I18NCase, CanonicalizeTest) {
  using iv::core::i18n::LanguageTagScanner;
  {
    const std::string str("i-ami");
    LanguageTagScanner scanner(str.begin(), str.end());
    EXPECT_EQ(scanner.Canonicalize(), "ami");
  }
  {
    const std::string str("sr-DEVA");
    LanguageTagScanner scanner(str.begin(), str.end());
    EXPECT_EQ(scanner.Canonicalize(), "sr-Deva");
  }
}
#endif  // IV_ENABLE_I18N
