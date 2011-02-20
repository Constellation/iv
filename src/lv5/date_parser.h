#ifndef _IV_LV5_DATE_PARSER_H_
#define _IV_LV5_DATE_PARSER_H_
#include <tr1/array>
#include <algorithm>
#include <limits>
#include "noncopyable.h"
#include "stringpiece.h"
#include "lv5/date_utils.h"
namespace iv {
namespace lv5 {
namespace date {

// inspired from V8 Keyword Table
struct Keyword {
  enum Type {
    NONE = 0,
    MONTH,
    AM_PM,
    TZ
  };
  core::StringPiece keyword;
  Type type;
  int value;

  inline bool IsNone() const {
    return type == NONE;
  }

  inline bool IsMonth() const {
    return type == MONTH;
  }

  inline bool IsAMPM() const {
    return type == AM_PM;
  }

  inline bool IsTimezone() const {
    return type == TZ;
  }
};

static const std::tr1::array<const Keyword, 27> kKeywords = { {
  { "jan", Keyword::MONTH, 1  },
  { "feb", Keyword::MONTH, 2  },
  { "mar", Keyword::MONTH, 3  },
  { "apr", Keyword::MONTH, 4  },
  { "may", Keyword::MONTH, 5  },
  { "jun", Keyword::MONTH, 6  },
  { "jul", Keyword::MONTH, 7  },
  { "aug", Keyword::MONTH, 8  },
  { "sep", Keyword::MONTH, 9  },
  { "oct", Keyword::MONTH, 10 },
  { "nov", Keyword::MONTH, 11 },
  { "dec", Keyword::MONTH, 12 },
  { "am" , Keyword::AM_PM, 0  },
  { "pm" , Keyword::AM_PM, 12 },
  { "ut" , Keyword::TZ,    0  },
  { "utc", Keyword::TZ,    0  },
  { "z",   Keyword::TZ,    0  },
  { "gmt", Keyword::TZ,    0  },
  { "cdt", Keyword::TZ,    -5 },
  { "cst", Keyword::TZ,    -6 },
  { "edt", Keyword::TZ,    -4 },
  { "est", Keyword::TZ,    -5 },
  { "mdt", Keyword::TZ,    -6 },
  { "mst", Keyword::TZ,    -7 },
  { "pdt", Keyword::TZ,    -7 },
  { "pst", Keyword::TZ,    -8 },
  { "",    Keyword::NONE,  0  },
} };

static const int kNone = std::numeric_limits<int>::max();

class KeywordChecker : private core::Noncopyable<KeywordChecker>::type {
 public:

  template<typename char_type>
  class Finder {
   public:
    typedef std::tr1::array<char_type, 4> buf_type;
    Finder(const buf_type& buf, std::size_t len)
      : buf_(buf), len_(len) { }

    bool operator()(const Keyword& key) const {
      if (key.IsNone()) {
        return true;
      }
      if (key.keyword.size() != len_) {
        return false;
      }
      return std::equal(key.keyword.begin(), key.keyword.end(), buf_.begin());
    }
   private:
    const buf_type& buf_;
    const std::size_t len_;
  };

  template<typename Iter>
  static const Keyword& Lookup(Iter* it, Iter last) {
    // fill buffer 4 chars
    typedef typename Iter::value_type char_type;
    std::tr1::array<char_type, 4> buf = { { 0 } };
    std::size_t i = 0;
    for (;
         (*it != last) &&
         (i < 4) &&
         core::character::IsASCIIAlpha(**it);
         ++(*it), ++i) {
      buf[i] = (**it | 0x20);
    }
    return *(std::find_if(kKeywords.begin(),
                          kKeywords.end(),
                          Finder<char_type>(buf, i)));
  }
};

class DateParser : private core::Noncopyable<DateParser>::type {
 private:
  class DateComponent : private core::Noncopyable<DateComponent>::type {
   public:
    DateComponent()
      : month_(kNone),
        slots_(),
        pos_(0) {
      std::fill(slots_.begin(), slots_.end(), kNone);
    }

