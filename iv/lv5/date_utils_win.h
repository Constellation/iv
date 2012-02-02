// Windows implementation of date_utils some functions
// see also
//
// GetTimeZoneInformation function
// http://msdn.microsoft.com/en-us/library/ms724421(VS.85).aspx
//
// TIME_ZONE_INFORMATION struct
// http://msdn.microsoft.com/en-us/library/ms725481(v=VS.85).aspx
//
// WideCharToMultiByte function
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd374130(v=vs.85).aspx
//
#ifndef IV_LV5_DATE_UTILS_WIN_H_
#define IV_LV5_DATE_UTILS_WIN_H_
#include <windows.h>
#include <iv/detail/array.h>
#include <iv/singleton.h>
#include <iv/platform_math.h>
namespace iv {
namespace lv5 {
namespace date {

// UTF-8 code max length is 4
// WCHAR 32 size to multibyte requires 32 * 4 space
static const int kMaxTZNameSize = 32 * 4;

inline std::time_t FileTimeToUnixTime(const FILETIME& ft) {
  LARGE_INTEGER i;
  i.LowPart = ft.dwLowDateTime;
  i.HighPart = ft.dwHighDateTime;
  return (i.QuadPart - kEpochTime) / 10 / 1000000;
}

inline double FileTimeToMs(const FILETIME& ft) {
  LARGE_INTEGER i;
  i.LowPart = ft.dwLowDateTime;
  i.HighPart = ft.dwHighDateTime;
  return (i.QuadPart - kEpochTime) / 10000.0;
}

inline std::time_t SystemTimeToUnixTime(const SYSTEMTIME& st) {
  FILETIME ft;
  ::SystemTimeToFileTime(&st, &ft);
  return FileTimeToUnixTime(ft);
}

inline double DaylightSavingTA(double utc) {
  // GetTimeZoneInformation
  // http://msdn.microsoft.com/en-us/library/ms724421(VS.85).aspx
  if (core::math::IsNaN(utc)) {
    return utc;
  }
  TIME_ZONE_INFORMATION tzi;
  const DWORD r = ::GetTimeZoneInformation(&tzi);
  if (r == TIME_ZONE_ID_STANDARD || r == TIME_ZONE_ID_DAYLIGHT) {
    // has DST
    const double local = utc + LocalTZA();
    if (tzi.StandardDate.wMonth == 0 || tzi.DaylightDate.wMonth == 0) {
      return 0.0;
    }

    const std::time_t ts = SystemTimeToUnixTime(tzi.StandardDate);
    const std::time_t td = SystemTimeToUnixTime(tzi.DaylightDate);

    if (td <= local && local <= ts) {
      return - tzi.DaylightBias * (60 * kMsPerSecond);
    } else {
      return 0.0;
    }
  }
  // r is TIME_ZONE_ID_UNKNOWN or TIME_ZONE_ID_INVALID
  // Daylight Saving Time not used in this time zone or failed
  return 0.0;
}

inline double CurrentTime() {
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  return FileTimeToMs(ft);
}

class HiResTimeCounter : public core::Singleton<HiResTimeCounter> {
 public:
  friend class core::Singleton<HiResTimeCounter>;

  double GetHiResTime() const {
    return (query_performance_is_used_) ? CalculateHiResTime() : CurrentTime();
  }

 private:
  HiResTimeCounter()
    : frequency_(),
      query_performance_is_used_(false),
      start_(),
      counter_() {
    if (::QueryPerformanceFrequency(&frequency_)) {
      query_performance_is_used_ = true;
      FILETIME ft;
      FILETIME start;
      ::GetSystemTimeAsFileTime(&ft);
      do {
        ::GetSystemTimeAsFileTime(&start);
        ::QueryPerformanceCounter(&counter_);
      } while ((ft.dwHighDateTime == start.dwHighDateTime) &&
               (ft.dwLowDateTime == start.dwLowDateTime));
      start_ = FileTimeToMs(start);
    }
  }

  ~HiResTimeCounter() { }  // private destructor

  double CalculateHiResTime() const {
    LARGE_INTEGER i;
    ::QueryPerformanceCounter(&i);
    return start_ +
        ((i.QuadPart - counter_.QuadPart) * 1000.0 / frequency_.QuadPart);
  }

  LARGE_INTEGER frequency_;
  bool query_performance_is_used_;
  double start_;
  LARGE_INTEGER counter_;
};

inline double HighResTime() {
  return HiResTimeCounter::Instance()->GetHiResTime();
}

inline std::array<char, kMaxTZNameSize> LocalTimeZoneImpl(double t) {
  std::array<char, kMaxTZNameSize> buffer = { { } };
  TIME_ZONE_INFORMATION tzi;
  const DWORD r = ::GetTimeZoneInformation(&tzi);
  switch (r) {
    case TIME_ZONE_ID_STANDARD:
    case TIME_ZONE_ID_UNKNOWN: {
      // no DST or not DST
      //
      // no dwFlags set when converting
      ::WideCharToMultiByte(CP_UTF8,
                            0,
                            tzi.StandardName,
                            -1,
                            buffer.data(),
                            buffer.size(),
                            NULL,
                            NULL);
      buffer[buffer.size() - 1] = '\0';
      break;
    }
    case TIME_ZONE_ID_DAYLIGHT: {
      ::WideCharToMultiByte(CP_UTF8,
                            0,
                            tzi.DaylightName,
                            -1,
                            buffer.data(),
                            buffer.size(),
                            NULL,
                            NULL);
      buffer[buffer.size() - 1] = '\0';
      break;
    }
  }
  return buffer;
}

inline const char* LocalTimeZone(double t) {
  if (core::math::IsNaN(t)) {
    return "";
  }
  static const std::array<char, kMaxTZNameSize> kTZ = LocalTimeZoneImpl(t);
  return kTZ.data();
}

} } }  // namespace iv::lv5::date
#endif  // IV_LV5_DATE_UTILS_WIN_H_
