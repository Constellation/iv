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
    LOAD_ARRAY_LENGTH,
    STORE_REPLACE_PROPERTY,
    STORE_REPLACE_PROPERTY_WITH_MAP_TRANSITION,
    STORE_NEW_PROPERTY,
    STORE_NEW_PROPERTY_WITH_REALLOCATION,
    STORE_NEW_ELEMENT,
    LOAD_ARRAY_HOLE
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
  uint8_t* tail() const { return tail_; }
  void set_tail(uint8_t* tail) { tail_ = tail; }

  void Redirect(uintptr_t ptr) { IC::Rewrite(tail(), ptr, k64Size); }

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

  static bool CanEmitDirectCall(Xbyak::CodeGenerator* as, const void* addr) {
    return Xbyak::inner::IsInInt32(reinterpret_cast<const uint8_t*>(addr) - as->getCurr());
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

  LoadPropertyIC(NativeCode* native_code, Symbol name)
    : from_original_(),
      native_code_(native_code),
      name_(name) {
  }

  void BindOriginal(std::size_t from_original) {
    from_original_ = from_original;
    native_code()->assembler()->rewrite(from_original_, core::BitCast<uintptr_t>(&stub::LOAD_PROP), k64Size);
  }

  // load to rax
  static void GenerateFastLoad(Xbyak::CodeGenerator* as,
                               const Xbyak::Reg64& reg, uint32_t offset) {
    const std::ptrdiff_t data_offset =
        JSObject::SlotsOffset() +
        JSObject::Slots::DataOffset();
    as->mov(rax, qword[reg + data_offset]);
    as->mov(rax, qword[rax + kJSValSize * offset]);
    as->ret();
  }

  class LoadOwnPropertyCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_OWN_PROPERTY;
    static const int kSize = 128;

    std::size_t size() const { return kSize; }

    LoadOwnPropertyCompiler(Map* map, uint32_t offset)
      : map_(map),
        offset_(offset) {
    }

    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      // own map guard
      IC::TestMapConstant(as, map_, r8, r10, fail);
      // load
      LoadPropertyIC::GenerateFastLoad(as, r8, offset_);
    }

   private:
    Map* map_;
    uint32_t offset_;
  };

  class LoadPrototypePropertyCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_PROTOTYPE_PROPERTY;
    static const int kSize = 128;

    std::size_t size() const { return kSize; }

    LoadPrototypePropertyCompiler(Map* map, Map* prototype, uint32_t offset)
      : map_(map),
        prototype_(prototype),
        offset_(offset) {
    }

    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      // own map guard
      IC::TestMapConstant(as, map_, r8, r10, fail);
      // prototype map guard
      as->mov(r11, core::BitCast<uintptr_t>(map_->prototype()));
      IC::TestMapConstant(as, prototype_, r11, r10, fail);
      // load
      LoadPropertyIC::GenerateFastLoad(as, r11, offset_);
    }

   private:
    Map* map_;
    Map* prototype_;
    uint32_t offset_;
  };

  class LoadChainPropertyCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_CHAIN_PROPERTY;
    static const int kSize = 64;

    std::size_t size() const { return kSize * (chain_->size() + 1); }

    LoadChainPropertyCompiler(Chain* chain, Map* last, uint32_t offset)
      : map_(last),
        chain_(chain),
        offset_(offset) {
    }

    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      JSObject* prototype = NULL;
      as->mov(r11, r8);
      for (Chain::const_iterator it = chain_->begin(),
           last = chain_->end(); it != last; ++it) {
        Map* map = *it;
        IC::TestMapConstant(as, map, r11, r10, fail, Xbyak::CodeGenerator::T_NEAR);
        prototype = map->prototype();
        as->mov(r11, core::BitCast<uintptr_t>(prototype));
      }
      // last check
      IC::TestMapConstant(as, map_, r11, r10, fail, Xbyak::CodeGenerator::T_NEAR);
      // load
      LoadPropertyIC::GenerateFastLoad(as, r11, offset_);
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

    std::size_t size() const { return kSize; }
    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      const std::ptrdiff_t offset = IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ClassOffset();
      static const uintptr_t cls = core::BitCast<uintptr_t>(JSArray::GetClass());
      // check target class is Array
      helper::CmpConstant(as, qword[r8 + offset], cls, r10);
      as->jne(fail);
      // load
      const std::size_t length = JSObject::ElementsOffset() + IndexedElements::LengthOffset();
      as->mov(eax, word[r8 + length]);
      as->cmp(eax, INT32_MAX);
      as->ja(fail);
      as->or(rax, r15);
      as->ret();
    }
  };

  class LoadStringLengthCompiler {
   public:
    static const Unit::Type kType = Unit::LOAD_STRING_LENGTH;
    static const int kSize = 128;

    std::size_t size() const { return kSize; }

    LoadStringLengthCompiler(Map* map)
      : map_(map) {
    }

    void operator()(LoadPropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      // own map guard
      IC::TestMapConstant(as, map_, r8, r10, fail);
      // load string length
      const std::ptrdiff_t length_offset =
          IV_CAST_OFFSET(radio::Cell*, JSString*) + JSString::SizeOffset();
      as->mov(eax, word[r8 + length_offset]);
      as->or(rax, r15);
      as->ret();
    }

   private:
    Map* map_;
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
    Generate(ctx, LoadStringLengthCompiler(ctx->global_data()->primitive_string_map()));
  }

  NativeCode* native_code() const { return native_code_; }
  bool length_property() const { return name_ == symbol::length(); }
  Symbol name() const { return name_; }

 private:
  void GenerateGuardPrologue(Xbyak::CodeGenerator* as) {
    // check target is Cell
    helper::TestConstant(as, rsi, detail::jsval64::kValueMask, r10);
    as->jnz("POLY_IC_GUARD_GENERIC", Xbyak::CodeGenerator::T_NEAR);
    // target is guaranteed as cell
    as->mov(r8, rsi);
  }

  void GenerateGuardEpilogue(Xbyak::CodeGenerator* as) {
    // They are used as last entry
    const uintptr_t call = core::BitCast<uintptr_t>(&stub::LOAD_PROP_GENERIC);
    as->L("POLY_IC_GUARD_GENERIC");
    as->mov(rax, call);
    as->jmp(rax);
  }

  // main generation path
  template<typename Generator>
  Unit* Generate(Context* ctx, const Generator& gen) {
    if (size() >= kMaxPolyICSize) {
      return NULL;
    }

    Unit* ic = new Unit(Generator::kType);

    NativeCode::Pages::Buffer buffer = native_code()->pages()->Gain(gen.size());
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);
    const bool generate_guard = empty();

    if (generate_guard) {
      GenerateGuardPrologue(&as);
    }

    as.inLocalLabel();
    gen(this, &as, ".EXIT");
    // fail path
    as.L(".EXIT");
    const std::size_t pos = GenerateTail(&as);
    ic->set_tail(const_cast<uint8_t*>(as.getCode()) + pos);
    as.outLocalLabel();

    Chaining(ic, core::BitCast<uintptr_t>(as.getCode()));
    if (generate_guard) {
      GenerateGuardEpilogue(&as);
    }
    push_back(*ic);
    assert(as.getSize() <= gen.size());
    return ic;
  }

  void Chaining(Unit* ic, uintptr_t code) {
    if (empty()) {
      // rewrite original
      native_code()->assembler()->rewrite(from_original_, code, k64Size);
    } else {
      back().Redirect(code);
    }

    if ((size() + 1) == kMaxPolyICSize) {
      // last one
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP_GENERIC));
    } else {
      ic->Redirect(core::BitCast<uintptr_t>(&stub::LOAD_PROP));
    }
  }

  std::size_t from_original_;
  NativeCode* native_code_;
  Symbol name_;
};

