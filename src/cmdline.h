// Copyright 2010 Constellation <utatane.tea@gmail.com> All rights reserved.
// New BSD Lisence.
// this is modified and created as CmdLine.
// original source is
//   cmdline.h (http://github.com/tanakh/cmdline) (c) Hideyuki Tanaka
// and original lisence is folloing
//
// Copyright (c) 2009, Hideyuki Tanaka
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the <organization> nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#ifndef _IV_CMDLINE_H
#define _IV_CMDLINE_H_

#include <sstream>
#include <iterator>
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <cstring>
#include <cxxabi.h>
#include <cstdlib>
#include <cassert>

namespace iv {
namespace cmdline {
namespace detail {

template <typename Target, typename Source, bool Same>
class lexical_cast_t {
 public:
  static Target cast(const Source &arg) {
    Target ret;
    std::stringstream ss;
    if (!(ss << arg && ss >> ret && ss.eof())) {
      throw std::bad_cast();
    }
    return ret;
  }
};

template <typename Target, typename Source>
class lexical_cast_t<Target, Source, true> {
 public:
  static Target cast(const Source &arg) {
    return arg;
  }
};

template <typename Source>
class lexical_cast_t<std::string, Source, false> {
 public:
  static std::string cast(const Source &arg) {
    std::ostringstream ss;
    ss << arg;
    return ss.str();
  }
};

template <typename Target>
class lexical_cast_t<Target, std::string, false> {
 public:
  static Target cast(const std::string& arg) {
    Target ret;
    std::istringstream ss(arg);
    if (!(ss >> ret && ss.eof())) {
      throw std::bad_cast();
    }
    return ret;
  }
};

template <typename T1, typename T2>
struct is_same {
  static const bool value = false;
};

template <typename T>
struct is_same<T, T> {
  static const bool value = true;
};

template<typename Target, typename Source>
Target lexical_cast(const Source &arg) {
  return lexical_cast_t<Target,
                        Source,
                        detail::is_same<Target, Source>::value>::cast(arg);
}

static inline std::string demangle(const std::string& name) {
  int status = 0;
  char *p = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
  std::string ret(p);
  free(p);
  return ret;
}

template <class T>
std::string readable_typename() {
  return demangle(typeid(T).name());
}

template <>
std::string readable_typename<std::string>() {
  return "string";
}

}  // namespace iv::cmdline::detail

//-----

class cmdline_error : public std::exception {
 public:
  cmdline_error(const std::string& msg) : msg(msg) { }  // NOLINT
  ~cmdline_error() throw() { }
  const char *what() const throw() { return msg.c_str(); }
 private:
  std::string msg;
};

template <class T>
struct default_reader {
  T operator()(const std::string& str) {
    return detail::lexical_cast<T>(str);
  }
};

template <class T>
struct range_reader {
  range_reader(const T& low, const T& high): low(low), high(high) {}
  T operator()(const std::string& s) const {
    T ret = default_reader<T>()(s);
    if (!(ret >= low && ret <= high)) {
      throw cmdline::cmdline_error("range_error");
    }
    return ret;
  }
 private:
  T low, high;
};

template <class T>
range_reader<T> range(const T& low, const T& high) {
  return range_reader<T>(low, high);
}

template <class T>
struct oneof_reader {
  T operator()(const std::string& s) {
    T ret = default_reader<T>()(s);
    if (std::find(alt.begin(), alt.end(), s) == alt.end()) {
      throw cmdline_error("");
    }
    return ret;
  }
  void add(const T& v) { alt.push_back(v); }
 private:
  std::vector<T> alt;
};

template <class T>
oneof_reader<T> oneof(T a1) {
  oneof_reader<T> ret;
  ret.add(a1);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  ret.add(a8);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8, T a9) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  ret.add(a8);
  ret.add(a9);
  return ret;
}

template <class T>
oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5,
                      T a6, T a7, T a8, T a9, T a10) {
  oneof_reader<T> ret;
  ret.add(a1);
  ret.add(a2);
  ret.add(a3);
  ret.add(a4);
  ret.add(a5);
  ret.add(a6);
  ret.add(a7);
  ret.add(a8);
  ret.add(a9);
  ret.add(a10);
  return ret;
}

//-----

class Parser {
 private:
  class option_base {
   public:
    virtual ~option_base() { }

    virtual bool has_value() const = 0;
    virtual bool set() = 0;
    virtual bool set(const std::string& value) = 0;
    virtual bool has_set() const = 0;
    virtual bool valid() const = 0;
    virtual bool must() const = 0;

    virtual const std::string& name() const = 0;
    virtual char short_name() const = 0;
    virtual const std::string& description() const = 0;
    virtual std::string short_description() const = 0;
  };

