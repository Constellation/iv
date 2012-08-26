#ifndef IV_INTRUSIVE_LIST_H_
#define IV_INTRUSIVE_LIST_H_
#include <cassert>
#include <iterator>
#include <iv/detail/type_traits.h>
#include <iv/static_assert.h>
#include <iv/noncopyable.h>
namespace iv {
namespace core {
namespace intrusive_list_detail {

template<typename T, bool IsConst>
struct Prefix {
  typedef T type;
};

template<typename T>
struct Prefix<T, true> {
  typedef const T type;
};

}  // namespace intrusive_list_detail

template<typename T, bool IsConst>
class IntrusiveListIterator;

template<typename T>
class IntrusiveList;

class IntrusiveListBase {
 public:
  typedef IntrusiveListBase this_type;
  template<typename U, bool IsConst> friend class IntrusiveListIterator;
  template<typename U> friend class IntrusiveList;

  IntrusiveListBase() : next_(NULL), prev_(NULL) { }
  IntrusiveListBase(this_type* n, this_type* p) : next_(n), prev_(p) { }

  void Unlink() {
    assert(IsLinked());
    next_->prev_ = prev_;
    prev_->next_ = next_;
    next_ = prev_ = NULL;
  }

  bool IsLinked() const { return !(next_ == NULL && prev_ == NULL); }

 private:
  this_type* pointer() { return this; }
  const this_type* pointer() const { return this; }

  this_type* next_;
  this_type* prev_;
};

template<typename T, bool IsConst>
class IntrusiveListIterator
  : public std::iterator<std::bidirectional_iterator_tag, T> {
 public:
  friend class IntrusiveList<T>;
  template<typename U, bool IsConst2> friend class IntrusiveListIterator;

  typedef IntrusiveListIterator<T, IsConst> this_type;
  typedef std::iterator<std::bidirectional_iterator_tag, T> super_type;
  typedef IntrusiveListBase node_type;
  typedef IntrusiveListBase* cursor_type;

  typedef T value_type;
  typedef typename intrusive_list_detail::Prefix<value_type*, IsConst>::type pointer;  // NOLINT
  typedef typename std::add_pointer<const typename std::remove_pointer<pointer>::type>::type const_pointer;  // NOLINT
  typedef typename intrusive_list_detail::Prefix<value_type&, IsConst>::type reference;  // NOLINT
  typedef typename std::add_reference<const typename std::remove_reference<reference>::type>::type const_reference;  // NOLINT
  typedef typename super_type::difference_type difference_type;

  typedef IntrusiveListIterator<T, false> iterator;
  typedef IntrusiveListIterator<T, true> const_iterator;

  IntrusiveListIterator() : current_(NULL) { }

  explicit IntrusiveListIterator(cursor_type node) : current_(node) { }

  explicit IntrusiveListIterator(const node_type* node)
    : current_(const_cast<cursor_type>(node)) { }

  IntrusiveListIterator(const iterator& it)  // NOLINT
    : current_(it.current_) { }

  reference operator *() {
    return *static_cast<pointer>(current_);
  }
  const_reference operator *() const {
    return *static_cast<const_pointer>(current_);
  }

  pointer operator ->() {
    return static_cast<pointer>(current_);
  }
  const_pointer operator ->() const {
    return static_cast<const_pointer>(current_);
  }

  this_type& operator++() {
    current_ = current_->next_;
    return *this;
  }

  this_type operator++(int dummy) {
    const this_type result(*this);
    ++(*this);
    return result;
  }

  this_type& operator--() {
    current_ = current_->prev_;
    return *this;
  }

  this_type operator--(int dummy) {
    const this_type result(*this);
    --(*this);
    return result;
  }

  friend bool operator==(const this_type& lhs, const this_type& rhs) {
    return lhs.current_ == rhs.current_;
  }

  friend bool operator!=(const this_type& lhs, const this_type& rhs) {
    return lhs.current_ != rhs.current_;
  }

 private:
  cursor_type current() const { return current_; }

  cursor_type current_;
};

template<typename T>
class IntrusiveList
  : protected IntrusiveListBase, private Noncopyable<IntrusiveList<T> > {
 public:
  typedef IntrusiveListBase node_type;
  typedef IntrusiveListIterator<T, false> iterator;
  typedef IntrusiveListIterator<T, true> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  IntrusiveList() : IntrusiveListBase(this, this), size_(0) { }

  iterator begin() { return iterator(this->next_); }
  const_iterator begin() const { return const_iterator(this->next_); }
  const_iterator cbegin() const { return begin(); }

  iterator end() { return iterator(this); }
  const_iterator end() const { return const_iterator(this); }
  const_iterator cend() const { return end(); }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const { return rbegin(); }


  reverse_iterator rend() {
    return reverse_iterator(begin());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const { return rend(); }

  void push_back(reference v) { insert(end(), v); }

  void push_front(reference v) { insert(begin(), v); }

  void pop_back() {
    back().Unlink();
    --size_;
  }

  void pop_front() {
    front().Unlink();
    --size_;
  }

  iterator insert(const_iterator cur, reference v) {
    InsertBefore(cur->pointer(), v.pointer());
    ++size_;
    return iterator(v.pointer());
  }

  template<typename Iterator>
  void insert(const_iterator cur, Iterator it, Iterator last) {
    for (; it != last; ++it) {
      cur = insert(cur, *it);
    }
  }

  iterator erase(const_iterator it) {
    iterator res(it.current());
    ++res;
    it.current()->Unlink();
    --size_;
    return res;
  }

  iterator erase(const_iterator it, const_iterator last) {
    while (it != last) {
      it = erase(it);
    }
    return iterator(last.current());
  }

  reference front() { return *begin(); }
  const_reference front() const { return *begin(); }

  reference back() { return *(--end()); }
  const_reference back() const { return *(--end()); }

  void clear() {
    erase(begin(), end());
  }

  size_type size() const { return size_; }

  bool empty() const { return size_ == 0; }

 private:
  void InsertBefore(node_type* next, node_type* target) {
    node_type* prev = next->prev_;
    prev->next_ = target;
    target->prev_ = prev;
    target->next_ = next;
    next->prev_ = target;
  }

  size_type size_;
};

} }  // namespace iv::core
#endif  // IV_INTRUSIVE_LIST_H_
