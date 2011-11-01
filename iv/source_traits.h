#ifndef IV_SOURCE_TRAITS_H_
#define IV_SOURCE_TRAITS_H_
#include <string>
namespace iv {
namespace core {

template<typename T>
struct SourceTraits {
  static std::string GetFileName(const T& src) {
    return "<anonymous>";
  }
};

} }  // namespace iv::core
#endif  // IV_SOURCE_TRAITS_H_
