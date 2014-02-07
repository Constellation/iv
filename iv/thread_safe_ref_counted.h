#ifndef IV_THREAD_SAFE_REF_COUNTED_H_
#define IV_THREAD_SAFE_REF_COUNTED_H_
#include <atomic>
#include <iv/noncopyable.h>
#include <iv/platform.h>
namespace iv {
namespace core {

template<typename T>
class ThreadSafeRefCounted : private iv::core::Noncopyable<T> {
 public:
  explicit ThreadSafeRefCounted(int count = 1)
    : ref_(count) {
  }

  inline void Retain() const {
    std::atomic_fetch_add_explicit(&ref_, 1u, std::memory_order_relaxed);
  }

  inline void Release() const {
    // Reference counter becomes 0.
    if (std::atomic_fetch_sub_explicit(
            &ref_,
            1u,
            std::memory_order_release) == 1) {
      // Memory fence is inserted to ensure delete should be executed after
      // decrementing the counter.
#if defined(IV_COMPILER_GCC) && IV_COMPILER_GCC < IV_MAKE_VERSION(4, 7, 0)
      // Memory fence operation is not implemented until GCC 4.7,
      // so we use __sync_synchronize builtin function instead.
      // http://stackoverflow.com/questions/16429669/stdatomic-thread-fence-has-undefined-reference
      __sync_synchronize();
#else
      std::atomic_thread_fence(std::memory_order_acquire);
#endif
      delete static_cast<const T*>(this);
    }
  }

  unsigned RetainCount() const {
    return ref_.load();
  }

 private:
  mutable std::atomic<unsigned> ref_;
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
