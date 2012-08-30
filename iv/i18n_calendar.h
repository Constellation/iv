#ifndef IV_I18N_CALENDAR_H_
#define IV_I18N_CALENDAR_H_
#include <iv/detail/array.h>
namespace iv {
namespace core {
namespace i18n {

class Calendar {
 public:
  enum Type {
    SENTINEL = 0,  // this is sentinel value
    GREGORY = 1
  };
  static const int kNumOfCalendars = 1;
  typedef std::array<Type, kNumOfCalendars> Candidate;
};

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_CALENDAR_H_
