#ifndef _IV_LV5_SYMBOLTABLE_H_
#define _IV_LV5_SYMBOLTABLE_H_
#include <string>
#include <vector>
#include <tr1/unordered_map>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include "jsstring.h"
#include "symbol.h"
#include "ustring.h"
#include "conversions-inl.h"
namespace iv {
namespace lv5 {
class Context;
class SymbolTable {
 public:
  typedef std::vector<core::UString> Strings;
  typedef std::vector<std::size_t> Indexes;
  typedef std::tr1::unordered_map<std::size_t, Indexes> Table;
  SymbolTable();
  JSString* ToString(Context* ctx, Symbol sym) const;

  template<class CharT>
  Symbol Lookup(const CharT* str) {
    using std::char_traits;
    return Lookup(core::BasicStringPiece<CharT>(str));
  }

  template<class String>
  Symbol Lookup(const String& str) {
    std::size_t hash = StringToHash(str);
    core::UString target(str.begin(), str.end());
    {
      boost::mutex::scoped_lock lock(sync_);
      Table::iterator it = table_.find(hash);
      if (it == table_.end()) {
        Symbol sym = strings_.size();
        strings_.push_back(target);
        Indexes vec(1, sym);
        table_.insert(it, make_pair(hash, vec));
        return sym;
      } else {
        Indexes& vec = it->second;
        BOOST_FOREACH(const std::size_t& i, vec) {
          if (strings_[i] == target) {
            return i;
          }
        }
        Symbol sym = strings_.size();
        strings_.push_back(target);
        vec.push_back(sym);
        return sym;
      }
    }
  }
 private:
  boost::mutex sync_;
  Table table_;
  Strings strings_;
};
} }  // namespace iv::lv5
#endif  // _IV_LV5_SYMBOLTABLE_H_