class StorePropertyIC : public PolyIC {
 public:
  static const std::size_t kMaxPolyICSize = 5;

  StorePropertyIC(NativeCode* native_code, Symbol name, bool strict)
    : from_original_(),
      native_code_(native_code),
      name_(name),
      strict_(strict) {
  }

  void BindOriginal(std::size_t from_original) {
    from_original_ = from_original;
    const uintptr_t call = core::BitCast<uintptr_t>(&stub::STORE_PROP);
    native_code()->assembler()->rewrite(from_original_, call, k64Size);
  }

  static void GenerateFastStore(Xbyak::CodeGenerator* as, const Xbyak::Reg64& reg, uint32_t offset) {
    const std::ptrdiff_t data_offset =
        JSObject::SlotsOffset() +
        JSObject::Slots::DataOffset();
    as->mov(rax, qword[reg + data_offset]);
    as->mov(qword[rax + kJSValSize * offset], rdx);
    as->ret();
  }

  class StoreReplacePropertyCompiler {
   public:
    static const Unit::Type kType = Unit::STORE_REPLACE_PROPERTY;
    static const int kSize = 128;

    std::size_t size() const { return kSize; }

    StoreReplacePropertyCompiler(Map* map, uint32_t offset)
      : map_(map),
        offset_(offset) {
    }

