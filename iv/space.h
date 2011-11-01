#ifndef IV_SPACE_H_
#define IV_SPACE_H_
#include <cassert>
#include <cstddef>
#include <new>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <string>
#include <limits>
#include <functional>
#include <iv/detail/unordered_map.h>
#include <iv/detail/functional.h>
#include <iv/detail/cstdint.h>
#include <iv/utils.h>
#include <iv/alloc.h>
#include <iv/conversions.h>

namespace iv {
namespace core {

class SpaceObject {
 public:
  template<typename Factory>
  void* operator new(std::size_t size, Factory* factory) {
    return factory->New(size);
  }
  void operator delete(void*, std::size_t) {
    UNREACHABLE();
  }
};

template<class Factory, class T>
class SpaceAllocator {
 public:
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;
  typedef SpaceAllocator<Factory, T> this_type;
  template<class U>
  struct rebind {
    typedef SpaceAllocator<Factory, U> other;
  };

  SpaceAllocator() : space_(NULL) { }
  explicit SpaceAllocator(Factory* factory) throw() : space_(factory) { }

  template<class U>
  SpaceAllocator(const SpaceAllocator<Factory, U>& alloc) throw()  // NOLINT
    : space_(alloc.space()) {
  }

  inline pointer address(reference x) const {
    return &x;
  }

  inline const_pointer address(const_reference x) const {
    return &x;
  }

  inline pointer allocate(size_type n, const void* = 0) {
    assert(space_);
    return reinterpret_cast<pointer>(space_->New(n * sizeof(T)));
  }

  inline void deallocate(pointer, size_type) { }

  inline size_type max_size() const {
    return std::numeric_limits<size_type>::max() / sizeof(T);
  }

  inline void construct(pointer p, const T& val) {
    new(reinterpret_cast<void*>(p)) T(val);
  }

  inline void destroy(pointer p) {
    (p)->~T();
  }

  inline char* _Charalloc(size_type n) {
    return allocate(n);
  }

  template<typename Other>
  inline this_type& operator=(const SpaceAllocator<Factory, Other>& rhs) {
    if (this != &rhs) {
      this_type(rhs).Swap(*this);
    }
    return *this;
  }

  inline Factory* space() const {
    return space_;
  }

 private:
  void Swap(this_type& rhs) {
    using std::swap;
    swap(space_, rhs.space_);
  }
  void operator=(const SpaceAllocator&);

  Factory* space_;
};

template <typename Factory, typename T>
bool operator==(const SpaceAllocator<Factory, T>& lhs,
                const SpaceAllocator<Factory, T>& rhs) {
  return true;
}

template <typename Factory, typename T>
bool operator!=(const SpaceAllocator<Factory, T>& lhs,
                const SpaceAllocator<Factory, T>& rhs) {
  return false;
}

template<typename Factory, typename T>
struct SpaceVector {
  typedef std::vector<T, SpaceAllocator<Factory, T> > type;
};

template<typename Factory, typename T1, typename T2>
struct SpaceMap {
  typedef std::map<T1,
                   T2,
                   std::less<T1>,
                   SpaceAllocator<Factory, std::pair<const T1, T2> > > type;
};

template<typename Factory, typename T1, typename T2>
struct SpaceHashMap {
  typedef std::unordered_map<T1,
                             T2,
                             std::hash<T1>,
                             std::equal_to<T1>,
                             SpaceAllocator<
                             Factory,
                             std::pair<const T1, T2> > > type;
};

template<typename Factory, typename T>
struct SpaceList {
  typedef std::list<T, SpaceAllocator<Factory, T> > type;
};

template<typename Factory>
struct SpaceUString {
  typedef std::basic_string<uint16_t,
                            std::char_traits<uint16_t>,
                            SpaceAllocator<Factory, uint16_t> > type;
};

} }  // namespace iv::core

namespace IV_HASH_NAMESPACE_START {

// template specialization for SpaceUString in std::unordered_map
// allowed in section 17.4.3.1
template<typename Factory>
struct hash<std::basic_string<uint16_t,
                              std::char_traits<uint16_t>,
                              iv::core::SpaceAllocator<Factory, uint16_t> > >
  : public std::unary_function<
    std::basic_string<
      uint16_t,
      std::char_traits<uint16_t>,
      iv::core::SpaceAllocator<Factory, uint16_t> >, std::size_t> {
  typedef std::unary_function<
    std::basic_string<
      uint16_t,
      std::char_traits<uint16_t>,
      iv::core::SpaceAllocator<Factory, uint16_t> >, std::size_t> super_type;
  typedef typename super_type::argument_type argument_type;
  typedef typename super_type::result_type result_type;
  result_type operator()(const argument_type& x) const {
    return iv::core::StringToHash(x);
  }
};

} IV_HASH_NAMESPACE_END  // namespace std
#endif  // IV_SPACE_H_
