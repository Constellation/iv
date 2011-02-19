#ifndef _IV_LV5_SYMBOLTABLE_H_
#define _IV_LV5_SYMBOLTABLE_H_
#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <boost/thread.hpp>
#include "ustring.h"
#include "conversions.h"
#include "lv5/jsstring.h"
#include "lv5/symbol.h"
namespace iv {
namespace lv5 {
class Context;
class SymbolTable {
 public:
  typedef std::vector<core::UString> Strings;
  typedef std::vector<Symbol> Indexes;
  typedef std::tr1::unordered_map<std::size_t, Indexes> Table;
  SymbolTable()
    : sync_(),
      table_(),
      strings_() {
  }

  template<class CharT>
  inline Symbol Lookup(const CharT* str) {
    using std::char_traits;
    return Lookup(core::BasicStringPiece<CharT>(str));
  }

  template<class String>
  inline Symbol Lookup(const String& str) {
    std::size_t hash = StringToHash(str);
    core::UString target(str.begin(), str.end());
    {
      boost::mutex::scoped_lock lock(sync_);
      Table::iterator it = table_.find(hash);
      if (it == table_.end()) {
        const Symbol sym = { strings_.size() };
        strings_.push_back(target);
        Indexes vec(1, sym);
        table_.insert(it, make_pair(hash, vec));
        return sym;
      } else {
        Indexes& vec = it->second;
        for (typename Indexes::const_iterator iit = vec.begin(),
             last = vec.end(); iit != last; ++iit) {
          if (strings_[iit->value_as_index] == target) {
            return *iit;
          }
        }
        const Symbol sym = { strings_.size() };
        strings_.push_back(target);
        vec.push_back(sym);
        return sym;
      }
    }
  }

  template<class String>
  inline Symbol LookupAndCheck(const String& str, bool* found) {
    static const Symbol dummy = {0};
    *found = true;
    std::size_t hash = StringToHash(str);
    core::UString target(str.begin(), str.end());
    {
      boost::mutex::scoped_lock lock(sync_);
      Table::iterator it = table_.find(hash);
      if (it == table_.end()) {
        *found = false;
        return dummy;
      } else {
        Indexes& vec = it->second;
        for (typename Indexes::const_iterator iit = vec.begin(),
             last = vec.end(); iit != last; ++iit) {
          if (strings_[iit->value_as_index] == target) {
            return *iit;
          }
        }
        *found = false;
        return dummy;
      }
    }
  }

  inline JSString* ToString(Context* ctx, Symbol sym) const {
    const core::UString& str = strings_[sym.value_as_index];
    return JSString::New(ctx, str);
  }

  inline const core::UString& GetSymbolString(Symbol sym) const {
    return strings_[sym.value_as_index];
  }

 private:
  boost::mutex sync_;
  Table table_;
  Strings strings_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_SYMBOLTABLE_H_
