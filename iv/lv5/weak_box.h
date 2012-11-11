// Based on Gauche's WeakBox implementation
//
// Copyright (c) 2000-2012  Shiro Kawai  <shiro@acm.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the authors nor the names of its contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#ifndef IV_LV5_WEAK_BOX_H_
#define IV_LV5_WEAK_BOX_H_
#include <gc/gc.h>
#include <iv/noncopyable.h>
namespace iv {
namespace lv5 {

template<typename T>
class WeakBox : core::Noncopyable<WeakBox<T> > {
 public:
  typedef WeakBox<T> this_type;
  typedef T** pointer;
  void set(T* ptr) {
    pointer target = &data_;
    // release
    if (registered_) {
      GC_unregister_disappearing_link(reinterpret_cast<void**>(target));
      registered_ = false;
    }
    // register
    *target = ptr;
    if (ptr) {
      GC_general_register_disappearing_link(reinterpret_cast<void**>(target), reinterpret_cast<void*>(ptr));
      registered_ = true;
    }
  }

  T* get() const { return data_; }

  bool IsCollected() const { return registered_ && !get(); };

  static this_type* New(T* ptr = NULL) {
    this_type* box = static_cast<this_type*>(GC_MALLOC_ATOMIC(sizeof(this_type)));
    box->data_ = NULL;
    box->registered_ = false;
    if (ptr) {
      box->set(ptr);
    }
    return box;
  }
 private:
  T* data_;
  bool registered_;
};

} }  // namespace iv::lv5
#endif  // IV_LV5_WEAK_BOX_H_
