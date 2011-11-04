#ifndef IV_INTRUSIVE_PTR_H_
#define IV_INTRUSIVE_PTR_H_
#include <algorithm>
namespace iv {
namespace core {

template<typename T>
class IntrusivePtr {
 public:
  typedef IntrusivePtr<T> this_type;
  typedef void (this_type::*bool_type)() const;

  explicit IntrusivePtr(T* ptr = NULL, bool not_retain = true)
    : ptr_(ptr) {
    if (not_retain) {
      Retain(ptr_);
    }
  }

  IntrusivePtr(const this_type& rhs)
    : ptr_(rhs.get()) {
    Retain(ptr_);
  }

  template<typename U>
  IntrusivePtr(const IntrusivePtr<U>& rhs)
    : ptr_(rhs.get()) {
    Retain(ptr_);
  }

  ~IntrusivePtr() { Release(ptr_); }

  void reset(T* ptr = NULL) {
    if (ptr_ != ptr) {
      Retain(ptr);
      Release(ptr_);
      ptr_ = ptr;
    }
  }

  T& operator*() const {
    return *ptr_;
  }

  T* operator->() const {
    return ptr_;
  }

  T* get() const {
    return ptr_;
  }

  bool operator==(T* rhs) const {
    return ptr_ == rhs;
  }

  bool operator!=(T* rhs) const {
    return ptr_ != rhs;
  }

  T* release() const {
    Release(ptr_);
    T* res = ptr_;
    ptr_ = NULL;
    return res;
  }

  void swap(this_type& rhs) {
    using std::swap;
    swap(ptr_, rhs.ptr_);
  }

  operator bool_type() const {
    return (ptr_ == NULL) ?
        0 : &this_type::this_type_does_not_support_comparisons;
  }

  bool operator!() const {
    return ptr_ == NULL;
  }

  this_type& operator=(const this_type& rhs) {
    reset(rhs.get());
    return *this;
  }

  template<typename U>
  this_type& operator=(const IntrusivePtr<U>& rhs) {
    reset(rhs.get());
    return *this;
  }

  this_type& operator=(T* rhs) {
    reset(rhs);
    return *this;
  }

 private:
  void this_type_does_not_support_comparisons() const { }

  static inline void Retain(T* ptr) {
    if (ptr) {
      intrusive_ptr_add_ref(ptr);
    }
  }

  static inline void Release(T* ptr) {
    if (ptr) {
      intrusive_ptr_release(ptr);
    }
  }

  T* ptr_;
};

template <typename T>
inline void swap(IntrusivePtr<T>& lhs, IntrusivePtr<T>& rhs) {
  lhs.swap(rhs);
}

template<typename T, typename U>
inline bool operator==(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) {
  return lhs.get() == rhs.get();
}

template<typename T, typename U>
inline bool operator!=(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) {
  return lhs.get() != rhs.get();
}

template <typename T>
inline bool operator==(const IntrusivePtr<T>& lhs, T* rhs) {
  return lhs.get() == rhs;
}

template <typename T>
inline bool operator!=(const IntrusivePtr<T>& lhs, T* rhs) {
  return lhs.get() != rhs;
}

template <typename T>
inline bool operator==(T* lhs, const IntrusivePtr<T>& rhs) {
  return lhs == rhs.get();
}

template <typename T>
inline bool operator!=(T* lhs, const IntrusivePtr<T>& rhs) {
  return lhs != rhs.get();
}

template <typename T, typename U>
inline bool operator<(const IntrusivePtr<T>& lhs, const IntrusivePtr<U>& rhs) {
  return lhs.get() < rhs.get();
}

template <typename T>
inline T* get_pointer(const IntrusivePtr<T>& ptr) {
  return ptr.get();
}

template <typename T, typename U>
inline IntrusivePtr<T> static_pointer_cast(const IntrusivePtr<U>& ptr) {
  return IntrusivePtr<T>(static_cast<T*>(ptr.get()));
}

template <typename T, typename U>
inline IntrusivePtr<T> dynamic_pointer_cast(const IntrusivePtr<U>& ptr) {
  return IntrusivePtr<T>(dynamic_cast<T*>(ptr.get()));
}

} }  // namespace iv::core
#endif  // IV_INTRUSIVE_PTR_H_
