#ifndef IV_DUMMY_CC_H_
#define IV_DUMMY_CC_H_
#include <cstddef>
#include <iv/concat.h>
namespace iv {
namespace core {

template<std::size_t i>
class DummyCC {};

#define IV_DUMMY_CC()\
    static const ::iv::core::DummyCC<__LINE__> IV_CONCAT(VALUE, __LINE__) {};

} }  // namespace iv::core
#endif  // IV_DUMMY_CC_H_
