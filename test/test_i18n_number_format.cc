#include <gtest/gtest.h>
#include <iv/i18n_number_format.h>

TEST(I18NNumberFormatCase, ENLocaleTest) {
  using iv::core::i18n::NumberFormat;
  {
    const NumberFormat formatter(&iv::core::i18n::number_format_data::EN, NumberFormat::DECIMAL);

    EXPECT_EQ("0", formatter.Format(0));
    EXPECT_EQ("0", formatter.Format(-0));
    EXPECT_EQ("100", formatter.Format(100));
    EXPECT_EQ("200", formatter.Format(200));
    EXPECT_EQ("-200", formatter.Format(-200));
  }

  {
    const NumberFormat formatter(&iv::core::i18n::number_format_data::EN, NumberFormat::PERCENT);
    EXPECT_EQ("0%", formatter.Format(0));
    EXPECT_EQ("0%", formatter.Format(-0));
    EXPECT_EQ("10%", formatter.Format(0.1));
    EXPECT_EQ("10000%", formatter.Format(100));
    EXPECT_EQ("20000%", formatter.Format(200));
    EXPECT_EQ("-20000%", formatter.Format(-200));
    EXPECT_EQ("-10%", formatter.Format(-0.1));
  }
}
