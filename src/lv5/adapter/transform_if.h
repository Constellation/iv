#ifndef _IV_LV5_ADAPTER_TRANSFORM_IF_H_
#define _IV_LV5_ADAPTER_TRANSFORM_IF_H_
namespace iv {
namespace lv5 {
namespace adapter {

template<class InputIterator,
         class OutputIterator,
         class UnaryFunction,
         class Predicate>
inline OutputIterator transform_if(InputIterator first,
                                   InputIterator last,
                                   OutputIterator result,
                                   UnaryFunction f,
                                   Predicate pred) {
  while (first != last) {
    if (pred(*first)) {
      *result++ = f(*first);
    }
    ++first;
  }
  return result;
}

} } }  // namespace iv::lv5::adapter
#endif  // _IV_LV5_ADAPTER_TRANSFORM_IF_H_
