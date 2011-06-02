#ifndef _IV_USTRING_H_
#define _IV_USTRING_H_
#include <string>
#include "detail/tr1/functional.h"
#include "uchar.h"
#include "stringpiece.h"
#include "conversions.h"
namespace iv {
namespace core {

typedef std::basic_string<uc16,
                          std::char_traits<uc16> > UString;

inline UString ToUString(StringPiece str) {
  return UString(str.begin(), str.end());
}

} }  // namespace iv::core

#ifdef BOOST_HAS_TR1_HASH
namespace std {
namespace tr1 {
#else
namespace boost {
#endif  // ifdef BOOST_HAS_TR1_HASH
// template specialization for UString in std::tr1::unordered_map
// allowed in section 17.4.3.1
template<>
struct hash<iv::core::UString>
  : public std::unary_function<iv::core::UString, std::size_t> {
  result_type operator()(const argument_type& x) const {
    return iv::core::StringToHash(x);
  }
};
#ifdef BOOST_HAS_TR1_HASH
} }  // namespace std::tr1
#else
}  // namespace boost
#endif  // ifdef BOOST_HAS_TR1_HASH

#endif  // _IV_USTRING_H_
