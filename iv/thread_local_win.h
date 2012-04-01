#ifndef IV_THREAD_LOCAL_WIN_H_
#define IV_THREAD_LOCAL_WIN_H_
#include <windows.h>
namespace iv {
namespace core {

template<typename T>
class ThreadLocalPtr {
 public:
  ThreadLocalPtr() : key_(::TlsAlloc()) { }

  ~ThreadLocalPtr() {
    TlsFree(key_);
  }

  bool set(T* val) {
    return ::TlsSetValue(key_, reinterpret_cast<LPVOID>(val));
  }

  T* get() const {
    return reinterpret_cast<T*>(::TlsGetValue(key_));
  }

 private:
  DWORD key_;
};


} }  // namespace iv::core
#endif  // IV_THREAD_LOCAL_WIN_H_
