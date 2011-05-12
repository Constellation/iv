#ifndef _IV_SINGLETON_H_
#define _IV_SINGLETON_H_
#include <cstdlib>
#include "noncopyable.h"
#include "callonce.h"
namespace iv {
namespace core {

template<class T>
class Singleton : private core::Noncopyable<T> {
 public:
  static T* Instance() {
    thread::CallOnce(&once_, &Singleton<T>::Initialize);
    return instance_;
  }

 private:
  static void Initialize() {
    instance_ = new T;
    std::atexit(&Singleton<T>::Destroy);
  }

  static void Destroy() {
    delete instance_;
    instance_ = NULL;
    thread::ResetOnce(&once_);
  }

  static thread::Once once_;
  static T* instance_;
};

template<class T>
thread::Once Singleton<T>::once_;

template<class T>
T* Singleton<T>::instance_ = NULL;


} }  // namespace iv::core
#endif  // _IV_SINGLETON_H_