    void operator()(StorePropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      // own map guard
      IC::TestMapConstant(as, map_, rsi, r10, fail);
      // store
      StorePropertyIC::GenerateFastStore(as, rsi, offset_);
    }

   private:
    Map* map_;
    uint32_t offset_;
  };

  class StoreReplacePropertyWithMapTransitionCompiler {
   public:
    static const Unit::Type kType = Unit::STORE_REPLACE_PROPERTY_WITH_MAP_TRANSITION;
    static const int kSize = 128;

    std::size_t size() const { return kSize; }

    StoreReplacePropertyWithMapTransitionCompiler(Map* map, Map* transit, uint32_t offset)
      : map_(map),
        transit_(transit),
        offset_(offset) {
    }

    void operator()(StorePropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      // own map guard
      IC::TestMapConstant(as, map_, rsi, r10, fail);
      // transition
      helper::MovConstant(as, qword[rsi + JSObject::MapOffset()], core::BitCast<uintptr_t>(transit_), r10);
      // store
      StorePropertyIC::GenerateFastStore(as, rsi, offset_);
    }

   private:
    Map* map_;
    Map* transit_;
    uint32_t offset_;
  };

  class StoreNewPropertyCompiler {
   public:
    static const Unit::Type kType = Unit::STORE_NEW_PROPERTY;
    static const int kSize = 64;

    std::size_t size() const { return kSize * (chain_->size() + 1); }

    StoreNewPropertyCompiler(Chain* chain, Map* transit, uint32_t offset)
      : chain_(chain),
        transit_(transit),
        offset_(offset) {
    }

    void operator()(StorePropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      JSObject* prototype = NULL;
      as->mov(r11, rsi);
      for (Chain::const_iterator it = chain_->begin(),
           last = chain_->end(); it != last; ++it) {
        Map* map = *it;
        IC::TestMapConstant(as, map, r11, r10, fail, Xbyak::CodeGenerator::T_NEAR);
        prototype = map->prototype();
        as->mov(r11, core::BitCast<uintptr_t>(prototype));
      }
      assert(prototype == NULL);  // last is NULL
      // transition
      helper::MovConstant(as, qword[rsi + JSObject::MapOffset()], core::BitCast<uintptr_t>(transit_), r10);
      // store
      StorePropertyIC::GenerateFastStore(as, rsi, offset_);
    }

   private:
    Chain* chain_;
    Map* transit_;
    uint32_t offset_;
  };

  class StoreNewPropertyWithReallocationCompiler {
   public:
    static const Unit::Type kType = Unit::STORE_NEW_PROPERTY_WITH_REALLOCATION;
    static const int kSize = 64;

    std::size_t size() const { return kSize * (chain_->size() + 1); }

    StoreNewPropertyWithReallocationCompiler(Chain* chain, Map* transit, uint32_t offset)
      : chain_(chain),
        transit_(transit),
        offset_(offset) {
    }

    void operator()(StorePropertyIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      JSObject* prototype = NULL;
      as->mov(r11, rsi);
      for (Chain::const_iterator it = chain_->begin(),
           last = chain_->end(); it != last; ++it) {
        Map* map = *it;
        IC::TestMapConstant(as, map, r11, r10, fail, Xbyak::CodeGenerator::T_NEAR);
        prototype = map->prototype();
        as->mov(r11, core::BitCast<uintptr_t>(prototype));
      }
      assert(prototype == NULL);  // last is NULL
      // call
      as->mov(rdi, rsi);
      as->mov(rsi, rdx);
      as->mov(rdx, core::BitCast<uintptr_t>(transit_));
      as->mov(ecx, offset_);
      as->mov(rax, core::BitCast<uintptr_t>(&JSObject::MapTransitionWithReallocation));
      as->jmp(rax);
    }

