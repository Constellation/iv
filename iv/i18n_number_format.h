#ifndef IV_I18N_NUMBER_FORMAT_H_
#define IV_I18N_NUMBER_FORMAT_H_
#include <string>
#include <iv/detail/array.h>
#include <iv/platform_math.h>
#include <iv/dtoa.h>
namespace iv {
namespace core {
namespace i18n {
namespace number_format_data {

struct PatternSet {
  std::string positive_pattern;
  std::string negative_pattern;
};

struct Data {
  // 0 => DECIMAL
  // 1 => PRECENT
  // 2 => CURRENCY
  PatternSet patterns[3];
};

static const Data EN = { {
  {
    "{number}",
    "-{number}"
  },
  {
    "{number}%",
    "-{number}%"
  },
  {
    "{currency} {number}",
    "-{currency} {number}"
  }
} };

typedef std::array<std::pair<StringPiece, const Data*>, 1> NumberFormatDataArray;  // NOLINT
static const NumberFormatDataArray kNumberFormatData = { {
  std::make_pair("en", &EN)
} };

}  // namespace number_format_data


class NumberFormat {
 public:
  static const int kUnspecified = -1;

  typedef number_format_data::Data Data;

  enum Style {
    DECIMAL = 0,
    PERCENT,
    CURRENCY
  };

  NumberFormat(const Data* data,
               Style style,
               int minimum_significant_digits = kUnspecified,
               int maximum_significant_digits = kUnspecified,
               int minimum_integer_digits = kUnspecified,
               int minimum_fraction_digits = kUnspecified,
               int maximum_fraction_digits = kUnspecified)
    : data_(data),
      style_(style),
      minimum_significant_digits_(minimum_significant_digits),
      maximum_significant_digits_(maximum_significant_digits),
      minimum_integer_digits_(minimum_integer_digits),
      minimum_fraction_digits_(minimum_integer_digits),
      maximum_fraction_digits_(maximum_fraction_digits) {
  }

  static std::string ToPrecision(double x, int max_precision) {
    assert(math::IsFinite(x));
    dtoa::StringDToA builder;
    return builder.BuildPrecision(x, max_precision, 0);
  }

  // section 12.3.2
  static std::string ToRawPrecision(
      double x,
      int min_precision,
      int max_precision) {
    std::string m = ToPrecision(x, max_precision);
    const std::size_t i = m.find('.');
    if (i != std::string::npos && max_precision > min_precision) {
      int cut = max_precision - min_precision;
      while (cut > 0 && m[m.size() - 1] == '0') {
        m.erase(m.size() - 1);
        --cut;
      }
      if (m[m.size() - 1] == '.') {
          m.erase(m.size() - 1);
      }
    }
    return m;
  }

  static std::string ToFixed(double x, int frac) {
    assert(math::IsFinite(x));
    if (!(std::fabs(x) < 1e+21)) {
      // included NaN and Infinity
      dtoa::StringDToA builder;
      return builder.Build(x);
    } else {
      dtoa::StringDToA builder;
      return builder.BuildFixed(x, frac, 0);
    }
  }

  // section 12.3.2
  static std::string ToRawFixed(
      double x,
      int minimum_integer_digits,
      int minimum_fraction_digits,
      int maximum_fraction_digits) {
    std::string m = ToFixed(x, maximum_fraction_digits);
    int i = 0;
    {
      const std::size_t t = m.find('.');
      if (t == std::string::npos) {
        i = m.size();
      } else {
        i = t;
      }
    }
    int cut = maximum_fraction_digits - minimum_fraction_digits;
    while (cut > 0 && m[m.size() - 1] == '0') {
      m.erase(m.size() - 1);
      --cut;
    }
    if (m[m.size() - 1] == '.') {
        m.erase(m.size() - 1);
    }
    if (i < minimum_integer_digits) {
      const std::string zero(minimum_integer_digits - i, '0');
      m = zero + m;
    }
    return m;
  }

  // section 12.3.2 Intl.NumberFormat.prototype.format(value)
  std::string Format(double x) const {
    bool negative = false;
    std::string n;
    if (!math::IsFinite(x)) {
      if (math::IsNaN(x)) {
        n = "NaN";
      } else {
        // infinity mark
        n.push_back(0xe2);
        n.push_back(0x88);
        n.push_back(0x9e);
        if (x < 0) {
          negative = true;
        }
      }
    } else {
      if (x < 0) {
        negative = true;
        x = -x;
      }

      if (style_ == PERCENT) {
        x *= 100;
      }

      if (minimum_significant_digits_ != kUnspecified &&
          maximum_significant_digits_ != kUnspecified) {
        n = ToRawPrecision(x,
                           minimum_significant_digits_,
                           maximum_significant_digits_);
      } else {
        n = ToRawFixed(x,
                       minimum_integer_digits_,
                       minimum_fraction_digits_,
                       maximum_fraction_digits_);
      }

      // TODO(Constellation) implement numbering system
      // section 12.3.2-5-e and f
    }

    std::string pattern;
    if (negative) {
      pattern = data_->patterns[style_].negative_pattern;
    } else {
      pattern = data_->patterns[style_].positive_pattern;
    }


    const std::size_t i = pattern.find("{number}");
    assert(i != std::string::npos);
    std::string result(pattern.begin(), pattern.begin() + i);
    result.append(n);
    result.append(pattern.begin() + i + std::strlen("{number}"), pattern.end());

    if (style_ == CURRENCY) {
      // TODO(Constellation) implement currency pattern
    }
    return result;
  }

  int minimum_significant_digits() { return minimum_significant_digits_; }
  int maximum_significant_digits() { return maximum_significant_digits_; }
  int minimum_integer_digits() { return minimum_integer_digits_; }
  int minimum_fraction_digits() { return minimum_fraction_digits_; }
  int maximum_fraction_digits() { return maximum_fraction_digits_; }

 private:
  const Data* data_;
  Style style_;
  int minimum_significant_digits_;
  int maximum_significant_digits_;
  int minimum_integer_digits_;
  int minimum_fraction_digits_;
  int maximum_fraction_digits_;
};

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_NUMBER_FORMAT_H_
