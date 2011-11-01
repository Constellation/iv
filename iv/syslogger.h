// this syslog header file only used in debug
#ifndef IV_SYSLOGGER_H_
#define IV_SYSLOGGER_H_
#include <cstdio>
#include <string>
#include <syslog.h>
#include <iv/noncopyable.h>
namespace iv {
namespace core {

class Syslog : private Noncopyable<> {
 public:
  explicit Syslog(const std::string& ident) {
    openlog(ident.c_str(), LOG_CONS | LOG_PID, LOG_USER);
  }

  ~Syslog() {
    closelog();
  }

  template<int priority>
  void Log(const std::string& message) {
    syslog(priority, "%s", message.c_str());
  }
};

} }  // namespace iv::core
#endif  // IV_SYSLOGGER_H_
