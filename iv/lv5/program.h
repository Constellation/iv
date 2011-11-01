#ifndef IV_LV5_PROGRAM_H_
#define IV_LV5_PROGRAM_H_
#include <string>
#include <vector>

namespace iv {
namespace lv5 {
namespace program {

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

void Init(int argc, char ** argv) {
  Program::Init(argc, argv);
}

} } }  // namespace iv::lv5::program
#endif  // IV_LV5_PROGRAM_H_