  class option_without_value : public option_base {
   public:
    option_without_value(const std::string& name,
                         char short_name,
                         const std::string& desc)
      : name_(name),
        short_name_(short_name),
        has_(false),
        description_(desc) {
    }
    ~option_without_value() { }

    bool has_value() const { return false; }

    bool set() {
      return has_ = true;
    }

    bool set(const std::string&) {
      return false;
    }

    bool has_set() const {
      return has_;
    }

    bool valid() const {
      return true;
    }

    bool must() const {
      return false;
    }

    const std::string& name() const {
      return name_;
    }

    char short_name() const {
      return short_name_;
    }

    const std::string& description() const {
      return description_;
    }

    std::string short_description() const {
      return "--"+name_;
    }

   private:
    std::string name_;
    char short_name_;
    bool has_;
    std::string description_;
  };

  template <class T>
  class option_with_value : public option_base {
   public:
    option_with_value(const std::string& name,
                      char short_name,
                      bool need,
                      const T& def,
                      const std::string& desc)
      : name_(name),
        short_name_(short_name),
        need_(need),
        has_(false),
        def_(def) {
      description_ = full_description(desc);
    }

    const T& get() const {
      return def_;
    }

    bool has_value() const { return true; }

    bool set() {
      return false;
    }

    bool set(const std::string& value) {
      try {
        def_ = read(value);
        has_ = true;
      } catch(const std::exception &e) {
        return false;
      }
      return true;
    }

    bool has_set() const {
      return has_;
    }

    bool valid() const {
      return !need_ || has_;
    }

    bool must() const {
      return need_;
    }

    const std::string& name() const {
      return name_;
    }

    char short_name() const {
      return short_name_;
    }

    const std::string& description() const {
      return description_;
    }

    std::string short_description() const {
      return "--"+name_+"="+detail::readable_typename<T>();
    }

   protected:
    std::string full_description(const std::string& desc) {
      return
        desc + " (" + detail::readable_typename<T>() +
        (need_ ? "" : " [="+detail::lexical_cast<std::string>(def_)+"]" )
        +")";
    }

    virtual T read(const std::string& s) = 0;

    std::string name_;
    char short_name_;
    bool need_;
    bool has_;
    T def_;
    std::string description_;
  };

  template <class T>
  class option_with_value_list : public option_base {
   public:
    option_with_value_list(const std::string& name,
                           char short_name,
                           const std::string& desc)
      : name_(name),
        short_name_(short_name),
        has_(false),
        def_() {
      description_ = full_description(desc);
    }

    const std::vector<T>& get() const {
      return def_;
    }

    bool has_value() const { return true; }

    bool set() {
      return false;
    }

    bool set(const std::string& value) {
      try {
        def_.push_back(read(value));
        has_ = true;
      } catch(const std::exception &e) {
        return false;
      }
      return true;
    }

    bool has_set() const {
      return has_;
    }

    bool valid() const {
      return true;
    }

    bool must() const {
      return false;
    }

    const std::string& name() const {
      return name_;
    }

    char short_name() const {
      return short_name_;
    }

    const std::string& description() const {
      return description_;
    }

    std::string short_description() const {
      return "--"+name_+"="+detail::readable_typename<T>();
    }

   protected:
    std::string full_description(const std::string& desc) {
      return desc;
    }

    virtual T read(const std::string& s) = 0;

    std::string name_;
    char short_name_;
    bool has_;
    std::vector<T> def_;
    std::string description_;
  };

  template <class T, class F>
  class option_with_value_with_reader : public option_with_value<T> {
   public:
    option_with_value_with_reader(const std::string& name,
                                  char short_name,
                                  bool need,
                                  const T def,
                                  const std::string& desc,
                                  F reader)
      : option_with_value<T>(name, short_name, need, def, desc),
        reader(reader) {
    }

   private:
    T read(const std::string& s) {
      return reader(s);
    }

    F reader;
  };

  template <class T, class F>
  class option_with_value_list_with_reader : public option_with_value_list<T> {
   public:
    option_with_value_list_with_reader(const std::string& name,
                                       char short_name,
                                       const std::string& desc,
                                       F reader)
      : option_with_value_list<T>(name, short_name, desc),
        reader_(reader) {
    }

   private:
    T read(const std::string& s) {
      return reader_(s);
    }

    F reader_;
  };

 public:
  typedef std::map<std::string, option_base*> OptionMap;
  typedef std::vector<option_base*> OptionVector;

  explicit Parser(const std::string& program_name = "")
     : options_(),
       ordered_(),
       footer_(""),
       program_name_(program_name),
       others_(),
       errors_() {
  }
  ~Parser() {
    for (OptionMap::const_iterator it = options_.begin(),
         end = options_.end();it != end; it++) {
      delete it->second;
    }
  }