    void SetMonth(int month) {
      month_ = month;
    }

    void Add(int n) {
      if (pos_ < slots_.size()) {
        slots_[pos_++] = n;
      }
    }

    static bool IsMonthExpecting(uint32_t month) {
      return 1 <= month && month <= 12;
    }

    static bool IsDateExpecting(uint32_t date) {
      return 1 <= date && date <= 31;  // about counting
    }

    double MakeDay() const {
      if (month_ != kNone) {
        // named month is explicitly defined
        // DY
        const int day = slots_[0];
        const int year = CalcYear(slots_[1]);
        if (day != kNone && year != kNone) {
          return date::MakeDay(year, month_ - 1, day);
        } else {
          // invalid
          return JSValData::kNaN;
        }
      } else {
        // YMD
        const int year = CalcYear(slots_[0]);
        const int month = slots_[1];
        const int day = slots_[2];
        if (year != kNone && day != kNone && year != kNone) {
          return date::MakeDay(year, month - 1, day);
        } else {
          return JSValData::kNaN;
        }
      }
    }

   private:

    static int CalcYear(int year) {
      if (year < 100) {
        if (year < 50) {
          return year + 2000;
        } else {
          return year + 1900;
        }
      } else {
        return year;
      }
    }

    int month_;
    std::tr1::array<int, 3> slots_;
    std::size_t pos_;
  };

  class TimeComponent : private core::Noncopyable<TimeComponent>::type {
   public:
    TimeComponent()
      : hour_(0),
        min_(0),
        sec_(0),
        msec_(0),
        offset_(kNone) {
    }

    void SetHour(int hour) {
      hour_ = hour;
    }

    void SetMin(int min) {
      min_ = min;
    }

    void SetSec(int sec) {
      sec_ = sec;
    }

    void SetMSec(int msec) {
      msec_ = msec;
    }

    void SetOffset(int offset) {
      offset_ = offset;
    }

    double MakeTime() const {
      if (!IsHourExpecting(hour_)) {
        return JSValData::kNaN;
      }
      if (!IsMinExpecting(min_)) {
        return JSValData::kNaN;
      }
      if (!IsSecExpecting(sec_)) {
        return JSValData::kNaN;
      }
      if (!IsMSecExpecting(msec_)) {
        return JSValData::kNaN;
      }
      int hour = hour_;
      if (offset_ != kNone) {
        if (!IsOffsetHourExpecting(hour_)) {
          return JSValData::kNaN;
        }
        hour = hour_ % 12 + offset_;
      }
      return date::MakeTime(hour, min_, sec_, msec_);
    }

    static bool IsHourExpecting(uint32_t hour) {
      return hour < 24;
    }

    static bool IsOffsetHourExpecting(uint32_t hour) {
      return hour <= 12;
    }

    static bool IsMinExpecting(uint32_t min) {
      return min < 60;
    }

    static bool IsSecExpecting(uint32_t sec) {
      return sec < 60;
    }

    static bool IsMSecExpecting(uint32_t msec) {
      return msec < 1000;
    }
   private:
    int hour_;
    int min_;
    int sec_;
    int msec_;
    int offset_;
  };

  class TimezoneComponent : private core::Noncopyable<TimezoneComponent>::type {
   public:
    TimezoneComponent()
      : hour_(0),
        min_(0),
        sign_(1) {
    }

    bool IsUTC() const {
      return hour_ == 0 && min_ == 0;
    }

    void SetSign(int sign) {
      sign_ = sign;
    }

    void SetHour(int hour) {
      hour_ = hour;
    }

    void SetMin(int min) {
      min_ = min;
    }

    void SetTimezone(int tz) {
      if (tz < 0) {
        sign_ = -1;
        hour_ = -tz;
      } else {
        hour_ = tz;
      }
      min_ = 0;
    }

