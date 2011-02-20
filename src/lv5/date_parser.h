#ifndef _IV_LV5_DATE_PARSER_H_
#define _IV_LV5_DATE_PARSER_H_
#include <tr1/array>
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
    std::size_t len_;
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
      : year_(0),
        month_(0),
        date_(0) {
    }
    void SetYear(int year) {
      if (year < 100) {
        if (year < 50) {
          year_ = year + 2000;
        } else {
          year_ = year + 1900;
        }
      } else {
        year_ = year;
      }
    }
    void SetMonth(int month) {
      month_ = month;
    }
    void SetDate(int date) {
      date_ = date;
    }

    bool IsValid() const {
      if (!IsMonthExpecting(month_)) {
        return false;
      }
      if (!IsDateExpecting(date_)) {
        return false;
      }
      return true;
    }

    static bool IsMonthExpecting(uint32_t month) {
      return 1 <= month && month <= 12;
    }

    static bool IsDateExpecting(uint32_t date) {
      return 1 <= date && date <= 31;  // about counting
    }

    double MakeDay() const {
      return date::MakeDay(year_, month_, date_);
    }
   private:
    int year_;
    int month_;
    int date_;
  };

  class TimeComponent : private core::Noncopyable<TimeComponent>::type {
   public:
    TimeComponent()
      : hour_(0),
        min_(0),
        sec_(0),
        msec_(0) {
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

    bool IsValid() const {
      if (!IsHourExpecting(hour_)) {
        return false;
      }
      if (!IsMinExpecting(min_)) {
        return false;
      }
      if (!IsSecExpecting(sec_)) {
        return false;
      }
      if (!IsMSecExpecting(msec_)) {
        return false;
      }
      return true;
    }

    double MakeTime() const {
      return date::MakeTime(hour_, min_, sec_, msec_);
    }

    static bool IsHourExpecting(uint32_t hour) {
      return hour < 24;
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
  };

  class TimezoneComponent : private core::Noncopyable<TimezoneComponent>::type {
   public:
    TimezoneComponent()
      : adjust_(0) {
    }
    void SetAdjust(int adjust) {
      adjust_ = adjust;
    }

    bool IsValid() const {
      return true;
    }
   private:
    int adjust_;
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
          std::cout << "hour:" << value << std::endl;
          time_.SetHour(value);
          ++it;
          if (it != last && core::character::IsDigit(*it)) {
            const uint32_t min = ReadUInt32(&it, last);
            std::cout << "min:" << min << std::endl;
            time_.SetMin(min);
            if (*it == ':') {
              ++it;
              if (it != last && core::character::IsDigit(*it)) {
                const uint32_t sec = ReadUInt32(&it, last);
                std::cout << "sec:" << sec << std::endl;
                time_.SetSec(sec);
                if (*it == '.') {
                  ++it;
                  if (it != last && core::character::IsDigit(*it)) {
                    const uint32_t msec = ReadUInt32(&it, last);
                    std::cout << "msec:" << msec << std::endl;
                    time_.SetMSec(msec);
                  }
                }
              }
            }
          }
        } else if (*it == '-') {
          // YYYY pattern
          date_.SetYear(value);
          std::cout << "year:" << value << std::endl;
          ++it;
          if (it != last && core::character::IsDigit(*it)) {
            const uint32_t month = ReadUInt32(&it, last);
            std::cout << "month:" << month << std::endl;
            date_.SetMonth(month);
            if (*it == '-') {
              ++it;
              if (it != last && core::character::IsDigit(*it)) {
                const uint32_t d = ReadUInt32(&it, last);
                std::cout << "day:" << d << std::endl;
                date_.SetDate(d);
              }
            }
          }
        } else {
          std::cout << "year:" << value << std::endl;
          date_.SetYear(value);
        }
      } else {
        std::cout << "year:" << value << std::endl;
        date_.SetYear(value);
      }
    } else if (core::character::IsASCIIAlpha(*it)) {
      std::cout << "ALPHA" << std::endl;
      const Keyword keyword = KeywordChecker::Lookup(&it, last);
      if (keyword.IsNone()) {
        std::cout << "NOT" << std::endl;
      } else if (keyword.IsMonth()) {
        std::cout << "MONTH" << std::endl;
      } else if (keyword.IsAMPM()) {
        std::cout << "AMPM" << std::endl;
      } else if (keyword.IsTimezone()) {
        std::cout << "TZ" << std::endl;
      }
    } else if (*it == '(') {
      // skip paren
      ++it;
      for (; it != last; ++it) {
        if (*it == ')') {
          break;
        }
      }
    } else {
      ++it;
    }
  }

  if (date_.IsValid() && time_.IsValid() && tz_.IsValid()) {
    return date::UTC(date::MakeDate(date_.MakeDay(),
                                    time_.MakeTime()));
  }
  return JSValData::kNaN;
}

template<typename String>
double Parse(const String& str) {
  return 0.0;
//  DateParser parser;
//  return parser.Parse(str);
}

} } }  // namespace iv::lv5::date
#endif  // _IV_LV5_DATE_PARSER_H_
