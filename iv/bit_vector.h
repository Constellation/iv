#ifndef IV_BIT_VECTOR_H_
#define IV_BIT_VECTOR_H_
#include <iv/detail/cstdint.h>
#include <algorithm>
namespace iv {
namespace core {

template<typename Alloc = std::allocator<std::size_t> >
class BitVector {
 public:
  typedef Alloc allocator_type;
  typedef std::size_t size_type;
  typedef typename allocator_type::value_type value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  typedef value_type word_type;
  typedef BitVector<Alloc> this_type;

  static const size_type kWordSize = sizeof(value_type) * 8;

 private:
  struct UnaryBitNot {
    value_type operator()(value_type val) {
      return ~val;
    }
  };

  struct UnaryTrue {
    bool operator()(value_type val) {
      return val;
    }
  };

  static value_type max_value() {
    return std::numeric_limits<value_type>::max();
  }

 public:
  explicit BitVector(const Alloc& alloc = Alloc())
    : data_(nullptr),
      size_(0),
      capacity_(0),
      alloc_(alloc) {
  }

  explicit BitVector(size_type size,
                     bool val = false, const Alloc& alloc = Alloc())
    : data_(nullptr),
      size_(0),
      capacity_(0),
      alloc_(alloc) {
    resize(size, val);
  }

  BitVector(const this_type& rhs)  // NOLINT
    : data_(nullptr),
      size_(rhs.size()),
      capacity_(Capacity(size())),
      alloc_(rhs.get_allocator()) {
    assert(capacity() <= rhs.capacity());
    data_ = alloc_.allocate(capacity());
    std::copy(rhs.data(), rhs.data() + capacity(), data());
  }

  this_type& operator=(const this_type& rhs) {
    resize(rhs.size());
    std::copy(rhs.data(), rhs.data() + num_words(), data());
    return *this;
  }

  friend void swap(this_type& lhs, this_type& rhs) {
    using std::swap;
    swap(lhs.data_, rhs.data_);
    swap(lhs.size_, rhs.size_);
    swap(lhs.capacity_, rhs.capacity_);
    swap(lhs.alloc_, rhs.alloc_);
  }

  void swap(this_type& rhs) {
    using std::swap;
    swap(*this, rhs);
  }

  ~BitVector() {
    if (data()) {
      alloc_.deallocate(data(), capacity());
    }
  }

  bool operator[](size_type pos) const {
    return data_[pos / kWordSize] &
        (static_cast<value_type>(1) << (pos % kWordSize));
  }

  this_type& set() {
    std::fill(data_, data_ + num_words(), max_value());
    return *this;
  }

  this_type& set(size_type pos, bool val = true) {
    if (val) {
      data_[pos / kWordSize] |=
          (static_cast<value_type>(1) << (pos % kWordSize));
    } else {
      reset(pos);
    }
    return *this;
  }

  this_type& reset() {
    std::fill(data_, data_ + num_words(), 0);
    return *this;
  }

  this_type& reset(size_type pos) {
    data_[pos / kWordSize] &=
        ~(static_cast<value_type>(1) << (pos % kWordSize));
    return *this;
  }

  void clear() {
    size_ = 0;
  }

  this_type& flip() {
    std::transform(data(), data() + num_words(), data(), UnaryBitNot());
    return *this;
  }

  this_type& flip(size_type pos) {
    if (test(pos)) {
      return reset(pos);
    } else {
      return set(pos);
    }
  }

  bool test(size_type pos) const {
    return (*this)[pos];
  }

