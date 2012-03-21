#ifndef IV_THREAD_SAFE_REF_COUNTED_H_
#define IV_THREAD_SAFE_REF_COUNTED_H_
#include <iv/noncopyable.h>
#include <iv/atomic.h>
namespace iv {
namespace core {

template<typename T>
class ThreadSafeRefCounted : private iv::core::Noncopyable<T> {
 public:
  explicit ThreadSafeRefCounted(int count = 1)
    : ref_(count) {
  }

  inline void Retain() const {
    AtomicIncrement(&ref_);
  }

  inline void Release() const {
    assert(ref_ != 0);
    if (!(AtomicDecrement(&ref_))) {  // ref counter will be 0
      delete static_cast<const T*>(this);
    }
  }

  int RetainCount() const {
    return ref_;
  }

 private:
  mutable int ref_;
};

// for boost::intrusive_ptr
template<typename T>
inline void intrusive_ptr_add_ref(const ThreadSafeRefCounted<T>* obj) {
  obj->Retain();
}

template<typename T>
inline void intrusive_ptr_release(const ThreadSafeRefCounted<T>* obj) {
  obj->Release();
}

} }  // namespace iv::core
#endif  // IV_REF_COUNTED_H_
