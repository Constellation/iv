#ifndef IV_ASSOC_VECTOR_H_
#define IV_ASSOC_VECTOR_H_
#include <vector>
#include <utility>
namespace iv {
namespace core {
namespace detail {

template<typename KeyCompare, typename ValueType>
struct AssocVectorValueCompare
  : public std::binary_function<ValueType, ValueType, bool> {
  KeyCompare comp_;
  AssocVectorValueCompare(KeyCompare c) : comp_(c) { }
  bool operator()(const ValueType& lhs, const ValueType& rhs) const {
    return comp_(lhs.first, rhs.first);
  }
};

}  // namespace detail

template<
  typename Key,
  typename Val,
  typename Compare=std::less<Key>,
  typename Alloc=std::allocator<std::pair<Key, Val> > >
class AssocVector
  : protected std::vector<std::pair<Key, Val>, Alloc> {
 public:
  typedef Key key_type;
  typedef Val mapped_type;
  typedef std::pair<key_type, mapped_type> value_type;
  typedef Compare key_compare;
  typedef detail::AssocVectorValueCompare<key_compare, value_type> value_compare;
  typedef std::vector<value_type, Alloc> container_type;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::const_iterator const_iterator;
  typedef typename container_type::reverse_iterator reverse_iterator;
  typedef typename container_type::const_reverse_iterator const_reverse_iterator;
  typedef typename container_type::pointer pointer;
  typedef typename container_type::const_pointer const_pointer;
  typedef typename container_type::reference reference;
  typedef typename container_type::const_reference const_reference;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::difference_type difference_type;
  typedef typename container_type::allocator_type allocator_type;
  typedef const container_type const_container_type;

  using container_type::begin;
  using container_type::end;
  using container_type::empty;
  using container_type::size;
  using container_type::clear;
  using container_type::get_allocator;
  key_compare key_comp() const { return key_compare(); }
  value_compare value_comp() const { return value_compare(key_compare()); }

  using container_type::erase;
  size_type erase(const key_type& key) {
    const iterator it = find(key);
    if (it == container_type::end()) {
      return 0;
    }
    container_type::erase(it);
    return 1;
  }

  iterator find(const Key& key) {
    const iterator it = lower_bound(key);
    if (it != container_type::end() && it->first == key) {
      return it;
    }
    return container_type::end();
  }

  const_iterator find(const Key& key) const {
    const const_iterator it = lower_bound(key);
    if (it != container_type::end() && it->first == key) {
      return it;
    }
    return container_type::end();
  }

  iterator lower_bound(const key_type& key) {
    return std::lower_bound(container_type::begin(),
                            container_type::end(),
                            std::make_pair(key, mapped_type()), value_comp());
  }

  const_iterator lower_bound(const key_type& key) const {
    return std::lower_bound(container_type::begin(),
                            container_type::end(),
                            std::make_pair(key, mapped_type()), value_comp());
  }

  iterator upper_bound(const key_type& key) {
    return std::upper_bound(container_type::begin(),
                            container_type::end(),
                            std::make_pair(key, mapped_type()), value_comp());
  }

  const_iterator upper_bound(const key_type& key) const {
    return std::upper_bound(container_type::begin(),
                            container_type::end(),
                            std::make_pair(key, mapped_type()), value_comp());
  }

  iterator equal_range(const key_type& key) {
    return std::equal_range(container_type::begin(),
                            container_type::end(),
                            std::make_pair(key, mapped_type()), value_comp());
  }

  const_iterator equal_range(const key_type& key) const {
    return std::equal_range(container_type::begin(),
                            container_type::end(),
                            std::make_pair(key, mapped_type()), value_comp());
  }

  iterator insert(const value_type& val) {
    const iterator it = lower_bound(val.first);
    if (it != end() && it->first == val.first) {
      return it;
    }
    return container_type::insert(it, val);
  }

  template<typename Iter>
  void insert(Iter it, Iter last) {
    for (; it != last; ++it) {
      insert(*it);
    }
  }

  size_type count(const key_type& key) {
    return find(key) != container_type::end();
  }

  mapped_type& operator[](const key_type& key) {
    return insert(std::make_pair(key, mapped_type()))->second;
  }
};

} }  // namespace iv::core
#endif  // IV_ASSOC_VECTOR_H_