  void resize(size_type n, bool val = false) {
    if (n <= size()) {
      size_ = n;
      return;
    }

    reserve(n);
    const size_type old_num_words = num_words();
    if (old_num_words) {
      const size_type d = size() % kWordSize;
      const word_type mask = max_value() << d;
      if (val) {
        data()[old_num_words - 1] |= mask;
      } else {
        data()[old_num_words - 1] &= ~mask;
      }
    }
    const size_type d = Capacity(n) - old_num_words;
    size_ = n;
    if (d) {
      const size_type full = d - 1;
      if (full) {
        if (val) {
          std::fill(data() + old_num_words,
                    data() + old_num_words + full, max_value());
        } else {
          std::fill(data() + old_num_words,
                    data() + old_num_words + full, 0);
        }
      }
      // the last one
      if (val) {
        data()[old_num_words + full] = max_value();
      } else {
        data()[old_num_words + full] = 0;
      }
    }
  }

  void reserve(size_type n) {
    const size_type cap = Capacity(n);
    if (cap > capacity()) {
      pointer new_data = alloc_.allocate(cap);
      if (data()) {
        std::copy(data(), data() + Capacity(size()), new_data);
        alloc_.deallocate(data(), capacity());
      }
      data_ = new_data;
      capacity_ = cap;
    }
  }

  this_type& operator&=(const this_type& rhs) {
    assert(size() == rhs.size());
    for (size_type i = 0, len = num_words(); i < len; ++i) {
      data()[i] &= rhs.data()[i];
    }
  }

  this_type& operator|=(const this_type& rhs) {
    assert(size() == rhs.size());
    for (size_type i = 0, len = num_words(); i < len; ++i) {
      data()[i] |= rhs.data()[i];
    }
  }

  this_type& operator^=(const this_type& rhs) {
    assert(size() == rhs.size());
    for (size_type i = 0, len = num_words(); i < len; ++i) {
      data()[i] ^= rhs.data()[i];
    }
  }

  this_type& operator-=(const this_type& rhs) {
    assert(size() == rhs.size());
    for (size_type i = 0, len = num_words(); i < len; ++i) {
      data()[i] &= (~rhs.data()[i]);
    }
  }

  this_type operator~() const {
    return this_type(*this).flip();
  }

  friend bool operator==(const this_type& lhs, const this_type& rhs) {
    if (lhs.size() != rhs.size()) {
      return false;
    }

    const size_type n = lhs.num_words();
    const size_type d = lhs.size() % kWordSize;

    if (d == 0) {
      return std::equal(lhs.data(), lhs.data() + n, rhs.data());
    }

    // when n == 0, d == 0
    assert(n != 0);

    if (n > 1) {
      for (size_type i = 0, len = n - 1; i < len; ++i) {
        if (lhs.data()[i] != rhs.data()[i]) {
          return false;
        }
      }
    }
    // the last one
    const word_type mask = max_value() >> (kWordSize - d);
    return !((lhs.data()[n - 1] ^ rhs.data()[n - 1]) & mask);
  }

  friend bool operator!=(const this_type& lhs, const this_type& rhs) {
    return !(lhs == rhs);
  }

  bool any() const {
    const size_type n = num_words();
    const size_type d = size() % kWordSize;
    if (d == 0) {
      return std::find_if(data(), data() + n, UnaryTrue()) != (data() + n);
    }
    if (n > 1) {
      for (size_type i = 0, len = n - 1; i < len; ++i) {
        if (data()[i]) {
          return true;
        }
      }
    }
    // the last one
    const word_type mask = max_value() >> (kWordSize - d);
    return data()[n - 1] & mask;
  }

  bool none() const {
    return !any();
  }

  pointer data() { return data_; }
  const_pointer data() const { return data_; }
  size_type size() const { return size_; }
  bool empty() const { return size() == 0; }
  size_type num_words() const { return Capacity(size()); }
  size_type capacity() const { return capacity_; }
  allocator_type get_allocator() const { return alloc_; }
 private:
  static size_type Capacity(size_type size) {
    return (size == 0) ? 0 : 1 + ((size - 1) / kWordSize);
  }

  pointer data_;
  size_type size_;
  size_type capacity_;
  allocator_type alloc_;
};

} }  // namespace iv::core
#endif  // IV_BIT_VECTOR_H_
