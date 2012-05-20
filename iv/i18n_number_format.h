#ifndef IV_I18N_NUMBER_FORMAT_H_
#define IV_I18N_NUMBER_FORMAT_H_
#include <string>
#include <cstdlib>
#include <iv/detail/array.h>
#include <iv/platform_math.h>
#include <iv/dtoa.h>
#include <iv/i18n_numbering_system.h>
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
      minimum_fraction_digits_(minimum_fraction_digits),
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
    assert(min_precision >= 1 && min_precision <= 21);
    assert(max_precision >= 1 && max_precision <= 21);
    assert(math::IsFinite(x) && x >= 0);

    std::string m = ToPrecision(x, max_precision);
    // Expand exponential.
    m = ExpandExponential(m);

    const std::size_t period = m.find('.');
    if (period != std::string::npos && max_precision > min_precision) {
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
      dtoa::StringDToA builder;
      return builder.Build(x);
    } else {
      dtoa::StringDToA builder;
      return builder.BuildFixed(x, frac, 0);
    }
  }

  static std::string ExpandExponential(const std::string& m) {
    const std::string::size_type dot_pos = m.find('.');
    const std::string::size_type exp_pos = m.find('e');

    std::string before_dot;
    std::string after_dot;
    std::string exp;

    if (dot_pos == std::string::npos) {
      if (exp_pos  == std::string::npos) {
        before_dot = m;
      } else {
        before_dot.assign(m.begin(), m.begin() + exp_pos);
        exp.assign(m.begin() + exp_pos, m.end());
      }
    } else {
      before_dot.assign(m.begin(), m.begin() + dot_pos);
      if (exp_pos == std::string::npos) {
        after_dot.assign(m.begin() + dot_pos + 1, m.end());
      } else {
        after_dot.assign(m.begin() + dot_pos + 1, m.begin() + exp_pos);
        exp.assign(m.begin() + exp_pos, m.end());
      }
    }

    if (exp.empty() || after_dot.empty()) {
      return m;
    }

    const int e = std::atoi(exp.c_str() + 1);
    if (exp[0] == '-') {
      return "0." + std::string(e - 1, '0') + before_dot + after_dot;
    } else {
      return before_dot + after_dot + std::string(e - after_dot.size(), '0');
    }
  }

  // section 12.3.2
  static std::string ToRawFixed(
      double x,
      int minimum_integer_digits,
      int minimum_fraction_digits,
      int maximum_fraction_digits) {
    assert(minimum_integer_digits >= 1 && minimum_integer_digits <= 21);
    assert(minimum_fraction_digits >= 0 && minimum_fraction_digits <= 21);
    assert(maximum_fraction_digits >= 0 && maximum_fraction_digits <= 21);
    assert(math::IsFinite(x) && x >= 0);

    std::string m = ToFixed(x, maximum_fraction_digits);

    // Expand exponential and add trailing zeros.
    // We must ensure digits after dot exactly equals to max_fraction_digits.
    // So If it is less, we should append trailing zeros.
    m = ExpandExponential(m);
    if (m.find('.') == std::string::npos) {
      m.push_back('.');
      m.append(std::string(maximum_fraction_digits, '0'));
    }

    assert(static_cast<int>(m.size() - m.find('.') - 1) == maximum_fraction_digits);

    const int i = m.find('.');
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
