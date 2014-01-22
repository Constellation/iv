// C++11 compatible gc allocator
#ifndef IV_LV5_GC_ALLOCATOR_H_
#define IV_LV5_GC_ALLOCATOR_H_
#include <new>
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
    return library_.allocate(size);
  }

  void deallocate(value_type* ptr, std::size_t size) {
    library_.deallocate(ptr, size);
  }

  template<class U, class... Args>
  void construct(U* p, Args&&... args) {
    // FIXME(Yusuke Suzuki): This cast is needed to build lv5 on FreeBSD 10.0.
    new (const_cast<void*>(reinterpret_cast<const void*>(p)))
        U(std::forward<Args>(args)...);
  }

  template< class U >
  void destroy(U* p) {
    p->~U();
  }

  GCAllocator() : library_() { }
  GCAllocator(const GCAllocator&) : library_() { }
  template<class U>
  GCAllocator(const GCAllocator<U>&) : library_() { }

  pointer address(reference ref) const { return &ref; }
  const_pointer address(const_reference ref) const { return &ref; }

  size_type max_size() const { return size_t(-1) / sizeof(value_type); }

  template<typename U>
  bool operator==(const GCAllocator<U>&) { return true; }
  template<typename U>
  bool operator!=(const GCAllocator<U>&) { return false; }

 private:
  gc_allocator<T> library_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GC_ALLOCATOR_H_
