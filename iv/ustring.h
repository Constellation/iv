#ifndef IV_USTRING_H_
#define IV_USTRING_H_
#include <string>
#include <iv/detail/functional.h>
#include <iv/detail/cstdint.h>
#include <iv/stringpiece.h>
#include <iv/conversions.h>
namespace iv {
namespace core {

typedef std::basic_string<uint16_t,
                          std::char_traits<uint16_t> > UString;

inline UString ToUString(StringPiece str) {
  return UString(str.begin(), str.end());
}

inline UString ToUString(uint16_t ch) {
  return UString(&ch, &ch + 1);
}

} }  // namespace iv::core

namespace IV_HASH_NAMESPACE_START {

// template specialization for UString in std::unordered_map
// allowed in section 17.4.3.1
template<>
struct hash<iv::core::UString>
  : public std::unary_function<iv::core::UString, std::size_t> {
  result_type operator()(const argument_type& x) const {
    return iv::core::StringToHash(x);
  }
};

} IV_HASH_NAMESPACE_END  // namespace std

#endif  // IV_USTRING_H_
