#ifndef _IV_ALGORITHM_H
#define _IV_ALGORITHM_H_
namespace iv {
namespace algorithm {
template<class InputIterator,
         class OutputIterator,
         class UnaryFunction,
         class Predicate>
OutputIterator transform_if(InputIterator first,
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
} }  // namespace iv::algorithm
#endif  // _IV_ALGORITHM_H_
