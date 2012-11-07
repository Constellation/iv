// PolyIC compiler
#ifndef IV_BREAKER_POLY_IC_H_
#define IV_BREAKER_POLY_IC_H_
#include <iv/lv5/breaker/assembler.h>
#include <iv/lv5/breaker/ic.h>
#include <iv/lv5/breaker/fwd.h>
#include <iv/intrusive_list.h>
namespace iv {
namespace lv5 {
namespace breaker {

class PolyICUnit: public core::IntrusiveListBase {
 public:
  enum Type {
    LOAD_OWN_PROPERTY,
    LOAD_PROTOTYPE_PROPERTY,
    LOAD_CHAIN_PROPERTY,
    LOAD_STRING_LENGTH
  };

  explicit PolyICUnit(Type type)
    : own_(NULL),
      proto_(NULL),
      type_(type),
      tail_(0) {
  }

  Map* own() const { return own_; }
  void set_own(Map* map) { own_ = map; }
  Map* proto() const { return proto_; }
  void set_proto(Map* map) { proto_ = map; }
  Chain* chain() const { return chain_; }
  void set_chain(Chain* chain) { chain_ = chain; }

  void Redirect(uintptr_t ptr) {
		for (std::size_t i = 0; i < sizeof(uintptr_t); ++i) {
			tail_[i] = static_cast<uint8_t>(ptr >> (i * 8));
		}
  }

  void set_tail(uint8_t* tail) {
    tail_ = tail;
  }
 protected:

  Map* own_;
  union {
    Chain* chain_;
    Map* proto_;
  };
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
  static const std::size_t k64MovImmOffset = 2;

  explicit LoadPropertyIC(NativeCode* native_code, bool length)
    : direct_call_(NULL),
      length_call_(),
      main_path_(NULL),
      object_chain_(NULL),
      native_(native_code),
      length_property_(length) {
  }

  void BindDirectCall(uint8_t* direct_call) {
    direct_call_ = direct_call;
    if (length_property()) {
      length_call_ = direct_call;
    }
  }

  static bool CanEmitDirectCall(Xbyak::CodeGenerator* as, const void* addr) {
    return Xbyak::inner::IsInInt32(reinterpret_cast<const uint8_t*>(addr) - as->getCurr());
  }

  // Generate Tail position
  static std::size_t GenerateTail(Xbyak::CodeGenerator* as) {
    const uint64_t dummy64 = UINT64_C(0x0FFF000000000000);
    const std::size_t result = as->getSize() + k64MovImmOffset;
    as->mov(as->rax, dummy64);
    as->jmp(as->rax);
    return result;
  }

  template<bool TRAILING_STRING>
  void GenerateGuardPrologue(Xbyak::CodeGenerator* as) {
    // check target is Cell
    as->mov(as->r10, detail::jsval64::kValueMask);
    as->test(as->rsi, as->r10);
    as->jnz("POLY_IC_GUARD_GENERIC");

    // target is guaranteed as cell
    as->mov(as->r8, as->rsi);
    as->cmp(as->word[as->rsi + radio::Cell::TagOffset()], radio::OBJECT);
    if (TRAILING_STRING) {
      assert(length_property());
      as->je("POLY_IC_GUARD_OTHER");
    } else {
      as->jne("POLY_IC_GUARD_OTHER");  // we should purge this to string check path
    }
    as->L("POLY_IC_START_MAIN");
    main_path_ = const_cast<uint8_t*>(as->getCurr());
  }

  template<bool LEADING_STRING>
  void GenerateGuardEpilogue(Context* ctx, Xbyak::CodeGenerator* as) {
    as->L("POLY_IC_GUARD_OTHER");
    if (LEADING_STRING) {
      // object path
      const std::size_t pos = GenerateTail(as);
      direct_call_ = const_cast<uint8_t*>(as->getCode()) + pos;
      Rewrite(direct_call(), core::BitCast<uintptr_t>(&stub::LOAD_PROP), k64Size);
    } else {
      // string path
      if (length_property()) {
        const std::size_t pos = GenerateTail(as);
        length_call_ = const_cast<uint8_t*>(as->getCode()) + pos;
        Rewrite(length_call(), core::BitCast<uintptr_t>(main_path()), k64Size);
      } else {
        // use String.prototype object
        JSObject* prototype = ctx->global_data()->GetClassSlot(Class::String).prototype;
        as->mov(as->r8, core::BitCast<uintptr_t>(prototype));
        as->jmp("POLY_IC_START_MAIN");
      }
    }

    // They are used as last entry
    as->L("POLY_IC_GUARD_GENERIC");
    as->mov(as->rax, core::BitCast<uint64_t>(&stub::LOAD_PROP_GENERIC));
    as->jmp(as->rax);
  }

