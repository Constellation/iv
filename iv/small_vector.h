#ifndef IV_SMALL_VECTOR_H_
#define IV_SMALL_VECTOR_H_
#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
namespace iv {
namespace core {

template<typename T, std::size_t N, typename Allocator = std::allocator<T>>
class small_vector {
 public:
  typedef T& reference;
  typedef const T& const_reference;
  typedef T* iterator;
  typedef const T* const_iterator;
  typedef ::std::size_t size_type;
  typedef ::std::ptrdiff_t difference_type;
  typedef T value_type;
  typedef Allocator allocator_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef small_vector<T, N, Allocator> this_type;

  static_assert(N != 0, "N should be larger than 0");

  small_vector(const allocator_type& allocator = allocator_type())
      : allocator_(allocator)
      , static_storage_()
      , data_(static_storage_data())
      , size_(0)
      , capacity_(static_storage_.size()) {
    assert(capacity() != 0);
  }

  small_vector(const this_type& x)
      : allocator_(x.allocator_)
      , static_storage_()
      , data_(static_storage_data())
      , size_(0)
      , capacity_(static_storage_.size()) {
    assert(capacity() != 0);
    std::copy(x.begin(), x.end(), std::back_inserter(*this));
  }

  this_type& operator=(const this_type& x) {
    this_type copy(x);
    copy.swap(*this);
    return *this;
  }

  ~small_vector() {
    finalize();
  }

  allocator_type get_allocator() const {
    return allocator_;
  }

  inline reference operator[](size_type n) {
    return data()[n];
  }

  inline const_reference operator[](size_type n) const {
    return data()[n];
  }

  inline pointer data() {
    return data_;
  }

  inline const_pointer data() const {
    return data_;
  }

  inline iterator begin() {
    return data();
  }

  inline const_iterator begin() const {
    return data();
  }

  inline const_iterator cbegin() const {
    return data();
  }

  inline iterator end() {
    return data() + size();
  }

  inline const_iterator end() const {
    return data() + size();
  }

  inline const_iterator cend() const {
    return data() + size();
  }

  inline reverse_iterator rbegin() {
    return reverse_iterator(end());
  }

  inline const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  inline const_reverse_iterator crbegin() const {
    return const_reverse_iterator(cend());
  }

  inline reverse_iterator rend() {
    return reverse_iterator(begin());
  }

  inline const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  inline const_reverse_iterator crend() const {
    return const_reverse_iterator(cbegin());
  }

  size_type max_size() const {
    return std::numeric_limits<std::size_t>::max();
  }

  inline constexpr size_type static_size() const {
    return N;
  }

  inline size_type size() const {
    return size_;
  }

  inline bool empty() const {
    return size() == 0;
  }

  inline size_type capacity() const {
    return capacity_;
  }

  inline void reserve(size_type n) {
    const size_type old = capacity();
    if (n > old) {
      assert(old != 0);

      size_type new_capacity = (std::max<size_type>)(old * 2, n);
      pointer new_data = allocator_.allocate(new_capacity);
      for (pointer new_i = new_data, old_i = data(), old_iz = data() + size();
           old_i != old_iz;
           ++new_i, ++old_i) {
          allocator_.construct(new_i, std::move(*old_i));
          allocator_.destroy(old_i);
      }

      if (static_storage_data() != data()) {
        allocator_.deallocate(data(), capacity());
      }

      // Replace with new data.
      data_ = new_data;
      capacity_ = new_capacity;
    }
    assert(n <= capacity());
  }

  inline void push_back(const_reference x) {
    reserve(size() + 1);
    allocator_.construct(data() + size(), x);
    ++size_;
  }

  inline void push_back(value_type&& x) {
    reserve(size() + 1);
    allocator_.construct(data() + size(), std::move(x));
    ++size_;
  }

  template<class... Args>
  inline void emplace_back(Args&&... args) {
    reserve(size() + 1);
    allocator_.construct(data() + size(), std::forward<Args>(args)...);
    ++size_;
  }

  inline void pop_back() {
    --size_;
    allocator_.destroy(data() + size());
  }

  void swap(this_type& rhs) {
    using std::swap;
    swap(allocator_, rhs.allocator_);
    swap(static_storage_, rhs.static_storage_);
    swap(data_, rhs.data_);
    swap(size_, rhs.size_);
    swap(capacity_, rhs.capacity_);
  }

 private:
  inline pointer static_storage_data() {
    return static_cast<pointer>(
        static_cast<void*>(static_storage_.data()));
  }

  inline const_pointer static_storage_data() const {
    return static_cast<const_pointer>(
        static_cast<const void*>(static_storage_.data()));
  }

  inline bool is_static_storage_used() const {
    return static_storage_data() == data();
  }

  void finalize() {
    destroy(begin(), end());
    if (!is_static_storage_used()) {
      allocator_.deallocate(data(), capacity());
    }
  }

  template<typename Iter>
  void destroy(Iter i, Iter iz) {
    for (; i != iz; ++i) {
      allocator_.destroy(&*i);
    }
  }

  allocator_type allocator_;
  typedef typename std::aligned_storage<
      sizeof(T), std::alignment_of<T>::value>::type raw_buffer_element;
  std::array<raw_buffer_element, N> static_storage_;
  pointer data_;
  size_type size_;
  size_type capacity_;
};

} }  // namespace iv::core
#endif  // IV_SMALL_VECTOR_H_
