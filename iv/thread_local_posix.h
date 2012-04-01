#ifndef IV_THREAD_LOCAL_POSIX_H_
#define IV_THREAD_LOCAL_POSIX_H_
#include <pthread.h>
namespace iv {
namespace core {

template<typename T>
class ThreadLocalPtr {
 public:
  ThreadLocalPtr() : key_() {
    pthread_key_create(&key_, NULL);
  }

  ~ThreadLocalPtr() {
    pthread_key_delete(key_);
  }

  bool set(T* ptr) {
    pthread_setspecific(key_, reinterpret_cast<const void*>(ptr));
  }

  T* get() const {
    return reinterpret_cast<T*>(pthread_getspecific(key_));
  }

 private:
  pthread_key_t key_;
};

} }  // namespace iv::core
#endif  // IV_THREAD_LOCAL_POSIX_H_
