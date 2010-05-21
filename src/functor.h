#ifndef _IV_FUNCTOR_H_
#define _IV_FUNCTOR_H_
namespace iv {
namespace core {

template<class T>
struct Deleter {
  void operator()(T* p) const {
    delete p;
  }
};

template<class T>
struct Destructor {
  void operator()(T* p) const {
    p->~T();
  }
};

} }  // namespace iv::core
#endif  // _IV_FUNCTOR_H_

