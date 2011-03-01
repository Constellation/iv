#ifndef _IV_LV5_DATE_UTILS_WIN_H_
#define _IV_LV5_DATE_UTILS_WIN_H_
#include <windows.h>
namespace iv {
namespace lv5 {
namespace date {

inline std::time_t FileTimeToUnixTime(const FILETIME& ft) {
  LARGE_INTEGER i;
  i.LowPart = ft.dwLowDateTime;
  i.HighPart = ft.dwHighDateTime;
  return (i.QuadPart - kEpochTime) / 10 / 1000000;
}

inline std::time_t SystemTimeToUnixTime(const SYSTEMTIME& st) {
  FILETIME ft;
  ::SystemTimeToFileTime(&st, &ft);
  return FileTimeToUnixTime(ft);
}

inline double DaylightSavingTA(double utc) {
  // http://msdn.microsoft.com/en-us/library/ms724421
  if (std::isnan(utc)) {
    return utc;
  }
  TIME_ZONE_INFORMATION tzi;
  const DWORD r = ::GetTimeZoneInformation(&tzi);
  const double local = utc + LocalTZA();
  switch (r) {
    case TIME_ZONE_ID_STANDARD:
    case TIME_ZONE_ID_DAYLIGHT: {
      if (tzi.StandardDate.wMonth == 0 ||
          tzi.DaylightDate.wMonth == 0) {
        break;
      }

      const std::time_t ts = SystemTimeToUnixTime(tzi.StandardDate);
      const std::time_t td = SystemTimeToUnixTime(tzi.DaylightDate);

      if (td <= local && local <= ts) {
        return - tzi.DaylightBias * (60 * kMsPerSecond);
      } else {
        return 0.0;
      }
    }
    case TIME_ZONE_ID_UNKNOWN: {
      // Daylight Saving Time not used in this time zone
      return 0.0;
    }
  }
  return 0.0;
}

inline double CurrentTime() {
  FILETIME ft;
  ::GetSystemTimeAsFileTime(&ft);
  return FileTimeToUnixTime(ft);
}

} } }  // namespace iv::lv5::date
#endif  // _IV_LV5_DATE_UTILS_WIN_H_
