// Chain object
// save [[Prototype]] Chain and inline into bytecode for prototype PIC
//
// see also map.h
//
#ifndef IV_LV5_CHAIN_H_
#define IV_LV5_CHAIN_H_
#include <gc/gc.h>
#include <iterator>
#include <new>
#include <iv/debug.h>
#include <iv/utils.h>
#include <iv/noncopyable.h>
#include <iv/lv5/jsobject_fwd.h>
namespace iv {
namespace lv5 {

class Map;

class Chain : public radio::HeapObject<radio::POINTER> {
 public:
  typedef Chain this_type;
  typedef Map* value_type;
  typedef value_type* iterator;
  typedef const value_type* const_iterator;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  // chain not contains end map
  static Chain* New(const JSCell* begin, const JSCell* end) {
    assert(begin != end);
    assert(begin != NULL);
    std::vector<Map*> maps;
    maps.reserve(4);
    const JSCell* current = begin;
    do {
      Map* map = current->FlattenMap();
      maps.push_back(map);
      current = current->prototype();
    } while (current != end);
    void* mem = GC_MALLOC(
        GetControlSize() + sizeof(Map*) * maps.size());  // NOLINT
    return new (mem) Chain(maps.begin(), maps.size());
  }

  static std::size_t GetControlSize() {
    return IV_ROUNDUP(sizeof(this_type), sizeof(value_type));
  }

  const_reference operator[](size_type n) const {
    return (data())[n];
  }

  reference operator[](size_type n) {
    return (data())[n];
  }

  pointer data() {
    return
        reinterpret_cast<pointer>(this) +
        GetControlSize() / sizeof(value_type);
  }

  const_pointer data() const {
    return
        reinterpret_cast<const_pointer>(this) +
        GetControlSize() / sizeof(value_type);
  }

  iterator begin() {
    return data();
  }

  const_iterator begin() const {
    return data();
  }

  const_iterator cbegin() const {
    return data();
  }

  iterator end() {
    return begin() + size();
  }

  const_iterator end() const {
    return begin() + size();
  }

  const_iterator cend() const {
    return begin() + size();
  }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const {
    return rbegin();
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const {
    return rend();
  }

  size_type size() const {
    return size_;
  }

  // maybe return NULL
  JSObject* Validate(JSObject* target, Map* cache) const {
    JSObject* current = target;
    for (const_iterator it = cbegin(), last = cend();
         current && it != last; ++it, current = current->prototype()) {
      if (*it != current->map()) {
        return NULL;
      }
    }
    return (current && cache == current->map()) ? current : NULL;
  }

  void MarkChildren(radio::Core* core) {
    std::for_each(begin(), end(), radio::Core::Marker(core));
  }

 private:
  template<typename Iter>
  Chain(Iter it, std::size_t count)
    : size_(count) {
    std::copy(it, it + count, begin());
  }

  size_type size_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_CHAIN_H_
