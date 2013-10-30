// modified under the following, original copyright

// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// And this is modified by iv project
#ifndef IV_QHASHMAP_H_
#define IV_QHASHMAP_H_

#include <algorithm>
#include <cassert>
#include <utility>
#include <iv/platform.h>
namespace iv {
namespace core {

struct QHashMapDefaultAlloc {
  void* New(size_t sz) { return operator new(sz); }
  static void Delete(void* p) { operator delete(p); }
};

template<typename KeyType,
         typename ValueType,
         class KeyTraits, class Allocator = QHashMapDefaultAlloc>
class QHashMap {
  QHashMap& operator=(const QHashMap&);  // = delete
 public:
  // The default capacity.  This is used by the call sites which want
  // to pass in a non-default AllocationPolicy but want to use the
  // default value of capacity specified by the implementation.
  static const size_t kDefaultHashMapCapacity = 8;

  // initial_capacity is the size of the initial hash map;
  // it must be a power of 2 (and thus must not be 0).
  QHashMap(size_t capacity = kDefaultHashMapCapacity,
           Allocator allocator = Allocator());
  QHashMap(const QHashMap& x, Allocator allocator = Allocator());
  template<typename Iter>
  QHashMap(Iter it, Iter last, Allocator allocator = Allocator()) {
    Initialize(kDefaultHashMapCapacity, allocator);
    for (; it != last; ++it) {
      Lookup(it->first, true)->second = it->second;
    }
  }


  ~QHashMap();

  // HashMap entries are (key, value, hash) triplets.
  // Some clients may not need to use the value slot
  // (e.g. implementers of sets, where the key is the value).
  typedef std::pair<KeyType, ValueType> value_type;
  typedef value_type Entry;

  // If an entry with matching key is found, Lookup()
  // returns that entry. If no matching entry is found,
  // but insert is set, a new entry is inserted with
  // corresponding key, key hash, and NULL value.
  // Otherwise, NULL is returned.
  IV_ALWAYS_INLINE Entry* Lookup(
      KeyType key, bool insert, Allocator allocator = Allocator());
  IV_ALWAYS_INLINE std::pair<Entry*, bool>
      LookupWithFound(
          KeyType key, bool insert, Allocator allocator = Allocator());

  // Removes the entry with matching key.
  void Remove(Entry* p);
  bool Remove(KeyType key);

  // Empties the hash map (occupancy() == 0).
  void Clear();

  // The number of (non-empty) entries in the table.
  size_t size() const { return occupancy_; }
  bool empty() const { return size() == 0; }

  // The capacity of the table. The implementation
  // makes sure that occupancy is at most 80% of
  // the table capacity.
  size_t capacity() const { return capacity_; }

  // Iteration
  //
  // for (Entry* p = map.Start(); p != NULL; p = map.Next(p)) {
  //   ...
  // }
  //
  // If entries are inserted during iteration, the effect of
  // calling Next() is undefined.
  Entry* Start() const;
  Entry* Next(Entry* p) const;

 private:
  Entry* map_;
  size_t capacity_;
  size_t occupancy_;

  Entry* map_end() const { return map_ + capacity_; }
  Entry* Probe(KeyType key) const;
  void Initialize(size_t capacity, Allocator allocator = Allocator());
  void Resize(Allocator allocator = Allocator());

  // If an entry with matching key is found, Lookup()
  // returns that entry. If no matching entry is found, NULL is returned.
  IV_ALWAYS_INLINE Entry*
      Lookup(KeyType key, Allocator allocator = Allocator()) const;

 public:
  class const_iterator
    : public std::iterator<std::forward_iterator_tag, value_type> {
    const_iterator operator++(int);  // disabled NOLINT
   public:
    const_iterator& operator++() {
      entry_ = map_->Next(entry_);
      return *this;
    }

    const Entry& operator*() const { return *entry_; }
    const Entry* operator->() const { return entry_; }
    bool operator==(const const_iterator& other) const {
      return entry_ == other.entry_;
    }
    bool operator!=(const const_iterator& other) const {
      return entry_ != other.entry_;
    }

   private:
    const_iterator(const QHashMap* map, Entry* entry)
      : map_(map), entry_(entry) {}

   protected:
    const QHashMap* map_;
    Entry* entry_;

    friend class QHashMap<KeyType, ValueType, KeyTraits, Allocator>;
  };

