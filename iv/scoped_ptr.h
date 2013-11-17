#ifndef IV_SCOPRED_PTR_H_
#define IV_SCOPRED_PTR_H_
#include <algorithm>
#include <iv/noncopyable.h>
namespace iv {
namespace core {

template<class T>
class ScopedPtr : private Noncopyable<ScopedPtr<T> > {
 public:
  typedef ScopedPtr<T> this_type;
  typedef void (this_type::*bool_type)() const;

  explicit ScopedPtr(T* ptr = nullptr) : ptr_(ptr) { }

  ~ScopedPtr() {
    delete ptr_;
  }

  void reset(T* ptr = nullptr) {
    if (ptr_ != ptr) {
      delete ptr_;
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
    T* res = ptr_;
    ptr_ = nullptr;
    return res;
  }

  void swap(ScopedPtr& rhs) {
    using std::swap;
    swap(ptr_, rhs.ptr_);
  }

  operator bool_type() const {
    return (ptr_ == nullptr) ?
        0 : &this_type::this_type_does_not_support_comparisons;
  }

  bool operator!() const {
    return ptr_ == nullptr;
  }

 private:
  void this_type_does_not_support_comparisons() const { }

  T* ptr_;

  template<typename U> bool operator==(const ScopedPtr<U>& rhs) const;
  template<typename U> bool operator!=(const ScopedPtr<U>& rhs) const;
};

template <class T>
void swap(ScopedPtr<T>& lhs, ScopedPtr<T>& rhs) {
  lhs.swap(rhs);
}

template <class T>
bool operator==(T* lhs, const ScopedPtr<T>& rhs) {
  return lhs == rhs.get();
}

template <class T>
bool operator!=(T* lhs, const ScopedPtr<T>& rhs) {
  return lhs != rhs.get();
}

template<class T>
class ScopedPtr<T[]> : Noncopyable<ScopedPtr<T[]> > {
 public:
  typedef ScopedPtr<T> this_type;
  typedef void (this_type::*bool_type)() const;
  explicit ScopedPtr(T* ptr = nullptr) : ptr_(ptr) { }

  ~ScopedPtr() {
    delete [] ptr_;
  }

  void reset(T* ptr = nullptr) {
    if (ptr_ != ptr) {
      delete [] ptr_;
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

  T& operator[](std::size_t n) const {
    return ptr_[n];
  }

  bool operator==(T* rhs) const {
    return ptr_ == rhs;
  }

  bool operator!=(T* rhs) const {
    return ptr_ != rhs;
  }

  T* release() const {
    T* res = ptr_;
    ptr_ = nullptr;
    return res;
  }

  void swap(ScopedPtr& rhs) {
    std::swap(ptr_, rhs.ptr_);
  }

  operator bool_type() const {
    return (ptr_ == nullptr) ?
        0 : &this_type::this_type_does_not_support_comparisons;
  }

  bool operator!() const {
    return ptr_ == nullptr;
  }

 private:
  void this_type_does_not_support_comparisons() const { }

  T* ptr_;

  template<typename U> bool operator==(const ScopedPtr<U>& rhs) const;
  template<typename U> bool operator!=(const ScopedPtr<U>& rhs) const;
};

template <class T>
inline void swap(ScopedPtr<T[]>& lhs, ScopedPtr<T[]>& rhs) {
  lhs.swap(rhs);
}

template <class T>
inline bool operator==(T* lhs, const ScopedPtr<T[]>& rhs) {
  return lhs == rhs.get();
}

template <class T>
inline bool operator!=(T* lhs, const ScopedPtr<T[]>& rhs) {
  return lhs != rhs.get();
}

} }  // namespace iv::core
#endif  // IV_SCOPRED_PTR_H_