  // load to rax
  static void GenerateFastLoad(Xbyak::CodeGenerator* as,
                               const Xbyak::Reg64& reg, uint32_t offset) {
    const std::ptrdiff_t data_offset =
        JSObject::SlotsOffset() +
        JSObject::Slots::DataOffset();
    as->mov(as->rax, as->qword[reg + data_offset]);
    as->mov(as->rax, as->qword[as->rax + kJSValSize * offset]);
    as->ret();
  }

  class LoadOwnPropertyCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_OWN_PROPERTY;
    static const int kSize = 256;

    LoadOwnPropertyCompiler(Map* map, uint32_t offset)
      : map_(map),
        offset_(offset) {
    }

    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      // own map guard
      as->mov(as->r10, core::BitCast<uintptr_t>(map_));
      as->cmp(as->r10, as->qword[as->r8 + JSObject::MapOffset()]);
      as->jne(fail);
      // load
      site->GenerateFastLoad(as, as->r8, offset_);
    }

   private:
    Map* map_;
    uint32_t offset_;
  };

  class LoadPrototypePropertyCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_PROTOTYPE_PROPERTY;
    static const int kSize = 256;

    LoadPrototypePropertyCompiler(Map* map, Map* prototype, uint32_t offset)
      : map_(map),
        prototype_(prototype),
        offset_(offset) {
    }

    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      // own map guard
      as->mov(as->r10, core::BitCast<uintptr_t>(map_));
      as->cmp(as->r10, as->qword[as->r8 + JSObject::MapOffset()]);
      as->jne(fail);
      // prototype map guard
      as->mov(as->r11, as->qword[as->r8 + JSObject::PrototypeOffset()]);
      as->mov(as->r10, core::BitCast<uintptr_t>(prototype_));
      as->or(as->r11, as->r11);
      as->jz(fail);
      as->cmp(as->r10, as->qword[as->r11 + JSObject::MapOffset()]);
      as->jne(fail);
      // load
      site->GenerateFastLoad(as, as->r11, offset_);
    }

