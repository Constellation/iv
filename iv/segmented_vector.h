#ifndef IV_LV5_SEGMENTED_VECTOR_H_
#define IV_LV5_SEGMENTED_VECTOR_H_
#include <cstddef>
#include <memory>
#include <vector>
#include <iterator>
#include <algorithm>
#include <iv/detail/array.h>
#include <iv/utils.h>
namespace iv {
namespace core {

template<typename T,
         std::size_t SegmentSize,
         typename Alloc = std::allocator<T> >
class SegmentedVector {
 public:

  static_assert(SegmentSize != 0, "The segment size must not be 0");

  typedef std::size_t size_type;
  typedef Alloc allocator_type;
  typedef T value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::ptrdiff_t difference_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  typedef SegmentedVector<T, SegmentSize, Alloc> this_type;

  static const size_type npos = static_cast<size_type>(-1);

  template<typename Seg>
  class iterator_base
    : public std::iterator<std::bidirectional_iterator_tag, value_type> {
   public:
    friend class SegmentedVector<T, SegmentSize, Alloc>;

    typedef iterator_base<Seg> this_type;
    typedef SegmentedVector<T, SegmentSize, Alloc> segmented_vector_type;
    typedef std::iterator<
        std::bidirectional_iterator_tag, value_type> super_type;
    typedef typename super_type::pointer pointer;
    typedef const typename super_type::pointer const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef typename super_type::difference_type difference_type;
    typedef iterator_base<segmented_vector_type*> iterator;
    typedef iterator_base<const segmented_vector_type*> const_iterator;

    iterator_base(Seg vector, size_type size)  // NOLINT
      : vector_(vector),
        index_(size % SegmentSize),
        segment_index_(size / SegmentSize) { }

    iterator_base(const iterator& it)  // NOLINT
      : vector_(it.vector()),
        index_(it.index()),
        segment_index_(it.segment_index()) { }

    reference operator*() {
      return (*vector()->segments_[segment_index()])[index()];
    }

    const_reference operator*() const {
      return (*vector()->segments_[segment_index()])[index()];
    }

    pointer operator->() {
      return &(*vector()->segments_[segment_index()])[index()];
    }

    const_pointer operator->() const {
      return &(*vector()->segments_[segment_index()])[index()];
    }

    this_type& operator++() {
      ++index_;
      if (index() >= vector()->segments_[segment_index()]->size()) {
        const size_type total_index = (index() + segment_index() * SegmentSize);
        if (total_index == vector()->size()) {
          // end
          return *this;
        }

        assert(segment_index() + 1 < vector()->segments_.size());
        ++segment_index_;
        index_ = 0;
      }
      return *this;
    }

    this_type& operator++(int) {  // NOLINT
      this_type iter(*this);
      (*this)++;
      return iter;
    }

    this_type& operator--() {
      assert(!(index() == 0 && segment_index() == 0));
      if (index() != 0) {
        --index_;
        return *this;
      }

      index_ = SegmentSize - 1;
      segment_index_ -= 1;
      return *this;
    }

    this_type& operator--(int) {  // NOLINT
      this_type iter(*this);
      (*this)--;
      return iter;
    }

    friend bool operator==(const this_type& lhs, const this_type& rhs) {
      return lhs.index() == rhs.index() &&
          lhs.segment_index() == rhs.segment_index() &&
          lhs.vector() == rhs.vector();
    }

    friend bool operator!=(const this_type& lhs, const this_type& rhs) {
      return !(lhs == rhs);
    }

    Seg vector() const { return vector_; }

    size_type index() const { return index_; }

    size_type segment_index() const { return segment_index_; }
   private:
    Seg vector_;
    size_type index_;
    size_type segment_index_;
  };

  typedef iterator_base<this_type*> iterator;
  typedef iterator_base<const this_type*> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  class Segment : protected std::array<value_type, SegmentSize> {
   public:
    friend class SegmentedVector<T, SegmentSize, Alloc>;
    typedef std::array<value_type, SegmentSize> array_type;

    using array_type::operator[];
    using array_type::data;

   private:
    size_type size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void push_back(allocator_type& alloc, const value_type& value) {
      assert(size() < SegmentSize);
      alloc.construct(data() + size(), value);
      ++size_;
    }

    void pop_back(allocator_type& alloc) {  // NOLINT
      assert(size() != 0);
      --size_;
      alloc.destroy(data() + size());
    }

    void clear(allocator_type& alloc) { // NOLINT
      for (pointer it = data(), last = data() + size(); it != last; ++it) {
        alloc.destroy(it);
      }
      size_ = 0;
    }

    size_type size_;
  };