  class iterator : public const_iterator {
   public:
    iterator& operator++() {
      this->entry_ = this->map_->Next(this->entry_);
      return *this;
    }
    Entry& operator*() const { return *this->entry_; }
    Entry* operator->() const { return this->entry_; }
   private:
    iterator(const QHashMap* map, Entry* entry)
      : const_iterator(map, entry) {}

    friend class QHashMap<KeyType, ValueType, KeyTraits, Allocator>;
  };

  typedef KeyType key_type;
  typedef ValueType mapped_type;

  iterator begin() { return iterator(this, this->Start()); }
  iterator end() { return iterator(this, NULL); }
  const_iterator begin() const { return const_iterator(this, this->Start()); }
  const_iterator end() const { return const_iterator(this, NULL); }
  const_iterator cbegin() const { return const_iterator(this, this->Start()); }
  const_iterator cend() const { return const_iterator(this, NULL); }

  IV_ALWAYS_INLINE iterator find(KeyType key) {
    return iterator(this, this->Lookup(key));
  }

  IV_ALWAYS_INLINE const_iterator find(KeyType key) const {
    return const_iterator(this, this->Lookup(key));
  }

  void clear() { Clear(); }

  // They're not compatible to STL
  void erase(const const_iterator& i) {
    Remove(i.entry_);
  }

  void erase(const key_type& i) {
    Remove(i);
  }

  void insert(const value_type& pair) {
    Lookup(pair.first, true)->second = pair.second;
  }

