#ifndef _IV_FUNCTOR_H_
#define _IV_FUNCTOR_H_
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

template<typename T>
struct Destructor : public std::unary_function<T, void> {
  void operator()(T* p) const {
    p->~T();
  }
};

} }  // namespace iv::core
#endif  // _IV_FUNCTOR_H_
