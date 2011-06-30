#ifndef _IV_LV5_GC_TEMPLATE_H_
#define _IV_LV5_GC_TEMPLATE_H_
#include <string>
#include <vector>
#include <map>
#include <gc/gc_allocator.h>
#include "detail/unordered_map.h"
#include "detail/unordered_set.h"
#include "detail/cstdint.h"

namespace iv {
namespace lv5 {
namespace trace {

template<typename T>
struct Vector {
  typedef std::vector<T, traceable_allocator<T> > type;
};

template<typename T1, typename T2>
struct Map {
  typedef std::map<T1,
                   T2,
                   std::less<T1>,
                   traceable_allocator<std::pair<const T1, T2> > > type;
};

template<typename T1, typename T2>
struct HashMap {
  typedef std::unordered_map<T1,
                                  T2,
                                  std::hash<T1>,
                                  std::equal_to<T1>,
                                  traceable_allocator<
                                    std::pair<const T1, T2> > > type;
};

template<typename T>
struct HashSet {
  typedef std::unordered_set<T,
                                  std::hash<T>,
                                  std::equal_to<T>,
                                  traceable_allocator<T> > type;
};

}  // namespace trace

template<typename T>
struct GCVector {
  typedef std::vector<T, gc_allocator<T> > type;
};

template<typename T1, typename T2>
struct GCMap {
  typedef std::map<T1,
                   T2,
                   std::less<T1>,
                   gc_allocator<std::pair<const T1, T2> > > type;
};

template<typename T1, typename T2>
struct GCHashMap {
  typedef std::unordered_map<T1,
                                  T2,
                                  std::hash<T1>,
                                  std::equal_to<T1>,
                                  gc_allocator<
                                    std::pair<const T1, T2> > > type;
};

template<typename T>
struct GCHashSet {
  typedef std::unordered_set<T,
                                  std::hash<T>,
                                  std::equal_to<T>,
                                  gc_allocator<T> > type;
};

typedef std::basic_string<uint16_t,
                          std::char_traits<uint16_t>,
                          gc_allocator<uint16_t> > GCUString;

} }  // namespace iv::lv5
#endif  // _IV_LV5_GC_TEMPLATE_H_
