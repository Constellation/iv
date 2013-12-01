#ifndef IV_LV5_JSWEAK_MAP_H_
#define IV_LV5_JSWEAK_MAP_H_
#include <iv/ustring.h>
#include <iv/all_static.h>
#include <iv/lv5/gc_template.h>
#include <iv/lv5/weak_box.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/jsobject_fwd.h>
#include <iv/lv5/map.h>
#include <iv/lv5/class.h>
namespace iv {
namespace lv5 {

class JSWeakMap : public core::AllStatic {
 public:
  // use very simple open-address hash table
  class WeakCellMap : public radio::HeapObject<> {
   public:
    typedef WeakBox<radio::Cell> Box;
    enum State {
      EMPTY = 0,
      DELETED,
      USED
    };

    struct Element {
      Box* key;
      JSVal value;
      State state;
      Element() : key(nullptr), value(JSUndefined), state(EMPTY) { }
    };

    typedef GCVector<Element>::type Vector;

    static const int kInitialSize = 1 << 2;

    WeakCellMap() : vector_(kInitialSize), size_(0) { }

    void Set(radio::Cell* cell, JSVal value) {
      do {
        std::size_t key = (Hash(cell) & (vector_.size() - 1));
        const std::size_t start = key;
        bool inserted = false;
        do {
          Vector::value_type& point = vector_[key];
          if (point.state == DELETED) {
            point.key->set(cell);
            point.value = value;
            point.state = USED;
            size_ += 1;
            inserted = true;
            break;
          }

          if (point.state == EMPTY) {
            point.key = Box::New(cell);
            point.value = value;
            point.state = USED;
            size_ += 1;
            inserted = true;
            break;
          }

          if (point.key->IsCollected()) {
            point.key->set(cell);
            point.value = value;
            inserted = true;
            break;
          }

          if (point.key->get() == cell) {
            point.value = value;
            inserted = true;
            break;
          }

          // next
          key += 1;
          key &= (vector_.size() - 1);
        } while (key != start);
        if (inserted) {
          break;
        }
        GrowForce();
      } while (true);
      Grow();
    }

    JSVal Get(radio::Cell* cell) {
      std::size_t key = (Hash(cell) & (vector_.size() - 1));
      const std::size_t start = key;
      do {
        Vector::value_type& point = vector_[key];
        if (point.state == EMPTY) {
          return JSUndefined;
        }
        if (point.state == USED) {
          if (!point.key->IsCollected()) {
            if (point.key->get() == cell) {
              return point.value;
            }
          } else {
            point.value = JSUndefined;
            point.state = DELETED;
            size_ -= 1;
          }
        }
        // next
        key += 1;
        key &= (vector_.size() - 1);
      } while (key != start);
      return JSUndefined;
    }

    bool Has(radio::Cell* cell) {
      std::size_t key = (Hash(cell) & (vector_.size() - 1));
      const std::size_t start = key;
      do {
        Vector::value_type& point = vector_[key];
        if (point.state == EMPTY) {
          return false;
        }
        if (point.state == USED) {
          if (!point.key->IsCollected()) {
            if (point.key->get() == cell) {
              return true;
            }
          } else {
            point.value = JSUndefined;
            point.state = DELETED;
            size_ -= 1;
          }
        }
        // next
        key += 1;
        key &= (vector_.size() - 1);
      } while (key != start);
      return false;
    }

    bool Delete(radio::Cell* cell) {
      std::size_t key = (Hash(cell) & (vector_.size() - 1));
      const std::size_t start = key;
      do {
        Vector::value_type& point = vector_[key];
        if (point.state == EMPTY) {
          return false;
        }
        if (point.state == USED) {
          if (!point.key->IsCollected()) {
            if (point.key->get() == cell) {
              point.key->set(nullptr);
              point.value = JSUndefined;
              point.state = DELETED;
              size_ -= 1;
              return true;
            }
          } else {
            point.value = JSUndefined;
            point.state = DELETED;
            size_ -= 1;
          }
        }
        // next
        key += 1;
        key &= (vector_.size() - 1);
      } while (key != start);
      return false;
    }