  void Add(const std::string& key,
           const std::string& name = "",
           char short_name = 0,
           const std::string& desc = "") {
    assert(options_.count(key) == 0);
    option_base* const ptr = new option_without_value(name, short_name, desc);
    options_[key] = ptr;
    ordered_.push_back(ptr);
  }

  template <class T>
  void Add(const std::string& key,
           const std::string& name = "",
           char short_name = 0,
           const std::string& desc = "",
           bool need = true,
           const T def = T()) {
    Add(key, name, short_name, desc, need, def, default_reader<T>());
  }

  template <class T, class F>
  void Add(const std::string& key,
           const std::string& name = "",
           char short_name = 0,
           const std::string& desc = "",
           bool need = true,
           const T def = T(),
           F reader = F()) {
    assert(options_.count(key) == 0);
    option_base* const ptr =
        new option_with_value_with_reader<T, F>(name, short_name,
                                                need, def, desc, reader);
    options_[key] = ptr;
    ordered_.push_back(ptr);
  }

  template <class T>
  void AddList(const std::string& key,
               const std::string& name = "",
               char short_name = 0,
               const std::string& desc = "") {
    AddList<T>(key, name, short_name, desc, default_reader<T>());
  }

  template <class T, class F>
  void AddList(const std::string& key,
               const std::string& name = "",
               char short_name = 0,
               const std::string& desc = "",
               F reader = F()) {
    assert(options_.count(key) == 0);
    option_base* const ptr =
        new option_with_value_list_with_reader<T, F>(
            name, short_name, desc, reader);
    options_[key] = ptr;
    ordered_.push_back(ptr);
  }

  void set_footer(const std::string& f) {
    footer_ = f;
  }

  void set_program_name(const std::string& name) {
    program_name_ = name;
  }

  bool Exist(const std::string& key) const {
    const OptionMap::const_iterator it = options_.find(key);
    assert(it != options_.end());
    return it->second->has_set();
  }

  template <class T>
  const T& Get(const std::string& key) const {
    const OptionMap::const_iterator it = options_.find(key);
    assert(it != options_.end());
    const option_with_value<T> *p = dynamic_cast<  //NOLINT
        const option_with_value<T>*>(it->second);
    assert(p != NULL);
    return p->get();
  }

  template<typename T>
  const std::vector<T>& GetList(const std::string& key) const {
    const OptionMap::const_iterator it = options_.find(key);
    assert(it != options_.end());
    const option_with_value_list<T> *p = dynamic_cast<  //NOLINT
        const option_with_value_list<T>*>(it->second);
    assert(p != NULL);
    return p->get();
  }

  const std::vector<std::string>& rest() const {
    return others_;
  }

  bool Parse(const std::string& arg) {
    std::vector<std::string> args;

    std::string buf;
    bool in_quote = false;
    for (std::string::const_iterator it = arg.begin(),
         end = arg.end();it != end; ++it) {
      if (*it == '\"') {
        in_quote = !in_quote;
        continue;
      }

      if (*it == ' ' && !in_quote) {
        args.push_back(buf);
        buf.clear();
        continue;
      }

      if (*it == '\\') {
        ++it;
        if (it == end) {
          errors_.push_back("unexpected occurrence of '\\' at end of string");
          return false;
        }
      }

      buf.push_back(*it);
    }

    if (in_quote) {
      errors_.push_back("quote is not closed");
      return false;
    }

    if (!buf.empty()) {
      args.push_back(buf);
    }

    return Parse(args);
  }

  bool Parse(const std::vector<std::string>& args) {
    const std::size_t argc = args.size();
    std::vector<const char*> argv(argc);
    std::transform(args.begin(), args.end(),
                   argv.begin(), mem_fun_ref(&std::string::c_str));
    return Parse(argc, argv.data());
  }

