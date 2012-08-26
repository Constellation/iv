// PolyIC compiler
#ifndef IV_BREAKER_POLY_IC_H_
#define IV_BREAKER_POLY_IC_H_
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/ic.h>
#include <iv/intrusive_list.h>
namespace iv {
namespace lv5 {
namespace breaker {

class PolyICUnit: public core::IntrusiveListBase {
 public:
  enum Type {
    LOAD_OWN_PROPERTY,
    LOAD_PROTO_PROPERTY,
    LOAD_CHAIN_PROPERTY,
    LOAD_STRING_OWN_PROPERTY,
    LOAD_STRING_PROTO_PROPERTY,
    LOAD_STRING_CHAIN_PROPERTY
  };

  explicit PolyICUnit(Type type)
    : own_(NULL),
      proto_(NULL),
      type_(type),
      tail_(0) {
  }

  std::size_t tail() const { return tail_; }

  Map* own() const { return own_; }
  Map* proto() const { return proto_; }
  Chain* chain() const { return chain_; }

 protected:
  void set_tail(std::size_t tail) {
    tail_ = tail;
  }

  union {
    Map* own_;
    Chain* chain_;
  };
  Map* proto_;
  Type type_;
  std::size_t tail_;
};

class PolyIC : public IC, public core::IntrusiveList<PolyICUnit> {
 public:
  typedef PolyICUnit IC;

  ~PolyIC() {
    for (iterator it = begin(), last = end(); it != last;) {
      IC* ic = &*it;
      ++it;
      ic->Unlink();
      delete ic;
    }
  }

  virtual GC_ms_entry* MarkChildren(GC_word* top,
                                    GC_ms_entry* entry,
                                    GC_ms_entry* mark_sp_limit,
                                    GC_word env) {
    for (iterator it = begin(), last = end(); it != last; ++it) {
      entry = GC_MARK_AND_PUSH(
          it->own(),
          entry, mark_sp_limit, reinterpret_cast<void**>(this));
      entry = GC_MARK_AND_PUSH(
          it->proto(),
          entry, mark_sp_limit, reinterpret_cast<void**>(this));
    }
    return entry;
  }

  virtual void MarkChildren(radio::Core* core) {
    for (iterator it = begin(), last = end(); it != last; ++it) {
      core->MarkCell(it->own());
      core->MarkCell(it->proto());
    }
  }
};

class LoadPropertyIC : public PolyIC {
 public:
  static const std::size_t kMaxPolyICSize = 5;

  void LoadOwnProperty(NativeCode* code, Map* map, uint32_t offset) {
    if (size() <= kMaxPolyICSize) {
      return;
    }
    IC* ic = new IC(IC::LOAD_OWN_PROPERTY);
    NativeCode::Pages::Buffer buffer = code->pages()->Gain(256);
    Xbyak::CodeGenerator gen(buffer.size, buffer.ptr);
    push_back(*ic);
  }

  void LoadPrototypeProperty(NativeCode* code,
                             Map* map, Map* proto, uint32_t offset) {
    if (size() <= kMaxPolyICSize) {
      return;
    }
    IC* ic = new IC(IC::LOAD_PROTO_PROPERTY);
    NativeCode::Pages::Buffer buffer = code->pages()->Gain(256);
    Xbyak::CodeGenerator gen(buffer.size, buffer.ptr);
    push_back(*ic);
  }

  void LoadChainProperty(NativeCode* code,
                         Chain* chain, uint32_t offset) {
    if (size() <= kMaxPolyICSize) {
      return;
    }
    IC* ic = new IC(IC::LOAD_CHAIN_PROPERTY);
    NativeCode::Pages::Buffer buffer = code->pages()->Gain(256);
    Xbyak::CodeGenerator gen(buffer.size, buffer.ptr);
    push_back(*ic);
  }
};

class StorePropertyIC : public PolyIC {
  static const std::size_t kMaxPolyICSize = 5;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_POLY_IC_H_