  mapped_type& operator[](const key_type& key) {
    return Lookup(key, true)->second;
  }
};

template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline QHashMap<KeyType, ValueType, KeyTraits, Allocator>::QHashMap(
    size_t initial_capacity, Allocator allocator) {
  Initialize(initial_capacity, allocator);
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline QHashMap<KeyType, ValueType, KeyTraits, Allocator>::QHashMap(
    const QHashMap& x, Allocator allocator) {
  map_ = static_cast<Entry*>(allocator.New(x.capacity_ * sizeof(Entry)));
  capacity_ = x.capacity_;
  occupancy_ = x.occupancy_;
  std::copy(x.map_, x.map_ + x.capacity_, map_);
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline QHashMap<KeyType, ValueType, KeyTraits, Allocator>::~QHashMap() {
  Allocator::Delete(map_);
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline typename QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Entry*
QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Lookup(
    KeyType key, Allocator allocator) const {
  // Find a matching entry.
  Entry* p = Probe(key);
  if (p->first != KeyTraits::null()) {
    return p;
  }
  // No entry found and none inserted.
  return NULL;
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline typename QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Entry*
QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Lookup(
    KeyType key, bool insert, Allocator allocator) {
  // Find a matching entry.
  Entry* p = Probe(key);
  if (p->first != KeyTraits::null()) {
    return p;
  }

  // No entry found; insert one if necessary.
  if (insert) {
    p->first = key;
    // p->second = NULL;
    occupancy_++;

    // Grow the map if we reached >= 80% occupancy.
    if (occupancy_ + occupancy_/4 >= capacity_) {
      Resize(allocator);
      p = Probe(key);
    }

    return p;
  }

  // No entry found and none inserted.
  return NULL;
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline std::pair<
  typename QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Entry*, bool>
QHashMap<KeyType, ValueType, KeyTraits, Allocator>::LookupWithFound(
    KeyType key, bool insert, Allocator allocator) {
  // Find a matching entry.
  Entry* p = Probe(key);
  if (p->first != KeyTraits::null()) {
    return std::make_pair(p, true);
  }

  // No entry found; insert one if necessary.
  if (insert) {
    p->first = key;
    // p->second = NULL;
    occupancy_++;

    // Grow the map if we reached >= 80% occupancy.
    if (occupancy_ + occupancy_/4 >= capacity_) {
      Resize(allocator);
      p = Probe(key);
    }

    return std::make_pair(p, false);
  }

  // No entry found and none inserted.
  return std::make_pair(static_cast<Entry*>(NULL), false);
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline void QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Remove(
    Entry* p) {
  // To remove an entry we need to ensure that it does not create an empty
  // entry that will cause the search for another entry to stop too soon. If all
  // the entries between the entry to remove and the next empty slot have their
  // initial position inside this interval, clearing the entry to remove will
  // not break the search. If, while searching for the next empty entry, an
  // entry is encountered which does not have its initial position between the
  // entry to remove and the position looked at, then this entry can be moved to
  // the place of the entry to remove without breaking the search for it. The
  // entry made vacant by this move is now the entry to remove and the process
  // starts over.
  // Algorithm from http://en.wikipedia.org/wiki/Open_addressing.

  // This guarantees loop termination as there is at least one empty entry so
  // eventually the removed entry will have an empty entry after it.
  assert(occupancy_ < capacity_);

  // p is the candidate entry to clear. q is used to scan forwards.
  Entry* q = p;  // Start at the entry to remove.
  while (true) {
    // Move q to the next entry.
    q = q + 1;
    if (q == map_end()) {
      q = map_;
    }

    // All entries between p and q have their initial position between p and q
    // and the entry p can be cleared without breaking the search for these
    // entries.
    if (q->first == KeyTraits::null()) {
      break;
    }

    // Find the initial position for the entry at position q.
    Entry* r = map_ + (KeyTraits::hash(q->first) & (capacity_ - 1));

    // If the entry at position q has its initial position outside the range
    // between p and q it can be moved forward to position p and will still be
    // found. There is now a new candidate entry for clearing.
    if ((q > p && (r <= p || r > q)) ||
        (q < p && (r <= p && r > q))) {
      *p = *q;
      p = q;
    }
  }

  // Clear the entry which is allowed to en emptied.
  p->first = KeyTraits::null();
  occupancy_--;
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline bool QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Remove(
    KeyType key) {
  // Lookup the entry for the key to remove.
  Entry* p = Probe(key);
  if (p->first == KeyTraits::null()) {
    // Key not found nothing to remove.
    return false;
  }
  Remove(p);
  return true;
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline void QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Clear() {
  // Mark all entries as empty.
  const Entry* end = map_end();
  for (Entry* p = map_; p < end; p++) {
    p->first = KeyTraits::null();
  }
  occupancy_ = 0;
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline typename QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Entry*
QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Start() const {
  return Next(map_ - 1);
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline typename QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Entry*
QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Next(Entry* p) const {
  const Entry* end = map_end();
  assert(map_ - 1 <= p && p < end);
  for (p++; p < end; p++) {
    if (p->first != KeyTraits::null()) {
      return p;
    }
  }
  return NULL;
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline typename QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Entry*
QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Probe(KeyType key) const {
  assert(key != KeyTraits::null());

  assert((capacity_ & (capacity_ - 1)) == 0);
  const unsigned hash = KeyTraits::hash(key);
  Entry* p = map_ + (hash & (capacity_ - 1));
  const Entry* end = map_end();
  assert(map_ <= p && p < end);

  assert(occupancy_ < capacity_);  // Guarantees loop termination.
  while (p->first != KeyTraits::null()
         && (hash != KeyTraits::hash(p->first) ||
             !KeyTraits::equals(key, p->first))) {
    p++;
    if (p >= end) {
      p = map_;
    }
  }

  return p;
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline void QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Initialize(
    size_t capacity, Allocator allocator) {
  assert((capacity & (capacity - 1)) == 0);
  map_ = reinterpret_cast<Entry*>(allocator.New(capacity * sizeof(Entry)));
  capacity_ = capacity;
  Clear();
}


template<typename KeyType, typename ValueType, class KeyTraits, class Allocator>
inline void QHashMap<KeyType, ValueType, KeyTraits, Allocator>::Resize(
    Allocator allocator) {
  Entry* map = map_;
  size_t n = occupancy_;

  // Allocate larger map.
  Initialize(capacity_ * 2, allocator);

  // Rehash all current entries.
  for (Entry* p = map; n > 0; p++) {
    if (p->first != KeyTraits::null()) {
      Lookup(p->first, true)->second = p->second;
      n--;
    }
  }

  // Delete old map.
  Allocator::Delete(map);
}

} }  // namespace iv::core
#endif  // IV_QHASHMAP_H_