  typedef
      typename Alloc::template rebind<Segment>::other segment_allocator_type;
  typedef std::vector<Segment*,
          typename Alloc::template rebind<Segment*>::other> Segments;

  SegmentedVector(const allocator_type& alloc = allocator_type())
    : size_(0),
      segments_(typename Segments::allocator_type(alloc)),
      alloc_(alloc),
      segment_alloc_(alloc) {
  }

  ~SegmentedVector() {
    for (typename Segments::iterator it = segments_.begin(),
         last = segments_.end(); it != last; ++it) {
      if (!(*it)->empty()) {
        (*it)->clear(alloc_);
      }
      segment_alloc_.deallocate(*it, 1);
    }
  }

  iterator begin() { return iterator(this, 0); }
  const_iterator begin() const { return const_iterator(this, 0); }
  const_iterator cbegin() const { return const_iterator(this, 0); }
  iterator end() { return iterator(this, size()); }
  const_iterator end() const { return const_iterator(this, size()); }
  const_iterator cend() const { return const_iterator(this, size()); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crend() const {
    return const_reverse_iterator(begin());
  }

  reference operator[](size_type n) {
    assert(size() > n);
    return (*SegmentFor(n))[Subscript(n)];
  }

  const_reference operator[](size_type n) const {
    assert(size() > n);
    return (*SegmentFor(n))[Subscript(n)];
  }

  size_type size() const { return size_; }

  bool empty() const { return size_ == 0; }

  void push_back(value_type c) {
    reserve(size() + 1);
    SegmentFor(size())->push_back(alloc_, c);
    ++size_;
  }

  void pop_back() {
    assert(!empty());
    SegmentFor(size() - 1)->pop_back(alloc_);
    --size_;
  }

  void reserve(size_type n) {
    if (n == 0) {
      return;
    }

    const size_type segment_index = (n - 1) / SegmentSize;
    if (segment_index < segments_.size()) {
      return;
    }

    const size_type num = segment_index + 1 - segments_.size();
    Segment* result = segment_alloc_.allocate(num);
    for (size_type i = 0; i < num; ++i) {
      Segment* target = result + i;
      target->size_ = 0;
      segments_.push_back(target);
    }
  }

  void resize(size_type n, const value_type& c = value_type()) {
    const size_type previous = size();
    reserve(n);
    if (previous < n) {
      for (difference_type diff = n - previous; diff--;) {
        push_back(c);
      }
    } else {
      for (difference_type diff = previous - n; diff--;) {
        pop_back();
      }
    }
  }

  void clear() {
    for (typename Segments::iterator it = segments_.begin(),
         last = segments_.end(); it != last; ++it) {
      (*it)->clear(alloc_);
    }
    size_ = 0;
  }

  void shrink_to_fit() {
    const size_type used_segments =
        empty() ? 0 : (((size() - 1) / SegmentSize) + 1);
    for (typename Segments::iterator it = segments_.begin() + used_segments,
         last = segments_.end(); it != last; ++it) {
      segment_alloc_.deallocate(*it, 1);
    }
    segments_.resize(used_segments);
    ShrinkToFit(segments_);
  }

  reference front() { return (*this)[0]; }
  const_reference front() const { return (*this)[0]; }
  reference back() { return (*this)[size() - 1]; }
  const_reference back() const { return (*this)[size() - 1]; }

  Segment* SegmentFor(size_type index) {
    return segments_[index / SegmentSize];
  }

  const Segment* SegmentFor(size_type index) const {
    return segments_[index / SegmentSize];
  }

  static size_type Subscript(size_type index) {
    return index % SegmentSize;
  }

 private:
  size_type size_;
  Segments segments_;
  allocator_type alloc_;
  segment_allocator_type segment_alloc_;
};

} }  // namespace iv::core
#endif  // IV_LV5_SEGMENTED_VECTOR_H_
