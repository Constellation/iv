#ifndef _IV_USTRING_H_
#define _IV_USTRING_H_
#include <string>
#include <functional>
#include <tr1/functional>
#include "uchar.h"
#include "conversions.h"
namespace iv {
namespace core {

typedef std::basic_string<uc16,
                          std::char_traits<uc16> > UString;
} }  // namespace iv::core
namespace std {
namespace tr1 {

// template specialization for UString in std::tr1::unordered_map
// allowed in section 17.4.3.1
template<>
struct hash<iv::core::UString>
  : public unary_function<iv::core::UString, std::size_t> {
  result_type operator()(const argument_type& x) const {
    return iv::core::StringToHash(x);
  }
};

} }  // namespace std::tr1
#endif  // _IV_USTRING_H_
