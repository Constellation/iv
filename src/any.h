#ifndef _IV_ANY_H_
#define _IV_ANY_H_
#include <algorithm>
namespace iv {
namespace core {

class Any {
 public:
  Any() : ptr_(NULL) { }

  // This is Wrapper Class,
  // so we cannot set constructor as explicit.
  template<typename T>
  Any(const T & val) : ptr_(new Holder<T>(val)) { }  // NOLINT

  template<typename T>
  Any(T* val) : ptr_(new Holder<T*>(val)) { }  // NOLINT

  Any(const Any & other)
    : ptr_(other.ptr_ ? other.ptr_->Clone() : NULL) { }

  ~Any() {
    delete ptr_;
  }

  template<typename T>
  T* As() {
    return &(static_cast<Holder<T> *>(ptr_)->val_);
  }

  void Swap(Any& rhs) throw() {
    using std::swap;
    swap(ptr_, rhs.ptr_);
  }

  friend void swap(Any& rhs, Any& lhs) {
    return rhs.Swap(lhs);
  }

  template<typename T>
  Any& operator=(const T & rhs) {
    Any(rhs).Swap(*this);  // NOLINT
    return *this;
  }

  Any& operator=(Any & rhs) {
    rhs.Swap(*this);
    return *this;
  }

 private:
  class Placeholder {
   public:
    virtual ~Placeholder() { }
    virtual Placeholder* Clone() const = 0;
  };

  template<typename T>
  class Holder : public Placeholder {
   public:
    explicit Holder(const T& val) : val_(val) { }
    Placeholder* Clone() const {
      return new Holder<T>(val_);
    }
    T val_;
  };
  Placeholder* ptr_;
};

} }  // namespace iv::core
#endif  // _IV_ANY_H_