    void Clear() {
      vector_.clear();
      vector_.resize(kInitialSize);
      size_ = 0;
    }

   private:
    static std::size_t Hash(radio::Cell* cell) {
      return std::hash<radio::Cell*>()(cell);
    }

    static std::size_t Hash(Box* box) {
      if (box->IsCollected()) {
        return 0;
      }
      return std::hash<radio::Cell*>()(box->get());
    }

    void Grow() {
      if ((static_cast<double>(size_) / vector_.size()) >= 0.75) {
        GrowForce();
      }
    }

    void GrowForce() {
      Vector current(vector_.size() * 2);
      current.swap(vector_);
      size_ = 0;

      for (Vector::iterator it = current.begin(), last = current.end(); it != last; ++it) {
        const Vector::value_type& point = *it;
        if (point.state == USED && !point.key->IsCollected()) {
          Set(point.key->get(), point.value);
        }
      }
    }

    Vector vector_;
    std::size_t size_;
  };

  typedef WeakCellMap Data;

  static JSObject* Initialize(Context* ctx, JSVal input, JSVal it, Error* e) {
    if (!input.IsObject()) {
      e->Report(Error::Type, "WeakMapInitialize to non-object");
      return nullptr;
    }

    JSObject* obj = input.object();
    if (obj->HasOwnProperty(ctx, symbol())) {
      e->Report(Error::Type, "re-initialize map object");
      return nullptr;
    }

    if (!obj->IsExtensible()) {
      e->Report(Error::Type, "WeakMapInitialize to un-extensible object");
      return nullptr;
    }

    JSObject* iterable = nullptr;
    JSFunction* adder = nullptr;
    if (!it.IsUndefined()) {
      iterable = it.ToObject(ctx, IV_LV5_ERROR(e));
      JSVal val = obj->Get(ctx, symbol::set(), IV_LV5_ERROR(e));
      if (!val.IsCallable()) {
        e->Report(Error::Type, "WeakMapInitialize adder, `obj.set` is not callable");
        return nullptr;
      }
      adder = static_cast<JSFunction*>(val.object());
    }

    Data* data = new Data;

    obj->DefineOwnProperty(
      ctx,
      symbol(),
      DataDescriptor(JSVal::Cell(data), ATTR::W | ATTR::E | ATTR::C),
      false, IV_LV5_ERROR(e));

    if (iterable) {
      // TODO(Constellation) iv / lv5 doesn't have iterator system
      PropertyNamesCollector collector;
      iterable->GetOwnPropertyNames(ctx, &collector, EXCLUDE_NOT_ENUMERABLE);
      for (PropertyNamesCollector::Names::const_iterator
           it = collector.names().begin(),
           last = collector.names().end();
           it != last; ++it) {
        const JSVal v = iterable->Get(ctx, (*it), IV_LV5_ERROR(e));
        JSObject* item = v.ToObject(ctx, IV_LV5_ERROR(e));
        const JSVal key =
            item->Get(ctx, symbol::MakeSymbolFromIndex(0), IV_LV5_ERROR(e));
        const JSVal value =
            item->Get(ctx, symbol::MakeSymbolFromIndex(1), IV_LV5_ERROR(e));
        ScopedArguments arg_list(ctx, 2, IV_LV5_ERROR(e));
        arg_list[0] = key;
        arg_list[1] = value;
        adder->Call(&arg_list, obj, IV_LV5_ERROR(e));
      }
    }

    return obj;
  }

  static Symbol symbol() {
    static const char* const key = "[[WeakMapData]]";
    return symbol::MakePrivateSymbol(key);
  }
};


} }  // namespace iv::lv5
#endif  // IV_LV5_JSWEAK_MAP_H_
