#ifndef IV_I18N_DATE_TIME_FORMAT_H_
#define IV_I18N_DATE_TIME_FORMAT_H_
#include <memory>
#include <iv/string_view.h>
#include <iv/utils.h>
#include <iv/i18n_timezone.h>
#include <iv/i18n_calendar.h>
#include <iv/date_utils.h>
#include <iv/i18n_number_format.h>
#include <iv/i18n.h>
namespace iv {
namespace core {
namespace i18n {

class DateTimeFormatConstants {
 public:
  enum Details {
    WEEKDAY,
    ERA,
    YEAR,
    MONTH,
    DAY,
    HOUR,
    MINUTE,
    SECOND,
    TIME_ZONE_NAME
  };

  enum FormatValue {
    UNSPECIFIED = 0,
    TWO_DIGIT,
    NUMERIC,
    NARROW,
    SHORT,
    LONG
  };

  typedef std::array<FormatValue, TIME_ZONE_NAME + 1> FormatOptions;

  struct DateTimeOption {
    typedef std::array<const char*, 5> Values;

    const char* key;
    std::size_t size;
    Values values;
  };

  typedef std::array<DateTimeOption, TIME_ZONE_NAME + 1> DateTimeOptions;
};

struct DateTimeFormatData {
  struct FormatsSet {
    std::size_t size;
    const DateTimeFormatConstants::FormatOptions* formats;
  };
  static const int kMaxFormats = 4;
  typedef std::array<FormatsSet, kMaxFormats> Formats;

  const char* name;
  Calendar::Candidate ca;
  NumberingSystem::Candidates nu;
  bool hour12;
  std::size_t size;
  Formats formats;
};

namespace date_time_format_data {

// From Intl.js
// (c) Norbert Lindenberg 2011-2012. All rights reserved.
typedef DateTimeFormatConstants D;
static const std::array<D::FormatOptions, 8> kRequiredFormats = { {
  { { D::LONG, D::UNSPECIFIED, D::NUMERIC, D::LONG, D::NUMERIC, D::NUMERIC, D::NUMERIC, D::NUMERIC, D::UNSPECIFIED } },
  { { D::LONG, D::UNSPECIFIED, D::NUMERIC, D::LONG, D::NUMERIC, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED } },
  { { D::UNSPECIFIED, D::UNSPECIFIED, D::NUMERIC, D::NUMERIC, D::NUMERIC, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED } },
  { { D::UNSPECIFIED, D::UNSPECIFIED, D::NUMERIC, D::LONG, D::NUMERIC, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED } },
  { { D::UNSPECIFIED, D::UNSPECIFIED, D::NUMERIC, D::SHORT, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED } },
  { { D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::SHORT, D::NUMERIC, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED } },
  { { D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::NUMERIC, D::NUMERIC, D::NUMERIC, D::UNSPECIFIED } },
  { { D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::UNSPECIFIED, D::NUMERIC, D::NUMERIC, D::UNSPECIFIED, D::UNSPECIFIED } }
} };

static const DateTimeFormatData EN = {
  "en",
  { {
      Calendar::GREGORY
  } },
  { {
      NumberingSystem::LATN
  } },
  false,
  1,
  { {
      { kRequiredFormats.size(), kRequiredFormats.data() }
  } }
};

}  // namespace deta_time_format_data

typedef std::array<const char*, 1> DateTimeFormatDataNames;
typedef std::array<const DateTimeFormatData*, 1> DateTimeFormatDataValues;

static const DateTimeFormatDataNames kDateTimeFormatDataNames = { {
  date_time_format_data::EN.name,
} };

static const DateTimeFormatDataValues kDateTimeFormatDataValues = { {
  &date_time_format_data::EN,
} };

class DateTimeFormat : public DateTimeFormatConstants {
 public:
  typedef DateTimeFormatData Data;
  template<typename Iter>
  static FormatValue ToFormatValue(Iter it, Iter last) {
    if (Equals("narrow", it, last)) {
      return NARROW;
    } else if (Equals("numeric", it, last)) {
      return NUMERIC;
    } else if (Equals("short", it, last)) {
      return SHORT;
    } else if (Equals("long", it, last)) {
      return LONG;
    } else if (Equals("2-digit", it, last)) {
      return TWO_DIGIT;
    }
    return UNSPECIFIED;
  }

