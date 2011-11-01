#ifndef IV_SINGLETON_H_
#define IV_SINGLETON_H_
#include <cstdlib>
#include <iv/noncopyable.h>
#include <iv/callonce.h>
namespace iv {
namespace core {

template<class T>
class Singleton : private core::Noncopyable<T> {
 public:
  static T* Instance() {
    static thread::Once once = IV_ONCE_INIT;
    thread::CallOnce(&once, &Singleton<T>::Initialize);
    return instance_;
  }

 private:
  static void Initialize() {
    assert(instance_ == NULL);
    instance_ = new T;
    std::atexit(&Singleton<T>::Destroy);
  }

  static void Destroy() {
    delete instance_;
    instance_ = NULL;
  }

  static T* instance_;
};

template<class T>
T* Singleton<T>::instance_ = NULL;

} }  // namespace iv::core
#endif  // IV_SINGLETON_H_