   private:
    Chain* chain_;
    Map* transit_;
    uint32_t offset_;
  };

  void StoreReplaceProperty(Map* map, uint32_t offset) {
    if (Unit* ic = Generate(StoreReplacePropertyCompiler(map, offset))) {
      ic->set_own(map);
    }
  }

  void StoreReplacePropertyWithMapTransition(Map* map, Map* transit, uint32_t offset) {
    if (Unit* ic = Generate(StoreReplacePropertyWithMapTransitionCompiler(map, transit, offset))) {
      ic->set_own(map);
      ic->set_proto(transit);
    }
  }

  void StoreNewProperty(Chain* chain, Map* transit, uint32_t offset) {
    if (Unit* ic = Generate(StoreNewPropertyCompiler(chain, transit, offset))) {
      ic->set_own(transit);
      ic->set_chain(chain);
    }
  }

  void StoreNewPropertyWithReallocation(Chain* chain, Map* transit, uint32_t offset) {
    if (Unit* ic = Generate(StoreNewPropertyWithReallocationCompiler(chain, transit, offset))) {
      ic->set_own(transit);
      ic->set_chain(chain);
    }
  }

  NativeCode* native_code() const { return native_code_; }
  bool strict() const { return strict_; }
  Symbol name() const { return name_; }

 private:
  void GenerateGuardPrologue(Xbyak::CodeGenerator* as) {
    // check target is Cell
    helper::TestConstant(as, rsi, detail::jsval64::kValueMask, r10);
    as->jnz("POLY_IC_GUARD_GENERIC", Xbyak::CodeGenerator::T_NEAR);
  }

  void GenerateGuardEpilogue(Xbyak::CodeGenerator* as) {
    // They are used as last entry
    const uintptr_t call = core::BitCast<uintptr_t>(&stub::STORE_PROP_GENERIC);
    as->L("POLY_IC_GUARD_GENERIC");
    as->mov(rax, call);
    as->jmp(rax);
  }

  template<typename Generator>
  Unit* Generate(const Generator& gen) {
    if (size() >= kMaxPolyICSize) {
      return NULL;
    }

    Unit* ic = new Unit(Generator::kType);

    NativeCode::Pages::Buffer buffer = native_code()->pages()->Gain(gen.size());
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);
    const bool generate_guard = empty();

    if (generate_guard) {
      GenerateGuardPrologue(&as);
    }

    as.inLocalLabel();
    gen(this, &as, ".EXIT");
    // fail path
    as.L(".EXIT");
    const std::size_t pos = GenerateTail(&as);
    ic->set_tail(const_cast<uint8_t*>(as.getCode()) + pos);
    as.outLocalLabel();

    Chaining(ic, core::BitCast<uintptr_t>(as.getCode()));
    if (generate_guard) {
      GenerateGuardEpilogue(&as);
    }
    push_back(*ic);
    assert(as.getSize() <= gen.size());
    return ic;
  }

  void Chaining(Unit* ic, uintptr_t code) {
    if (empty()) {
      // rewrite original
      native_code()->assembler()->rewrite(from_original_, code, k64Size);
    } else {
      back().Redirect(code);
    }

    if ((size() + 1) == kMaxPolyICSize) {
      // last one
      ic->Redirect(core::BitCast<uintptr_t>(&stub::STORE_PROP_GENERIC));
    } else {
      ic->Redirect(core::BitCast<uintptr_t>(&stub::STORE_PROP));
    }
  }

  std::size_t from_original_;
  NativeCode* native_code_;
  Symbol name_;
  bool strict_;
};

class StoreElementIC : public PolyIC {
 public:
  static const std::size_t kMaxPolyICSize = 5;

  StoreElementIC(NativeCode* native_code, bool strict, uint32_t index = UINT32_MAX)
    : from_original_(),
      native_code_(native_code),
      index_(index),
      strict_(strict) {
  }

  void BindOriginal(std::size_t from_original) {
    from_original_ = from_original;
    const uintptr_t call = core::BitCast<uintptr_t>(&stub::STORE_ELEMENT_INDEXED);
    native_code()->assembler()->rewrite(from_original_, call, k64Size);
  }

  void StoreNewElement(Chain* chain) {
    if (Unit* ic = Generate(StoreNewElementCompiler(chain))) {
      ic->set_chain(chain);
    }
  }

