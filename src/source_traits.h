#ifndef _IV_SOURCE_TRAITS_H_
#define _IV_SOURCE_TRAITS_H_
#include <string>
namespace iv {
namespace core {

template<typename T>
struct SourceTraits {
  static std::string FileName(const T& src) {
    return "<anonymous>";
  }
};

} }  // namespace iv::core
#endif  // _IV_SOURCE_TRAITS_H_