  bool Parse(int argc, const char * const argv[]) {
    typedef std::map<char, option_base*> ShortLookup;
    typedef std::map<std::string, option_base*> LongLookup;
    errors_.clear();
    others_.clear();

    if (argc < 1) {
      errors_.push_back("argument number must be longer than 0");
      return false;
    }

    if (program_name_.empty()) {
      program_name_ = argv[0];
    }

    ShortLookup short_lookup;
    LongLookup long_lookup;
    // set short options to lookup table
    for (OptionMap::const_iterator p = options_.begin(),
         end = options_.end();p != end; ++p) {
      char initial = p->second->short_name();
      if (initial) {
        const ShortLookup::iterator it = short_lookup.find(initial);
        if (it != short_lookup.end()) {
          errors_.push_back(
              std::string("short option '")+initial+"' is ambiguous");
          return false;
        } else {
          short_lookup.insert(it,
                              ShortLookup::value_type(initial, p->second));
        }
      }
      const std::string& name = p->second->name();
      if (!name.empty()) {
        const LongLookup::iterator it = long_lookup.find(name);
        if (it != long_lookup.end()) {
          errors_.push_back("option '"+name+"' is ambiguous");
          return false;
        } else {
          long_lookup.insert(it, LongLookup::value_type(name, p->second));
        }
      }
    }

    for (int i = 1; i < argc; ++i) {
      if (strncmp(argv[i], "--", 2) == 0) {
        const char *p = strchr(argv[i]+2, '=');
        if (p) {
          std::string name(argv[i]+2, p);
          std::string val(p+1);
          const LongLookup::iterator it = long_lookup.find(name);
          if (it == long_lookup.end()) {
            errors_.push_back("undefined option: --"+name);
            continue;
          }
          set_option(it->second, name, val);
        } else {
          std::string name(argv[i]+2);
          const LongLookup::iterator it = long_lookup.find(name);
          if (it == long_lookup.end()) {
            errors_.push_back("undefined option: --"+name);
            continue;
          }
          if (it->second->has_value()) {
            if (i+1 >= argc) {
              errors_.push_back("option needs value: --"+name);
              continue;
            } else {
              ++i;
              set_option(it->second, name, argv[i]);
            }
          } else {
            set_option(it->second, name);
          }
        }
      } else if (strncmp(argv[i], "-", 1) == 0) {
        if (!argv[i][1]) {
          continue;
        }

        char last = argv[i][1];
        for (int j = 2; argv[i][j]; ++j) {
          last = argv[i][j];
          char initial = argv[i][j-1];
          const ShortLookup::iterator it = short_lookup.find(initial);
          if (it == short_lookup.end()) {
            errors_.push_back(
                std::string("undefined short option: -")+initial);
            continue;
          }
          set_option(it->second, initial);
        }

        const ShortLookup::iterator it = short_lookup.find(last);
        if (it == short_lookup.end()) {
          errors_.push_back(std::string("undefined short option: -")+last);
          continue;
        }
        if (i+1 < argc && it->second->has_value()) {
          set_option(it->second, last, argv[i+1]);
          ++i;
        } else {
          set_option(it->second, last);
        }
      } else {
        others_.push_back(argv[i]);
      }
    }

    for (OptionMap::const_iterator p = options_.begin(),
         end = options_.end();p != end; ++p) {
      if (!p->second->valid()) {
        errors_.push_back("need option: --"+std::string(p->first));
      }
    }

    return errors_.empty();
  }

  std::string error() const {
    return errors_.empty() ? "" : errors_[0];
  }

  std::string error_full() const {
    std::ostringstream oss;
    std::copy(errors_.begin(), errors_.end(),
              std::ostream_iterator<std::string>(oss, "\n"));
    return oss.str();
  }

  std::string usage() const {
    std::ostringstream oss;
    oss << "usage: " << program_name_ << " ";

    std::size_t max_width = 0;
    for (OptionVector::const_iterator it = ordered_.begin(),
         end = ordered_.end(); it != end; ++it) {
      if ((*it)->must()) {
        oss << (*it)->short_description() << " ";
      }
      max_width = std::max(max_width, (*it)->name().size());
    }

    oss << "[options] " << footer_ << std::endl
        << "options:"   << std::endl;

    const std::size_t real_width = max_width + 4;
    for (OptionVector::const_iterator it = ordered_.begin(),
         end = ordered_.end(); it != end; ++it) {
      const char short_name = (*it)->short_name();
      if (short_name) {
        oss << "  -" << short_name;
      } else {
        oss << "    ";
      }
      const std::string& name = (*it)->name();
      if (name.empty()) {
        oss << "    ";
      } else {
        if (short_name) {
          oss << ", --";
        } else {
          oss << "  --";
        }
        oss << name;
      }
      oss << std::string(real_width - name.size(), ' ')
          << (*it)->description() << std::endl;
    }
    return oss.str();
  }

 private:
  void set_option(option_base* opt,
                  const std::string& name) {
    if (!opt->set()) {
      errors_.push_back("option needs value: --"+name);
      return;
    }
  }

  void set_option(option_base* opt,
                  const std::string& name, const std::string& value) {
    if (!opt->set(value)) {
      errors_.push_back("option value is invalid: --"+name+"="+value);
      return;
    }
  }

  void set_option(option_base* opt,
                  const char name) {
    if (!opt->set()) {
      errors_.push_back(
          std::string("option needs value: -")+name);
      return;
    }
  }

  void set_option(option_base* opt,
                  const char name, const std::string& value) {
    if (!opt->set(value)) {
      errors_.push_back(
          std::string("option value is invalid: -")+name+"="+value);
      return;
    }
  }

  OptionMap options_;
  OptionVector ordered_;
  std::string footer_;
  std::string program_name_;
  std::vector<std::string> others_;
  std::vector<std::string> errors_;
};

} }  // namespace iv::cmdline
#endif  // _IV_CMDLINE_H_