  DateTimeFormat(
      I18N* i18n, const Data* l,
      bool hour12,
      TimeZone::Type tz, Calendar::Type calendar,
      const FormatOptions& format)
    : locale_(l),
      hour12_(hour12),
      tz_(tz),
      calendar_(calendar),
      format_(format),
      nf_(),
      nf2_() {
    // TODO(Constellation) more good extension parsing
    // construct nf and nf2
    const std::string loc = locale()->name;
    const LookupResult res =
        i18n->BestFitMatcher(
            NumberFormat::AvailableLocales().begin(),
            NumberFormat::AvailableLocales().end(),
            &loc, (&loc) + 1);
    const NumberFormat::Data* nl =
        NumberFormat::Lookup(res.locale());

    // relevantExtensionKeys are ['nu']
    const NumberingSystem::Data* nnu = nullptr;
    {
      typedef LookupResult::UnicodeExtensions Ext;
      const Ext::const_iterator key =
          std::find(res.extensions().begin(), res.extensions().end(), "nu");
      if (key != res.extensions().end()) {
        const Ext::const_iterator val = key + 1;
        if (val != res.extensions().end() && val->size() > 2) {
          const NumberingSystem::Data* data = NumberingSystem::Lookup(*val);
          if (data && nl->AcceptedNumberingSystem(data->type)) {
            nnu = data;
          }
        }
      }
    }
    nf_.reset(
        new NumberFormat(
            nl,
            NumberFormat::DECIMAL,
            NumberFormat::kUnspecified,
            NumberFormat::kUnspecified,
            1,
            0,
            3,
            nnu,
            nullptr,
            Currency::NAME,
            false));
    nf2_.reset(
        new NumberFormat(
            nl,
            NumberFormat::DECIMAL,
            NumberFormat::kUnspecified,
            NumberFormat::kUnspecified,
            2,
            0,
            3,
            nnu,
            nullptr,
            Currency::NAME,
            false));
  }

  static const DateTimeFormatDataNames& AvailableLocales() {
    return kDateTimeFormatDataNames;
  }

  typedef std::unordered_map<std::string, const Data*> DateTimeFormatDataMap;

  static const Data* Lookup(string_view name) {
    const DateTimeFormatDataMap::const_iterator it = Map().find(name);
    if (it != Map().end()) {
      return it->second;
    }
    return nullptr;
  }

  // section 12.3.2 Intl.DateTimeFormat.prototype.format
  UString Format(double x) const {
    assert(core::math::IsFinite(x));
    // const double res = ToLocalTime(x, calendar(), tz());
    return UString();
  }

  static double ToLocalTime(double date, Calendar::Type calendar, TimeZone::Type time_zone) {
    // currently, simple local time wrapper,
    // because calendar is only GREGORY and time_zone is only UTC
    if (calendar == Calendar::GREGORY) {
      // nothing to do it
    }
    if (time_zone == TimeZone::UTC) {
      return date;
    }
    return date::LocalTime(date);
  }

  const Data* locale() const { return locale_; }
  bool hour12() const { return hour12_; }
  TimeZone::Type tz() const { return tz_; }
  Calendar::Type calendar() const { return calendar_; }
  const FormatOptions& format() const { return format_; }
  NumberFormat* nf() { return nf_.get(); }
  NumberFormat* nf2() { return nf2_.get(); }
  const NumberFormat* nf() const { return nf_.get(); }
  const NumberFormat* nf2() const { return nf2_.get(); }
 private:
  template<typename Iter>
  static bool Equals(const core::string_view& value, Iter it, Iter last) {
    return CompareIterators(value.begin(), value.end(), it, last) == 0;
  }

  static const DateTimeFormatDataMap& Map() {
    static const DateTimeFormatDataMap map = InitMap();
    return map;
  }

  static DateTimeFormatDataMap InitMap() {
    DateTimeFormatDataMap map;
    for (DateTimeFormatDataValues::const_iterator it = kDateTimeFormatDataValues.begin(),  // NOLINT
         last = kDateTimeFormatDataValues.end();
         it != last; ++it) {
      map.insert(std::make_pair((*it)->name, *it));
    }
    return map;
  }

  const Data* locale_;
  bool hour12_;
  TimeZone::Type tz_;
  Calendar::Type calendar_;
  FormatOptions format_;
  std::unique_ptr<NumberFormat> nf_;
  std::unique_ptr<NumberFormat> nf2_;
};

static const DateTimeFormat::DateTimeOptions kDateTimeOptions = { {
  { "weekday", 3, { { "narrow", "short", "long" } } },
  { "era", 3, { { "narrow", "short", "long" } } },
  { "year", 2, { { "2-digit", "numeric" } } },
  { "month", 5, { { "2-digit", "numeric", "narrow", "short", "long" } } },
  { "day", 2, { { "2-digit", "numeric" } } },
  { "hour", 2, { { "2-digit", "numeric" } } },
  { "minute", 2, { { "2-digit", "numeric" } } },
  { "second", 2, { { "2-digit", "numeric" } } },
  { "timeZoneName", 2, { { "short", "long" } } }
} };

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_DATE_TIME_FORMAT_H_
