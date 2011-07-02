#ifndef _IV_LV5_GLOBAL_DATA_H_
#define _IV_LV5_GLOBAL_DATA_H_
#include <boost/random.hpp>
#include "detail/array.h"
#include "dtoa.h"
#include "conversions.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "ustring.h"
#include "lv5/class.h"
#include "lv5/gc_template.h"
#include "lv5/xorshift.h"
#include "lv5/jsstring.h"
#include "lv5/jsglobal.h"
#include "lv5/jsfunction.h"
#include "lv5/symboltable.h"
namespace iv {
namespace lv5 {

class SymbolChecker;
class JSRegExpImpl;
class Context;

// GlobalData has symboltable, global object
class GlobalData {
 public:
  friend class SymbolChecker;
  friend class Context;
  typedef Xor128 random_engine_type;
  typedef boost::uniform_real<double> random_distribution_type;
  typedef boost::variate_generator<
      random_engine_type, random_distribution_type> random_generator;

  GlobalData()
    : random_engine_(random_engine_type(),
                     random_distribution_type(0, 1)),
      global_obj_(),
      regs_(),
      builtins_(),
      table_(),
      length_symbol_(Intern("length")),
      eval_symbol_(Intern("eval")),
      arguments_symbol_(Intern("arguments")),
      caller_symbol_(Intern("caller")),
      callee_symbol_(Intern("callee")),
      toString_symbol_(Intern("toString")),
      valueOf_symbol_(Intern("valueOf")),
      prototype_symbol_(Intern("prototype")),
      constructor_symbol_(Intern("constructor")),
      Array_symbol_(Intern("Array")) {
    // discard random
    for (std::size_t i = 0; i < 20; ++i) {
      Random();
    }
  }

  Symbol Intern(const core::StringPiece& str) {
    return table_.Lookup(str);
  }

  Symbol Intern(const core::UStringPiece& str) {
    return table_.Lookup(str);
  }

  Symbol InternUInt32(uint32_t index) {
    std::array<char, 15> buf;
    return table_.Lookup(
        core::StringPiece(
            buf.data(),
            snprintf(
                buf.data(), buf.size(), "%lu",
                static_cast<unsigned long>(index))));  // NOLINT
  }

  Symbol InternDouble(double number) {
    std::array<char, 80> buffer;
    const char* const str = core::DoubleToCString(number,
                                                  buffer.data(),
                                                  buffer.size());
    return table_.Lookup(core::StringPiece(str));
  }

  Symbol CheckIntern(const core::StringPiece& str, bool* found) {
    return table_.LookupAndCheck(str, found);
  }

  Symbol CheckIntern(const core::UStringPiece& str, bool* found) {
    return table_.LookupAndCheck(str, found);
  }

  Symbol CheckIntern(uint32_t index, bool* found) {
    std::array<char, 15> buf;
    return table_.LookupAndCheck(
        core::StringPiece(
            buf.data(),
            snprintf(
                buf.data(), buf.size(), "%lu",
                static_cast<unsigned long>(index))), found);  // NOLINT
  }

  Symbol CheckIntern(double number, bool* found) {
    std::array<char, 80> buffer;
    const char* const str = core::DoubleToCString(number,
                                                  buffer.data(),
                                                  buffer.size());
    return table_.LookupAndCheck(core::StringPiece(str), found);
  }

  const core::UString& GetSymbolString(const Symbol& sym) const {
    return table_.GetSymbolString(sym);
  }

  inline const Symbol& length_symbol() const {
    return length_symbol_;
  }

  inline const Symbol& eval_symbol() const {
    return eval_symbol_;
  }

  inline const Symbol& arguments_symbol() const {
    return arguments_symbol_;
  }

  inline const Symbol& caller_symbol() const {
    return caller_symbol_;
  }

  inline const Symbol& callee_symbol() const {
    return callee_symbol_;
  }

  inline const Symbol& toString_symbol() const {
    return toString_symbol_;
  }

  inline const Symbol& valueOf_symbol() const {
    return valueOf_symbol_;
  }

  inline const Symbol& prototype_symbol() const {
    return prototype_symbol_;
  }

  inline const Symbol& constructor_symbol() const {
    return constructor_symbol_;
  }

  inline const Symbol& Array_symbol() const {
    return Array_symbol_;
  }

  double Random() {
    return random_engine_();
  }

  const JSGlobal* global_obj() const {
    return &global_obj_;
  }

  JSGlobal* global_obj() {
    return &global_obj_;
  }

  void RegisterLiteralRegExp(JSRegExpImpl* reg) {
    regs_.push_back(reg);
  }

  template<Class::JSClassType CLS>
  void RegisterClass(const ClassSlot& slot) {
    classes_[CLS] = slot;
  }

  const ClassSlot& GetClassSlot(Class::JSClassType cls) const {
    return classes_[cls];
  }

 private:
  random_generator random_engine_;
  JSGlobal global_obj_;
  trace::Vector<JSRegExpImpl*>::type regs_;
  trace::HashMap<Symbol, Class>::type builtins_;
  SymbolTable table_;
  Symbol length_symbol_;
  Symbol eval_symbol_;
  Symbol arguments_symbol_;
  Symbol caller_symbol_;
  Symbol callee_symbol_;
  Symbol toString_symbol_;
  Symbol valueOf_symbol_;
  Symbol prototype_symbol_;
  Symbol constructor_symbol_;
  Symbol Array_symbol_;
  std::array<ClassSlot, Class::NUM_OF_CLASS> classes_;
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_GLOBAL_DATA_H_
