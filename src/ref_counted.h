#ifndef IV_REF_COUNTED_H_
#define IV_REF_COUNTED_H_
#include "noncopyable.h"
#include "platform.h"
namespace iv {
namespace core {

template<typename T>
class RefCounted : private iv::core::Noncopyable<T> {
 public:
  explicit RefCounted(int count = 1)
    : ref_(count) {
  }

  inline void Retain() {
    ++ref_;
  }

  inline void Release() {
    assert(ref_ != 0);
    if (!(--ref_)) {  // ref counter will be 0
      delete static_cast<T*>(this);
    }
  }

  int RetainCount() const {
    return ref_;
  }

 private:
  int ref_;
};

// for boost::intrusive_ptr
template<typename T>
inline void intrusive_ptr_add_ref(RefCounted<T>* obj) {
  obj->Retain();
}

template<typename T>
inline void intrusive_ptr_release(RefCounted<T>* obj) {
  obj->Release();
}

} }  // namespace iv::core
#endif  // IV_REF_COUNTED_H_
