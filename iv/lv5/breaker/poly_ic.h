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

  Map* own() const { return own_; }
  Map* proto() const { return proto_; }
  Chain* chain() const { return chain_; }

  void Redirect(uintptr_t ptr) {
		for (std::size_t i = 0; i < sizeof(uintptr_t); ++i) {
			tail_[i] = static_cast<uint8_t>(ptr >> (i * 8));
		}
  }

  void set_tail(uint8_t* tail) {
    tail_ = tail;
  }
 protected:

  union {
    Map* own_;
    Chain* chain_;
  };
  Map* proto_;
  Type type_;
  uint8_t* tail_;
};

class PolyIC : public IC, public core::IntrusiveList<PolyICUnit> {
 public:
  typedef PolyICUnit Unit;

  PolyIC() : IC(IC::POLY) { }

  ~PolyIC() {
    for (iterator it = begin(), last = end(); it != last;) {
      Unit* ic = &*it;
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
  static const std::size_t kCallOffset = 5;
  static const std::size_t k64MovImmOffset = 2;

  explicit LoadPropertyIC(uintptr_t* direct_call)
    : direct_call_(direct_call) { }

  static bool CanEmitDirectCall(Xbyak::CodeGenerator* as, const void* addr) {
    return Xbyak::inner::IsInInt32(reinterpret_cast<const uint8_t*>(addr) - as->getCurr());
  }

  class GuardGenerator {
   public:
    GuardGenerator(LoadPropertyIC* ic, Xbyak::CodeGenerator* as, bool f) : ic_(ic), asm_(as), first_(f) {
      if (first()) {
        // check target is Cell
        asm_->mov(asm_->r10, detail::jsval64::kValueMask);
        asm_->test(asm_->rdi, asm_->r10);
        asm_->jnz("POLY_IC_GUARD_GENERIC");

        // target is guaranteed asm_ cell
        asm_->mov(asm_->word[asm_->rdi + radio::Cell::TagOffset()], radio::OBJECT);
        asm_->jne("POLY_IC_GUARD_STRING");  // we should purge this to string check path
      }
    }

    ~GuardGenerator() {
      if (first()) {
        asm_->L("POLY_IC_GUARD_GENERIC");
        asm_->mov(asm_->rax, core::BitCast<uint64_t>(stub::LOAD_PROP_GENERIC));
        asm_->jmp(asm_->rax);

        asm_->L("POLY_IC_GUARD_STRING");
        const uint64_t dummy64 = UINT64_C(0x0FFF000000000000);
        ic_->set_string_rewrite_position(ic_->string_rewrite_position() + (asm_->getSize() + k64MovImmOffset));
        asm_->mov(asm_->rax, dummy64);
        asm_->jmp(asm_->rax);
      }
    }

    bool first() const { return first_; }

   private:

    LoadPropertyIC* ic_;
    Xbyak::CodeGenerator* asm_;
    bool first_;
  };

  void LoadOwnProperty(NativeCode* code, Map* map, uint32_t offset) {
    if (size() >= kMaxPolyICSize) {
      return;
    }

    Unit* ic = new Unit(Unit::LOAD_OWN_PROPERTY);

    NativeCode::Pages::Buffer buffer = code->pages()->Gain(256);
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);

    as.inLocalLabel();
    {
      const GuardGenerator guard(this, &as, empty());
      if (guard.first()) {
        set_string_rewrite_position(reinterpret_cast<uint8_t*>(buffer.ptr));
      }

      // map guard
      as.mov(as.r10, core::BitCast<uintptr_t>(map));
      as.cmp(as.r10, as.qword[as.rdi + JSObject::MapOffset()]);
      as.jne(".EXIT");

      const std::ptrdiff_t data_offset =
          JSObject::SlotsOffset() +
          JSObject::Slots::DataOffset();
      as.mov(as.rax, as.qword[as.rdi + data_offset]);
      as.mov(as.rax, as.qword[as.rax + offset]);
      as.ret();

      as.L(".EXIT");
      const uint64_t dummy64 = UINT64_C(0x0FFF000000000000);
      as.mov(as.rax, dummy64);
      as.jmp(as.rax);
    }
    as.outLocalLabel();

    Chaining(ic, core::BitCast<uintptr_t>(as.getCode()));
  }

  void LoadPrototypeProperty(NativeCode* code,
                             Map* map, Map* proto, uint32_t offset) {
    if (size() >= kMaxPolyICSize) {
      return;
    }
    Unit* ic = new Unit(Unit::LOAD_PROTO_PROPERTY);
    NativeCode::Pages::Buffer buffer = code->pages()->Gain(256);
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);

    Chaining(ic, core::BitCast<uintptr_t>(as.getCode()));
  }

  void LoadChainProperty(NativeCode* code,
                         Chain* chain, uint32_t offset) {
    if (size() >= kMaxPolyICSize) {
      return;
    }
    Unit* ic = new Unit(Unit::LOAD_CHAIN_PROPERTY);
    NativeCode::Pages::Buffer buffer = code->pages()->Gain(256);
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);

    Chaining(ic, core::BitCast<uintptr_t>(as.getCode()));
  }

  void set_string_rewrite_position(uint8_t* position) {
    string_rewrite_position_ = position;
  }

  uint8_t* string_rewrite_position() const {
    return string_rewrite_position_;
  }

 private:
  void Chaining(Unit* ic, uintptr_t code) {
    if (empty()) {
      *direct_call_ = code;
    } else {
      back().Redirect(code);
    }

    if ((size() + 1) == kMaxPolyICSize) {
      // last one
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP_GENERIC));
    } else {
      // TODO(Constellation) indicate stub function
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP_CHAINING));
    }
    push_back(*ic);
  }

  uintptr_t* direct_call_;
  uint8_t* string_rewrite_position_;
};

class StorePropertyIC : public PolyIC {
  static const std::size_t kMaxPolyICSize = 5;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_POLY_IC_H_