    double MakeTz(double value) const {
      return value - (sign_ * (hour_ * date::kMsPerHour + min_ * date::kMsPerMinute));
    }
   private:
    int hour_;
    int min_;
    int sign_;
  };

 public:
  DateParser()
    : date_(),
      time_(),
      tz_() {
  }

  template<typename Iter>
  uint32_t ReadUInt32(Iter* it, const Iter last) {
    assert(core::character::IsDigit(**it));  // start with digit
    uint32_t result = 0;
    for (; *it != last; ++(*it)) {
      if (core::character::IsDigit(**it)) {
        result = core::Radix36Value(**it) + result * 10;
      } else {
        return result;
      }
    }
    return result;
  }

  template<typename String>
  double Parse(const String& str);

 private:
  DateComponent date_;
  TimeComponent time_;
  TimezoneComponent tz_;
};

template<typename String>
double DateParser::Parse(const String& str) {
  typedef typename String::value_type char_type;
  for (typename String::const_iterator it = str.begin(),
       last = str.end(); it != last;) {
    if (core::character::IsDigit(*it)) {
      // YYYY or HH
      const uint32_t value = ReadUInt32(&it, last);
      if (it != last) {
        if (*it == ':') {
          // HH pattern
          time_.SetHour(value);
          ++it;
          if (it != last && core::character::IsDigit(*it)) {
            const uint32_t min = ReadUInt32(&it, last);
            time_.SetMin(min);
            if (*it == ':') {
              ++it;
              if (it != last && core::character::IsDigit(*it)) {
                const uint32_t sec = ReadUInt32(&it, last);
                time_.SetSec(sec);
                if (*it == '.') {
                  ++it;
                  if (it != last && core::character::IsDigit(*it)) {
                    const uint32_t msec = ReadUInt32(&it, last);
                    time_.SetMSec(msec);
                  }
                }
              }
            }
          }
        } else if (*it == '-') {
          // YYYY pattern
          date_.Add(value);
          ++it;
          if (it != last && core::character::IsDigit(*it)) {
            const uint32_t month = ReadUInt32(&it, last);
            date_.Add(month);
            if (*it == '-') {
              ++it;
              if (it != last && core::character::IsDigit(*it)) {
                const uint32_t d = ReadUInt32(&it, last);
                date_.Add(d);
              }
            }
          }
        } else {
          date_.Add(value);
        }
      } else {
        date_.Add(value);
      }
    } else if (core::character::IsASCIIAlpha(*it)) {
      const Keyword keyword = KeywordChecker::Lookup(&it, last);
      if (keyword.IsNone()) {
      } else if (keyword.IsMonth()) {
        date_.SetMonth(keyword.value);
      } else if (keyword.IsAMPM()) {
        time_.SetOffset(keyword.value);
      } else if (keyword.IsTimezone()) {
        tz_.SetTimezone(keyword.value);
      }
    } else if (*it == '(') {
      // skip paren
      ++it;
      for (; it != last; ++it) {
        if (*it == ')') {
          ++it;
          break;
        }
      }
    } else if ((*it == '+' || *it == '-') && tz_.IsUTC()) {
      // sign
      tz_.SetSign(*it == '+' ? 1 : -1);
      ++it;
      if (it != last && core::character::IsDigit(*it)) {
        const uint32_t value = ReadUInt32(&it, last);
        if (it != last && *it == ':') {
          ++it;
          tz_.SetHour(value);
          tz_.SetMin(0);
        } else {
          tz_.SetHour(value / 100);
          tz_.SetMin(value % 100);
        }
      }
    } else {
      ++it;
    }
  }

  return tz_.MakeTz(date::MakeDate(date_.MakeDay(),
                                   time_.MakeTime()));
}

template<typename String>
double Parse(const String& str) {
  DateParser parser;
  return parser.Parse(str);
}

} } }  // namespace iv::lv5::date
#endif  // _IV_LV5_DATE_PARSER_H_
