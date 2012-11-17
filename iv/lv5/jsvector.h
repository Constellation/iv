#ifndef IV_LV5_JSVECTOR_H_
#define IV_LV5_JSVECTOR_H_
#include <iv/lv5/jsarray.h>
namespace iv {
namespace lv5 {

class JSVector : private JSArray {
 public:
  // std::vector boilarplates begin
  //
  // +  constructor,
  // +  copy constructor,
  // +  operator=,
  // +  swap
  // +  get_allocator
  //     is not delegated
  //
  typedef JSValVector::value_type value_type;
  typedef JSValVector::iterator iterator;
  typedef JSValVector::const_iterator const_iterator;
  typedef JSValVector::reverse_iterator reverse_iterator;
  typedef JSValVector::const_reverse_iterator const_reverse_iterator;
  typedef JSValVector::pointer pointer;
  typedef JSValVector::const_pointer const_pointer;
  typedef JSValVector::reference reference;
  typedef JSValVector::const_reference const_reference;
  typedef JSValVector::difference_type difference_type;
  typedef JSValVector::size_type size_type;

  iterator begin() { return vector_.begin(); }
  const_iterator begin() const { return vector_.begin(); }
  const_iterator cbegin() const { return vector_.begin(); }

  iterator end() { return vector_.end(); }
  const_iterator end() const { return vector_.end(); }
  const_iterator cend() const { return vector_.end(); }

  reverse_iterator rbegin() { return vector_.rbegin(); }
  const_reverse_iterator rbegin() const { return vector_.rbegin(); }
  const_reverse_iterator crbegin() const { return vector_.rbegin(); }

  reverse_iterator rend() { return vector_.rend(); }
  const_reverse_iterator rend() const { return vector_.rend(); }
  const_reverse_iterator crend() const { return vector_.rend(); }

  size_type size() const { return vector_.size(); }

  size_type max_size() const { return vector_.max_size(); }

  void resize(size_type sz) { vector_.resize(sz); }
  void resize(size_type sz, const_reference c) { vector_.resize(sz, c); }


  size_type capacity() const { return vector_.capacity(); }

  bool empty() const { return vector_.empty(); }

  void reserve(size_type n) { vector_.reserve(n); }

  reference operator[](size_type n) { return vector_[n]; }
  const_reference operator[](size_type n) const { return vector_[n]; }
  //  reference at(size_type n) { return vector_.at(n); }
  //  const_reference at(size_type n) const { return vector_.at(n); }

  reference front() { return vector_.front(); }
  const_reference front() const { return vector_.front(); }
  reference back() { return vector_.back(); }
  const_reference back() const { return vector_.back(); }

  value_type* data() { return vector_.data(); }
  const value_type* data() const { return vector_.data(); }

  template<typename InputIterator>
  void assign(InputIterator first, InputIterator last) {
    vector_.assign(first, last);
  }
  void assign(size_type n, const JSVal& u) { vector_.assign(n, u); }

  void push_back(const JSVal& x) { vector_.push_back(x); }
  void pop_back() { vector_.pop_back(); }


  iterator insert(iterator position, const JSVal& x) {
    return vector_.insert(position, x);
  }
  void insert(iterator position, size_type n, const JSVal& x) {
    vector_.insert(position, n, x);
  }
  template<typename InputIterator>
  void insert(iterator position, InputIterator first, InputIterator last) {
    vector_.insert(position, first, last);
  }

  iterator erase(iterator position) { return vector_.erase(position); }
  iterator erase(iterator first, iterator last) {
    return vector_.erase(first, last);
  }

  void clear() { vector_.clear(); }
  // std::vector boilarplates end


  static JSVector* New(Context* ctx) {
    JSVector* vec = new JSVector(ctx);
    vec->set_cls(JSArray::GetClass());
    vec->set_prototype(ctx->global_data()->array_prototype());
    return vec;
  }

  static JSVector* New(Context* ctx, size_type n,
                       const JSVal& v = JSUndefined) {
    JSVector* vec = new JSVector(ctx, n, v);
    vec->set_cls(JSArray::GetClass());
    vec->set_prototype(ctx->global_data()->array_prototype());
    return vec;
  }

  // Once gain JSArray, JSVector is disabled, do not touch.
  JSArray* ToJSArray() {
    const size_type total = size();
    if (total > JSArray::kMaxVectorSize) {
      // alloc map
      ReserveMap(size());
      uint32_t i = JSArray::kMaxVectorSize;
      for (const_iterator it = cbegin() + i,
           last = cend(); it != last; ++it, ++i) {
        (*map_)[i] = *it;
      }
      vector_.resize(JSArray::kMaxVectorSize);
    }
    length_.set_value(total);
    return static_cast<JSArray*>(this);
  }

 private:
  explicit JSVector(Context* ctx)
    : JSArray(ctx, 0u) {
  }

  JSVector(Context* ctx, size_type n, const JSVal& v)
    : JSArray(ctx, 0u) {
    vector_.resize(n, v);
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_JSVECTOR_H_
