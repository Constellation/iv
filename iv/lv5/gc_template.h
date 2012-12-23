#ifndef IV_LV5_GC_TEMPLATE_H_
#define IV_LV5_GC_TEMPLATE_H_
#include <string>
#include <vector>
#include <map>
#include <gc/gc_allocator.h>
#include <gc/gc_cpp.h>
#include <iv/detail/unordered_map.h>
#include <iv/detail/unordered_set.h>
#include <iv/detail/cstdint.h>

namespace iv {
namespace lv5 {
namespace trace {

template<typename T>
struct Vector {
  typedef std::vector<T, traceable_allocator<T> > type;
};

template<typename T1, typename T2, typename Less = std::less<T1> >
struct Map {
  typedef std::map<T1, T2, Less,
                   traceable_allocator<std::pair<const T1, T2> > > type;
};

template<typename T1, typename T2,
         typename Hash = std::hash<T1>,
         typename Equal = std::equal_to<T1> >
struct HashMap {
  typedef std::unordered_map<T1, T2, Hash, Equal,
                             traceable_allocator<
                               std::pair<const T1, T2> > > type;
};

template<typename T,
         typename Hash = std::hash<T>,
         typename Equal = std::equal_to<T> >
struct HashSet {
  typedef std::unordered_set<T, Hash, Equal, traceable_allocator<T> > type;
};

}  // namespace trace

template<typename T>
struct GCVector {
  typedef std::vector<T, gc_allocator<T> > type;
};

template<typename T1, typename T2, typename Less = std::less<T1> >
struct GCMap {
  typedef std::map<T1, T2, Less,
                   gc_allocator<std::pair<const T1, T2> > > type;
};

template<typename T1, typename T2,
         typename Hash = std::hash<T1>,
         typename Equal = std::equal_to<T1> >
struct GCHashMap {
  typedef std::unordered_map<T1, T2, Hash, Equal,
                             gc_allocator<std::pair<const T1, T2> > > type;
};

template<typename T,
         typename Hash = std::hash<T>,
         typename Equal = std::equal_to<T> >
struct GCHashSet {
  typedef std::unordered_set<T, Hash, Equal, gc_allocator<T> > type;
};

typedef std::basic_string<uint16_t,
                          std::char_traits<uint16_t>,
                          gc_allocator<uint16_t> > GCUString;

struct GCAlloc {
  void* New(size_t sz) { return operator new(sz, GC); }
  static void Delete(void* p) {
    // do nothing because of GC
  }
};

} }  // namespace iv::lv5
#endif  // IV_LV5_GC_TEMPLATE_H_
