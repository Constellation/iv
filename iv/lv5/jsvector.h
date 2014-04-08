#ifndef IV_LV5_JSVECTOR_H_
#define IV_LV5_JSVECTOR_H_
#include <initializer_list>
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
  typedef DenseArrayVector::value_type value_type;
  typedef DenseArrayVector::iterator iterator;
  typedef DenseArrayVector::const_iterator const_iterator;
  typedef DenseArrayVector::reverse_iterator reverse_iterator;
  typedef DenseArrayVector::const_reverse_iterator const_reverse_iterator;
  typedef DenseArrayVector::pointer pointer;
  typedef DenseArrayVector::const_pointer const_pointer;
  typedef DenseArrayVector::reference reference;
  typedef DenseArrayVector::const_reference const_reference;
  typedef DenseArrayVector::difference_type difference_type;
  typedef DenseArrayVector::size_type size_type;

  iterator begin() { return elements_.vector.begin(); }
  const_iterator begin() const { return elements_.vector.begin(); }
  const_iterator cbegin() const { return elements_.vector.begin(); }

  iterator end() { return elements_.vector.end(); }
  const_iterator end() const { return elements_.vector.end(); }
  const_iterator cend() const { return elements_.vector.end(); }

  reverse_iterator rbegin() { return elements_.vector.rbegin(); }
  const_reverse_iterator rbegin() const { return elements_.vector.rbegin(); }
  const_reverse_iterator crbegin() const { return elements_.vector.rbegin(); }

  reverse_iterator rend() { return elements_.vector.rend(); }
  const_reverse_iterator rend() const { return elements_.vector.rend(); }
  const_reverse_iterator crend() const { return elements_.vector.rend(); }

  size_type size() const { return elements_.vector.size(); }

  size_type max_size() const { return elements_.vector.max_size(); }

  void resize(size_type sz) { elements_.vector.resize(sz); }
  void resize(size_type sz, const_reference c) {
    elements_.vector.resize(sz, c);
  }


  size_type capacity() const { return elements_.vector.capacity(); }

  bool empty() const { return elements_.vector.empty(); }

  void reserve(size_type n) { elements_.vector.reserve(n); }

  reference operator[](size_type n) { return elements_.vector[n]; }
  const_reference operator[](size_type n) const { return elements_.vector[n]; }
  //  reference at(size_type n) { return elements_.vector.at(n); }
  //  const_reference at(size_type n) const { return elements_.vector.at(n); }

  reference front() { return elements_.vector.front(); }
  const_reference front() const { return elements_.vector.front(); }
  reference back() { return elements_.vector.back(); }
  const_reference back() const { return elements_.vector.back(); }

  value_type* data() { return elements_.vector.data(); }
  const value_type* data() const { return elements_.vector.data(); }

  template<typename InputIterator>
  void assign(InputIterator first, InputIterator last) {
    elements_.vector.assign(first, last);
  }
  void assign(size_type n, const JSVal& u) { elements_.vector.assign(n, u); }

  void push_back(const JSVal& x) { elements_.vector.push_back(x); }
  void pop_back() { elements_.vector.pop_back(); }


  iterator insert(iterator position, const JSVal& x) {
    return elements_.vector.insert(position, x);
  }
  void insert(iterator position, size_type n, const JSVal& x) {
    elements_.vector.insert(position, n, x);
  }
  template<typename InputIterator>
  void insert(iterator position, InputIterator first, InputIterator last) {
    elements_.vector.insert(position, first, last);
  }

  iterator erase(iterator position) { return elements_.vector.erase(position); }
  iterator erase(iterator first, iterator last) {
    return elements_.vector.erase(first, last);
  }

  void clear() { elements_.vector.clear(); }
  // std::vector boilarplates end


  static JSVector* New(Context* ctx) {
    JSVector* vec = new JSVector(ctx);
    vec->set_cls(JSArray::GetClass());
    return vec;
  }

  static JSVector* New(Context* ctx, size_type n,
                       const JSVal& v = JSUndefined) {
    JSVector* vec = new JSVector(ctx, n, v);
    vec->set_cls(JSArray::GetClass());
    return vec;
  }

  static JSVector* New(Context* ctx, std::initializer_list<JSVal> list) {
    JSVector* vec = New(ctx);
    vec->assign(list.begin(), list.end());
    return vec;
  }

  template<typename Iter>
  static JSVector* New(Context* ctx, Iter it, Iter last) {
    JSVector* vec = New(ctx);
    vec->assign(it, last);
    return vec;
  }

  // Once gain JSArray, JSVector is disabled, do not touch.
  JSArray* ToJSArray() {
    const size_type total = size();
    if (total > IndexedElements::kMaxVectorSize) {
      // alloc map
      SparseArrayMap* sparse = elements_.EnsureMap();
      uint32_t i = IndexedElements::kMaxVectorSize;
      for (const_iterator it = cbegin() + i,
           last = cend(); it != last; ++it, ++i) {
        sparse->insert(
            std::make_pair(i, StoredSlot(*it, ATTR::Object::Data())));
      }
      elements_.vector.resize(IndexedElements::kMaxVectorSize);
    }
    elements_.set_length(total);
    return static_cast<JSArray*>(this);
  }

 private:
  explicit JSVector(Context* ctx)
    : JSArray(ctx, 0u) {
  }

  explicit JSVector(Context* ctx, uint32_t size, JSVal value)
    : JSArray(ctx, 0u) {
      elements_.vector.resize(size, value);
  }
};

static_assert(sizeof(JSArray) == sizeof(JSVector),
              "JSVector is interface for newly created JSArray");

} }  // namespace iv::lv5
#endif  // IV_LV5_JSVECTOR_H_
