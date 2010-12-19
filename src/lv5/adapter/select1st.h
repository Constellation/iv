#ifndef _IV_LV5_ADAPTER_SELECT1ST_H_
#define _IV_LV5_ADAPTER_SELECT1ST_H_
#include <functional>
namespace iv {
namespace lv5 {
namespace adapter {

template<class Pair>
struct select1st : public std::unary_function<Pair, typename Pair::first_type> {
  const typename Pair::first_type& operator()(const Pair& x) const {
    return x.first;
  }
  typename Pair::first_type& operator()(Pair& x) {
    return x.first;
  }
};

} } }  // namespace iv::lv5::adapter
#endif  // _IV_LV5_ADAPTER_SELECT1ST_H_
