// C++11 compatible gc allocator
#ifndef IV_LV5_GC_ALLOCATOR_H_
#define IV_LV5_GC_ALLOCATOR_H_
#include <gc/gc.h>
#include <gc/gc_allocator.h>
namespace iv {
namespace lv5 {

template <typename T>
class GCAllocator {
 public:
  typedef gc_allocator<T>   original_type;
  typedef T                 value_type;
  typedef size_t            size_type;
  typedef ptrdiff_t         difference_type;
  typedef value_type*       pointer;
  typedef const value_type* const_pointer;
  typedef value_type&       reference;
  typedef const value_type& const_reference;

  template <class U>
  struct rebind {
    typedef GCAllocator<U> other;
  };

  value_type* allocate(std::size_t size) {
    GC_type_traits<value_type> traits;
    return static_cast<value_type*>(GC_selective_alloc(size * sizeof(value_type), traits.GC_is_ptr_free, true));
  }

  void deallocate(value_type* ptr, std::size_t size) {
    GC_FREE(ptr);
  }

  template<class U, class... Args>
  void construct(U* p, Args&&... args) {
    new (p) U(std::forward<Args>(args)...);
  }

  template< class U >
  void destroy(U* p) {
    p->~U();
  }

  GCAllocator() { }
  GCAllocator(const GCAllocator&) { }
  template<class U>
  GCAllocator(const GCAllocator<U>&) { }

  template<typename U>
  bool operator==(const GCAllocator<U>&) { return true; }
  template<typename U>
  bool operator!=(const GCAllocator<U>&) { return false; }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GC_ALLOCATOR_H_
