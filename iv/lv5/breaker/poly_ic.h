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
    LOAD_STRING_LENGTH,
    LOAD_ARRAY_LENGTH
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
  static const std::size_t k64MovImmOffset = 2;
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

  static std::size_t Generate64Mov(Xbyak::CodeGenerator* as) {
    const uint64_t dummy64 = UINT64_C(0x0FFF000000000000);
    const std::size_t result = as->getSize() + k64MovImmOffset;
    as->mov(as->rax, dummy64);
    return result;
  }

  // Generate Tail position
  static std::size_t GenerateTail(Xbyak::CodeGenerator* as) {
    const std::size_t result = Generate64Mov(as);
    as->jmp(as->rax);
    return result;
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

  explicit LoadPropertyIC(NativeCode* native_code, Symbol name)
    : from_original_(),
      main_path_(NULL),
      object_chain_(NULL),
      native_code_(native_code),
      name_(name) {
  }

  void BindOriginal(std::size_t from_original) {
    from_original_ = from_original;
    native_code()->assembler()->rewrite(from_original_, core::BitCast<uintptr_t>(&stub::LOAD_PROP), k64Size);
  }

  static bool CanEmitDirectCall(Xbyak::CodeGenerator* as, const void* addr) {
    return Xbyak::inner::IsInInt32(reinterpret_cast<const uint8_t*>(addr) - as->getCurr());
  }

  template<bool TRAILING_STRING>
  void GenerateGuardPrologue(Xbyak::CodeGenerator* as) {
    // check target is Cell
    as->mov(as->r10, detail::jsval64::kValueMask);
    as->test(as->rsi, as->r10);
    as->jnz("POLY_IC_GUARD_GENERIC", Xbyak::CodeGenerator::T_NEAR);

    // target is guaranteed as cell
    as->mov(as->r8, as->rsi);
    as->cmp(as->word[as->rsi + radio::Cell::TagOffset()], radio::OBJECT);
    if (TRAILING_STRING) {
      assert(length_property());
      as->je("POLY_IC_GUARD_OTHER", Xbyak::CodeGenerator::T_NEAR);
    } else {
      as->jne("POLY_IC_GUARD_OTHER", Xbyak::CodeGenerator::T_NEAR);  // we should purge this to string check path
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
      from_object_guard_ = const_cast<uint8_t*>(as->getCode()) + pos;
      Rewrite(from_object_guard_, core::BitCast<uintptr_t>(&stub::LOAD_PROP), k64Size);
    } else {
      // string path
      if (length_property()) {
        const std::size_t pos = GenerateTail(as);
        from_string_guard_ = const_cast<uint8_t*>(as->getCode()) + pos;
        Rewrite(from_string_guard_, core::BitCast<uintptr_t>(main_path()), k64Size);
      } else {
        // use String.prototype object
        JSObject* prototype = ctx->global_data()->GetClassSlot(Class::String).prototype;
        as->mov(as->r8, core::BitCast<uintptr_t>(prototype));
        as->jmp("POLY_IC_START_MAIN", Xbyak::CodeGenerator::T_NEAR);
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
    static const int kSize = 128;

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
      LoadPropertyIC::GenerateFastLoad(as, as->r8, offset_);
    }

   private:
    Map* map_;
    uint32_t offset_;
  };

  class LoadPrototypePropertyCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_PROTOTYPE_PROPERTY;
    static const int kSize = 128;

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
      as->test(as->r11, as->r11);
      as->jz(fail);
      as->cmp(as->r10, as->qword[as->r11 + JSObject::MapOffset()]);
      as->jne(fail);
      // load
      LoadPropertyIC::GenerateFastLoad(as, as->r11, offset_);
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
          as->test(as->r11, as->r11);
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
          as->test(as->r11, as->r11);
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
      as->test(as->r11, as->r11);
      as->jz(fail);
      as->cmp(as->r10, as->qword[as->r11 + JSObject::MapOffset()]);
      as->jne(fail);
      // load
      LoadPropertyIC::GenerateFastLoad(as, as->r11, offset_);
    }

   private:
    Map* map_;
    Chain* chain_;
    uint32_t offset_;
  };

  class LoadArrayLengthCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_ARRAY_LENGTH;
    static const int kSize = 128;
    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      const std::ptrdiff_t offset = IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ClassOffset();
      // check target class is Array
      as->mov(as->r10, core::BitCast<uintptr_t>(JSArray::GetClass()));
      as->cmp(as->r10, as->qword[as->r8 + offset]);
      as->jne(fail);
      // load
      as->mov(as->rax, as->qword[as->r8 + JSArray::LengthOffset()]);
      as->ret();
    }
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

  void LoadArrayLength(Context* ctx) {
    Generate(ctx, LoadArrayLengthCompiler());
  }

  void LoadStringLength(Context* ctx) {
    // inject string length check
    Unit* ic = new Unit(Unit::LOAD_STRING_LENGTH);
    NativeCode::Pages::Buffer buffer = native_code()->pages()->Gain(256);
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);
    const bool generate_guard = empty();

    if (generate_guard) {
      GenerateGuardPrologue<true>(&as);
    }

    // load string length
    const std::ptrdiff_t length_offset =
        IV_CAST_OFFSET(radio::Cell*, JSString*) + JSString::SizeOffset();
    as.mov(as.eax, as.word[as.r8 + length_offset]);
    as.or(as.rax, as.r15);
    as.ret();

    if (empty()) {
      native_code()->assembler()->rewrite(from_original_, core::BitCast<uintptr_t>(as.getCode()), k64Size);
    } else {
      Rewrite(from_string_guard_, core::BitCast<uintptr_t>(as.getCode()), k64Size);
    }
    if (generate_guard) {
      GenerateGuardEpilogue<true>(ctx, &as);
    }
    push_back(*ic);
  }

  NativeCode* native_code() const { return native_code_; }
  uint8_t* main_path() const { return main_path_; }
  Unit* object_chain() const { return object_chain_; }
  bool length_property() const { return name_ == symbol::length(); }
  Symbol name() const { return name_; }

 private:
  // main generation path
  template<typename Generator>
  Unit* Generate(Context* ctx, const Generator& gen) {
    if (size() >= kMaxPolyICSize) {
      return NULL;
    }

    Unit* ic = new Unit(Generator::kType);

    NativeCode::Pages::Buffer buffer = native_code()->pages()->Gain(Generator::kSize);
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);
    const bool generate_guard = empty();

    if (generate_guard) {
      GenerateGuardPrologue<false>(&as);
    }

    as.inLocalLabel();
    gen(this, &as, ".EXIT");
    // fail path
    as.L(".EXIT");
    const std::size_t pos = GenerateTail(&as);
    ic->set_tail(const_cast<uint8_t*>(as.getCode()) + pos);
    as.outLocalLabel();


    ChainingObjectLoad(ic, core::BitCast<uintptr_t>(as.getCode()));
    if (generate_guard) {
      GenerateGuardEpilogue<false>(ctx, &as);
    }
    push_back(*ic);
    object_chain_ = ic;
    assert(as.getSize() <= Generator::kSize);
    return ic;
  }

  void ChainingObjectLoad(Unit* ic, uintptr_t code) {
    if (empty()) {
      // rewrite original
      native_code()->assembler()->rewrite(from_original_, code, k64Size);
    } else {
      if (!object_chain()) {
        // rewrite object guard
        Rewrite(from_object_guard_, code, k64Size);
      } else {
        object_chain()->Redirect(code);
      }
    }

    if ((size() + 1) == kMaxPolyICSize) {
      // last one
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP_GENERIC));
    } else {
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP));
    }
  }

  static void Rewrite(uint8_t* data, uint64_t disp, std::size_t size) {
    for (size_t i = 0; i < size; i++) {
      data[i] = static_cast<uint8_t>(disp >> (i * 8));
    }
  }

  union {
    std::size_t from_original_;
    uint8_t* from_object_guard_;
    uint8_t* from_string_guard_;
  };
  uint8_t* main_path_;
  Unit* object_chain_;
  NativeCode* native_code_;
  Symbol name_;
};

class StorePropertyIC : public PolyIC {
  static const std::size_t kMaxPolyICSize = 5;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_POLY_IC_H_