   private:
    Map* map_;
    Map* prototype_;
    uint32_t offset_;
  };

  class LoadChainPropertyCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_CHAIN_PROPERTY;
    static const int kSize = 256;

    LoadChainPropertyCompiler(Chain* chain, Map* last, uint32_t offset)
      : map_(last),
        chain_(chain),
        offset_(offset) {
    }

    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      if (chain_->size() < 5) {
        // unroling
        Chain::const_iterator it = chain_->begin();
        const Chain::const_iterator last = chain_->end();
        assert(it != last);
        {
          as->mov(as->r10, core::BitCast<uintptr_t>(*it));
          as->cmp(as->r10, as->qword[as->r8 + JSObject::MapOffset()]);
          as->jne(fail);
          as->mov(as->r11, as->qword[as->r8 + JSObject::PrototypeOffset()]);
          ++it;
        }
        for (; it != last; ++it) {
          as->mov(as->r10, core::BitCast<uintptr_t>(*it));
          as->or(as->r11, as->r11);
          as->jz(fail);
          as->cmp(as->r10, as->qword[as->r11 + JSObject::MapOffset()]);
          as->jne(fail);
          as->mov(as->r11, as->qword[as->r11 + JSObject::PrototypeOffset()]);
        }
      } else {
        as->mov(as->r11, as->r8);
        as->mov(as->r10, core::BitCast<uintptr_t>(chain_->data()));
        as->xor(as->r9, as->r9);
        as->L(".LOOP_HEAD");
        {
          as->mov(as->r10, as->qword[as->r10 + as->r9 * k64Size]);
          as->or(as->r11, as->r11);
          as->jz(fail);
          as->cmp(as->r10, as->qword[as->r11 + JSObject::MapOffset()]);
          as->jne(fail);
          as->mov(as->r11, as->qword[as->r11 + JSObject::PrototypeOffset()]);
          as->inc(as->r9);
          as->cmp(as->r9, chain_->size());
          as->jl(".LOOP_HEAD");
        }
      }

      // last check
      as->mov(as->r10, core::BitCast<uintptr_t>(map_));
      as->or(as->r11, as->r11);
      as->jz(fail);
      as->cmp(as->r10, as->qword[as->r11 + JSObject::MapOffset()]);
      as->jne(fail);
      // load
      site->GenerateFastLoad(as, as->r11, offset_);
    }

   private:
    Map* map_;
    Chain* chain_;
    uint32_t offset_;
  };

  void LoadOwnProperty(Context* ctx, Map* map, uint32_t offset) {
    if (Unit* ic = Generate(ctx, LoadOwnPropertyCompiler(map, offset))) {
      ic->set_own(map);
    }
  }

  void LoadPrototypeProperty(Context* ctx,
                             Map* map, Map* proto, uint32_t offset) {
    if (Unit* ic = Generate(ctx, LoadPrototypePropertyCompiler(map, proto, offset))) {
      ic->set_own(map);
      ic->set_proto(proto);
    }
  }

  void LoadChainProperty(Context* ctx, Chain* chain, Map* last, uint32_t offset) {
    if (Unit* ic = Generate(ctx, LoadChainPropertyCompiler(chain, last, offset))) {
      ic->set_chain(chain);
      ic->set_own(last);
    }
  }

  void LoadStringLength(Context* ctx) {
    // inject string length check
    Unit* ic = new Unit(Unit::LOAD_STRING_LENGTH);
    NativeCode::Pages::Buffer buffer = native_->pages()->Gain(256);
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);
    const bool generate_guard = empty();

    if (generate_guard) {
      GenerateGuardPrologue<true>(&as);
    }

    as.mov(as.rax, core::BitCast<uint64_t>(Templates<>::string_length()));
    as.jmp(as.rax);

    if (generate_guard) {
      GenerateGuardEpilogue<true>(ctx, &as);
    }
    Rewrite(length_call(), core::BitCast<uintptr_t>(as.getCode()), k64Size);
    push_back(*ic);
  }

  uint8_t* direct_call() const { return direct_call_; }
  uint8_t* length_call() const { return length_call_; }
  uint8_t* main_path() const { return main_path_; }
  Unit* object_chain() const { return object_chain_; }
  bool length_property() const { return length_property_; }

 private:
  // main generation path
  template<typename Generator>
  Unit* Generate(Context* ctx, const Generator& gen) {
    if (size() >= kMaxPolyICSize) {
      return NULL;
    }

    Unit* ic = new Unit(Generator::kType);

    NativeCode::Pages::Buffer buffer = native_->pages()->Gain(Generator::kSize);
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);
    const bool generate_guard = empty();

    if (generate_guard) {
      GenerateGuardPrologue<false>(&as);
    }

    as.inLocalLabel();
    gen(this, &as, ".EXIT");
    // fail path
    as.L(".EXIT");
    ic->set_tail(const_cast<uint8_t*>(as.getCode()) + GenerateTail(&as));
    as.outLocalLabel();


    if (generate_guard) {
      GenerateGuardEpilogue<false>(ctx, &as);
    }

    ChainingObjectLoad(ic, core::BitCast<uintptr_t>(as.getCode()));
    return ic;
  }

  void ChainingObjectLoad(Unit* ic, uintptr_t code) {
    if (!object_chain()) {
      assert(direct_call());
      Rewrite(direct_call(), code, k64Size);
    } else {
      object_chain()->Redirect(code);
    }

    if ((size() + 1) == kMaxPolyICSize) {
      // last one
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP_GENERIC));
    } else {
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP));
    }
    push_back(*ic);
    object_chain_ = ic;
  }

  static void Rewrite(uint8_t* data, uint64_t disp, std::size_t size) {
		for (size_t i = 0; i < size; i++) {
			data[i] = static_cast<uint8_t>(disp >> (i * 8));
		}
  }

  uint8_t* direct_call_;
  uint8_t* length_call_;
  uint8_t* main_path_;
  Unit* object_chain_;
  NativeCode* native_;
  bool length_property_;
};

class StorePropertyIC : public PolyIC {
  static const std::size_t kMaxPolyICSize = 5;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_POLY_IC_H_
