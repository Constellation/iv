#ifndef _IV_SCOPRED_PTR_H_
#define _IV_SCOPRED_PTR_H_
#include "noncopyable.h"
namespace iv {
namespace core {

template<class T>
class ScopedPtr : iv::core::Noncopyable<ScopedPtr<T> > {
 public:
  explicit ScopedPtr(T* ptr = NULL) : ptr_(ptr) { }

  ~ScopedPtr() {
    delete ptr_;
  }

  void reset(T* ptr = NULL) {
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
    ptr_ = NULL;
    return res;
  }

  void swap(ScopedPtr& rhs) {
    std::swap(ptr_, rhs.ptr_);
  }

 private:
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
class ScopedPtr<T[]> : iv::core::Noncopyable<ScopedPtr<T[]> > {
 public:
  explicit ScopedPtr(T* ptr = NULL) : ptr_(ptr) { }

  ~ScopedPtr() {
    delete [] ptr_;
  }

  void reset(T* ptr = NULL) {
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
    ptr_ = NULL;
    return res;
  }

  void swap(ScopedPtr& rhs) {
    std::swap(ptr_, rhs.ptr_);
  }

 private:
  T* ptr_;

  template<typename U> bool operator==(const ScopedPtr<U>& rhs) const;
  template<typename U> bool operator!=(const ScopedPtr<U>& rhs) const;
};

template <class T>
void swap(ScopedPtr<T[]>& lhs, ScopedPtr<T[]>& rhs) {
  lhs.swap(rhs);
}

template <class T>
bool operator==(T* lhs, const ScopedPtr<T[]>& rhs) {
  return lhs == rhs.get();
}

template <class T>
bool operator!=(T* lhs, const ScopedPtr<T[]>& rhs) {
  return lhs != rhs.get();
}

} }  // namespace iv::core
#endif  // _IV_SCOPRED_PTR_H_
