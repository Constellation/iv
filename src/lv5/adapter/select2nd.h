#ifndef _IV_LV5_ADAPTER_SELECT2ND_H_
#define _IV_LV5_ADAPTER_SELECT2ND_H_
#include <functional>
namespace iv {
namespace lv5 {
namespace adapter {

template<class Pair>
struct select2nd : public std::unary_function<Pair, typename Pair::second_type> {
  const typename Pair::second_type& operator()(const Pair& x) const {
    return x.second;
  }
  typename Pair::second_type& operator()(Pair& x) {
    return x.second;
  }
};

} } }  // namespace iv::lv5::adapter
#endif  // _IV_LV5_ADAPTER_SELECT2ND_H_
