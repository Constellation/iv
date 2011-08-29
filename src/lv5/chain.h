// Chain object
// save [[Prototype]] Chain and inline into bytecode for prototype PIC
//
// see also map.h
//
#ifndef _IV_LV5_CHAIN_H_
#define _IV_LV5_CHAIN_H_
#include <gc/gc.h>
#include <iterator>
#include <new>
#include "debug.h"
#include "utils.h"
#include "noncopyable.h"
#include "lv5/jsobject.h"
namespace iv {
namespace lv5 {

class Map;

class Chain : private core::Noncopyable<Chain> {
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

  Chain* New(JSObject* begin, JSObject* end, std::size_t count) {
    void* mem = GC_MALLOC(GetControlSize() + sizeof(Map*) * count);
    return new (mem) Chain(begin, end, count);
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
    return reinterpret_cast<pointer>(this) + GetControlSize() / sizeof(value_type);
  }

  const_pointer data() const {
    return reinterpret_cast<const_pointer>(this) + GetControlSize() / sizeof(value_type);
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

 private:
  Chain(JSObject* from, JSObject* to, size_type count)
    : size_(count) {
    iterator it = begin();
    JSObject* current = from;
    while (true) {
      assert(current);
      *it++ = current->map();
      if (current->prototype() == to) {
        // last one
        break;
      } else {
        current = current->prototype();
      }
    }
    assert(it == end());
  }

  size_type size_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_CHAIN_H_