  void Invalidate() {
  }

  NativeCode* native_code() const { return native_code_; }
  bool strict() const { return strict_; }
  uint32_t index() const { return index_; }

 private:
  class StoreNewElementCompiler {
   public:
    static const Unit::Type kType = Unit::STORE_NEW_ELEMENT;
    static const int kSize = 64;

    std::size_t size() const { return kSize * (chain_->size() + 1); }

    StoreNewElementCompiler(Chain* chain)
      : chain_(chain) {
    }

    void operator()(StoreElementIC* site, Xbyak::CodeGenerator* as, const char* fail) const {
      JSObject* prototype = NULL;
      as->mov(r9, rsi);
      for (Chain::const_iterator it = chain_->begin(),
           last = chain_->end(); it != last; ++it) {
        Map* map = *it;
        IC::TestMapConstant(as, map, r9, r10, fail, Xbyak::CodeGenerator::T_NEAR);
        prototype = map->prototype();
        as->mov(r9, core::BitCast<uintptr_t>(prototype));
      }

      // pass

      // length grow check
      as->test(r11d, r11d);
      as->jnz(".GROW");

      // store path
      as->L(".MAIN");
      const std::ptrdiff_t vector_offset =
          IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ElementsOffset() + IndexedElements::VectorOffset();
      const std::ptrdiff_t data_offset =
          vector_offset + IndexedElements::DenseArrayVector::DataOffset();
      as->mov(rax, qword[rsi + data_offset]);
      if (site->index() != UINT32_MAX) {
        as->mov(qword[rax + site->index() * kJSValSize], rdx);
      } else {
        // rcx is index
        as->mov(qword[rax + rcx * kJSValSize], rdx);
      }
      as->ret();

      as->L(".GROW");
      const std::ptrdiff_t size_offset =
          vector_offset + IndexedElements::DenseArrayVector::SizeOffset();
      const std::ptrdiff_t length_offset =
          IV_CAST_OFFSET(radio::Cell*, JSObject*) + JSObject::ElementsOffset() + IndexedElements::LengthOffset();
      if (site->index() != UINT32_MAX) {
        as->mov(qword[rsi + size_offset], site->index() + 1);
        as->cmp(dword[rsi + length_offset], site->index() + 1);
        as->jae(".END");
        as->mov(dword[rsi + length_offset], site->index() + 1);
        as->L(".END");
      } else {
        as->inc(ecx);
        as->mov(qword[rsi + size_offset], rcx);
        as->cmp(dword[rsi + length_offset], ecx);
        as->jae(".END");
        as->mov(dword[rsi + length_offset], ecx);
        as->L(".END");
        as->sub(ecx, 1);
      }
      as->jmp(".MAIN");
    }

   private:
    Chain* chain_;
  };

  template<typename Generator>
  Unit* Generate(const Generator& gen) {
    if (size() >= kMaxPolyICSize) {
      return NULL;
    }

    Unit* ic = new Unit(Generator::kType);

    NativeCode::Pages::Buffer buffer = native_code()->pages()->Gain(gen.size());
    Xbyak::CodeGenerator as(buffer.size, buffer.ptr);

    as.inLocalLabel();
    gen(this, &as, ".EXIT");
    // fail path
    as.L(".EXIT");
    const std::size_t pos = GenerateTail(&as);
    ic->set_tail(const_cast<uint8_t*>(as.getCode()) + pos);
    as.outLocalLabel();

    Chaining(ic, core::BitCast<uintptr_t>(as.getCode()));
    push_back(*ic);
    assert(as.getSize() <= gen.size());
    return ic;
  }

  void Chaining(Unit* ic, uintptr_t code) {
    if (empty()) {
      // rewrite original
      native_code()->assembler()->rewrite(from_original_, code, k64Size);
    } else {
      back().Redirect(code);
    }

    if ((size() + 1) == kMaxPolyICSize) {
      // last one
      ic->Redirect(core::BitCast<uintptr_t>(&stub::STORE_ELEMENT_GENERIC));
    } else {
      ic->Redirect(core::BitCast<uintptr_t>(&stub::STORE_ELEMENT_INDEXED));
    }
  }

  std::size_t from_original_;
  NativeCode* native_code_;
  uint32_t index_;
  bool strict_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_POLY_IC_H_
