#ifndef _IV_LV5_GLOBAL_DATA_H_
#define _IV_LV5_GLOBAL_DATA_H_
#include <tr1/array>
#include <tr1/cstdio>
#include <boost/random.hpp>
#include "dtoa.h"
#include "conversions.h"
#include "stringpiece.h"
#include "ustringpiece.h"
#include "ustring.h"
#include "lv5/gc_template.h"
#include "lv5/xorshift.h"
#include "lv5/jsstring.h"
#include "lv5/symboltable.h"
namespace iv {
namespace lv5 {
namespace detail {

static const char* kLengthString = "length";
static const char* kEvalString = "eval";
static const char* kArgumentsString = "arguments";
static const char* kCallerString = "caller";
static const char* kCalleeString = "callee";
static const char* kToStringString = "toString";
static const char* kValueOfString = "valueOf";
static const char* kPrototypeString = "prototype";
static const char* kConstructorString = "constructor";
static const char* kArrayString = "Array";

}  // namespace iv::lv5::detail

class SymbolChecker;
class JSRegExpImpl;

// GlobalData has symboltable, global object
class GlobalData {
 public:
  friend class SymbolChecker;
  typedef Xor128 random_engine_type;
  typedef boost::uniform_real<double> random_distribution_type;
  typedef boost::variate_generator<
      random_engine_type, random_distribution_type> random_generator;

  GlobalData()
    : random_engine_(random_engine_type(),
                     random_distribution_type(0, 1)),
      global_obj_(),
      regs_(),
      table_(),
      length_symbol_(Intern(detail::kLengthString)),
      eval_symbol_(Intern(detail::kEvalString)),
      arguments_symbol_(Intern(detail::kArgumentsString)),
      caller_symbol_(Intern(detail::kCallerString)),
      callee_symbol_(Intern(detail::kCalleeString)),
      toString_symbol_(Intern(detail::kToStringString)),
      valueOf_symbol_(Intern(detail::kValueOfString)),
      prototype_symbol_(Intern(detail::kPrototypeString)),
      constructor_symbol_(Intern(detail::kConstructorString)),
      Array_symbol_(Intern(detail::kArrayString)) {
  }

  Symbol Intern(const core::StringPiece& str) {
    return table_.Lookup(str);
  }

  Symbol Intern(const core::UStringPiece& str) {
    return table_.Lookup(str);
  }

  Symbol InternIndex(uint32_t index) {
    std::tr1::array<char, 15> buf;
    return table_.Lookup(
        core::StringPiece(
            buf.data(),
            snprintf(
                buf.data(), buf.size(), "%lu",
                static_cast<unsigned long>(index))));  // NOLINT
  }

  Symbol InternDouble(double number) {
    std::tr1::array<char, 80> buffer;
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
    std::tr1::array<char, 15> buf;
    return table_.LookupAndCheck(
        core::StringPiece(
            buf.data(),
            snprintf(
                buf.data(), buf.size(), "%lu",
                static_cast<unsigned long>(index))), found);  // NOLINT
  }

  Symbol CheckIntern(double number, bool* found) {
    std::tr1::array<char, 80> buffer;
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

  const JSObject* global_obj() const {
    return &global_obj_;
  }

  JSObject* global_obj() {
    return &global_obj_;
  }

  void RegisterLiteralRegExp(JSRegExpImpl* reg) {
    regs_.push_back(reg);
  }
 private:
  random_generator random_engine_;
  JSObject global_obj_;
  trace::Vector<JSRegExpImpl*>::type regs_;
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
};

} }  // namespace iv::lv5
#endif  // _IV_LV5_GLOBAL_DATA_H_
