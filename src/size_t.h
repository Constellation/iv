#ifndef IV_LV5_SIZE_T_H_
#define IV_LV5_SIZE_T_H_
#include <cstddef>
namespace iv {
namespace lv5 {
template<typename T>
struct Format {
  static const char * printf;
};

template<typename T>
const char* Format<T>::printf = "%u";

template<>
struct Format<unsigned long long> {  // NOLINT
  static const char * printf;
};

const char* Format<unsigned long long>::printf = "%lu";  // NOLINT

} }  // namespace iv::lv5
#endif  // IV_LV5_INTERPRETER_H_
