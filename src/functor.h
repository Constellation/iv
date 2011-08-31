#ifndef IV_FUNCTOR_H_
#define IV_FUNCTOR_H_
#include <functional>
namespace iv {
namespace core {

template<typename T>
struct Deleter : public std::unary_function<T, void> {
  void operator()(T* p) const {
    delete p;
  }
};

template<typename T>
struct Deleter<T[]> : public std::unary_function<T[], void> {
  void operator()(T* p) const {
    delete [] p;
  }
};

} }  // namespace iv::core
#endif  // IV_FUNCTOR_H_
