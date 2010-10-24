#ifndef _IV_LV5_GC_TEMPLATE_H_
#define _IV_LV5_GC_TEMPLATE_H_
#include <string>
#include <vector>
#include <map>
#include <tr1/unordered_map>
#include <gc/gc_allocator.h>
#include "uchar.h"

namespace iv {
namespace lv5 {

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
  typedef std::tr1::unordered_map<T1,
                                  T2,
                                  std::tr1::hash<T1>,
                                  std::equal_to<T1>,
                                  gc_allocator<
                                    std::pair<const T1, T2> > > type;
};


typedef std::basic_string<UChar,
                          std::char_traits<UChar>,
                          gc_allocator<UChar> > GCUString;

} }  // namespace iv::lv5
#endif  // _IV_LV5_GC_TEMPLATE_H_
