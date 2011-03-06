#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <sstream>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>

namespace test {

void Init(int argc, char ** argv);

template<typename T>
class ProgramImpl {
 private:
  friend void Init(int argc, char** argv);
  static ProgramImpl* instance_;

  static void Init(int argc, char **argv) {
    static ProgramImpl instance(argc, argv);
    instance_ = &instance;
  }

 public:
  static ProgramImpl* Instance() {
    return instance_;
  }

  const std::string& program() const {
    return program_;
  }

  const std::vector<std::string>& args() const {
    return args_;
  }

 private:
  ProgramImpl(int argc, char **argv)
    : program_(argv[0]),
      args_() {
    assert(argc > 0);
    for (int i = 1; i < argc; ++i) {
      args_.push_back(argv[i]);
    }
  }

  std::string program_;
  std::vector<std::string> args_;
};

template<typename T>
ProgramImpl<T>* ProgramImpl<T>::instance_ = NULL;
typedef ProgramImpl<void> Program;

class Syslog {
 public:
  explicit Syslog(const std::string& program) {
    openlog(program.c_str(), LOG_CONS | LOG_PID, LOG_USER);
  }
  ~Syslog() {
    closelog();
  }
  template<int priority>
  void Log(const std::string& message) {
    syslog(priority, "%s", message.c_str());
  }
};

static void SegvHandler(int sn, siginfo_t* si, void* src) {
  {
    std::stringstream ss;
    Program* program = Program::Instance();
    ss << program->program() << ":";
    std::copy(program->args().begin(),
              program->args().end(),
              std::ostream_iterator<std::string>(ss, " "));
    Syslog log(program->program());
    log.Log<LOG_INFO>(ss.str());
  }
  std::abort();
}

void Init(int argc, char ** argv) {
  struct sigaction sig;
  sig.sa_flags = SA_SIGINFO;
  sig.sa_sigaction = &SegvHandler;
  sigaction(SIGSEGV, &sig, reinterpret_cast<struct sigaction*>(NULL));
  Program::Init(argc, argv);
}

}  // namespace test

int main(int argc, char** argv) {
  test::Init(argc, argv);

  std::vector<int> vec;

  for (int i = 0; i < 10000000; ++i) {
    vec[i] = 10;
  }
  return 0;
}
