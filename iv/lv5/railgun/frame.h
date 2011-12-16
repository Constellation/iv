#ifndef IV_LV5_RAILGUN_FRAME_H_
#define IV_LV5_RAILGUN_FRAME_H_
#include <cstddef>
#include <iterator>
#include <iv/utils.h>
#include <iv/static_assert.h>
#include <iv/lv5/jsval.h>
#include <iv/lv5/railgun/fwd.h>
#include <iv/lv5/railgun/code.h>
namespace iv {
namespace lv5 {
namespace railgun {

//
// Frame structure is following
//
// FUNC | THIS | ARG1 | ARG2 | FRAME | LOCALS | STACK |
//
struct Frame {
  typedef JSVal* iterator;
  typedef const JSVal* const_iterator;

  typedef std::iterator_traits<iterator>::value_type value_type;

  typedef std::iterator_traits<iterator>::pointer pointer;
  typedef std::iterator_traits<const_iterator>::pointer const_pointer;
  typedef std::iterator_traits<iterator>::reference reference;
  typedef std::iterator_traits<const_iterator>::reference const_reference;

  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef std::iterator_traits<iterator>::difference_type difference_type;
  typedef std::size_t size_type;

  JSVal* GetFrameEnd() {
    return GetFrameBase() + GetFrameSize(code_->stack_depth());
  }

  JSVal* GetLocal() {
    return reinterpret_cast<JSVal*>(this) +
        (IV_ROUNDUP(sizeof(Frame), sizeof(JSVal)) / sizeof(JSVal));
  }

  JSVal* GetStackBase() {
    return GetLocal() + localc_;
  }

  JSVal* GetPreviousFrameStackTop() {
    return (reinterpret_cast<JSVal*>(this) - (argc_ + 2));
  }

  JSVal* GetFrameBase() {
    return reinterpret_cast<JSVal*>(this);
  }

  const JSVal* GetFrameBase() const {
    return reinterpret_cast<const JSVal*>(this);
  }

  JSVal GetThis() const {
    return this_binding();
  }

  bool IsGlobalFrame() const {
    return prev_ == NULL;
  }

  JSVal this_binding() const {
    return *(reinterpret_cast<const JSVal*>(this) - (argc_ + 1));
  }

  void set_this_binding(JSVal val) {
    *(reinterpret_cast<JSVal*>(this) - (argc_ + 1)) = val;
  }

  iterator arguments_begin() {
    return reinterpret_cast<JSVal*>(this) - argc_;
  }

  const_iterator arguments_begin() const {
    return reinterpret_cast<const JSVal*>(this) - argc_;
  }

  const_iterator arguments_cbegin() const {
    return arguments_begin();
  }

  iterator arguments_end() {
    return GetFrameBase();
  }

  const_iterator arguments_end() const {
    return GetFrameBase();
  }

  const_iterator arguments_cend() const {
    return arguments_end();
  }

  reverse_iterator arguments_rbegin() {
    return reverse_iterator(arguments_end());
  }

  const_reverse_iterator arguments_rbegin() const {
    return const_reverse_iterator(arguments_end());
  }

  const_reverse_iterator arguments_crbegin() const {
    return arguments_rbegin();
  }

  const_reverse_iterator arguments_rend() const {
    return const_reverse_iterator(arguments_begin());
  }

  const_reverse_iterator arguments_crend() const {
    return arguments_rend();
  }

  JSVal GetArg(uint32_t index) const {
    const const_iterator it = arguments_begin() + index;
    if (it < arguments_end()) {
      return *it;
    }
    return JSUndefined;
  }

  JSVal GetConstant(uint32_t index) const {
    return code()->constants()[index];
  }

  Symbol GetName(uint32_t index) const {
    return code()->names()[index];
  }

  static std::size_t GetFrameSize(std::size_t n) {
    return n + (IV_ROUNDUP(sizeof(Frame), sizeof(JSVal)) / sizeof(JSVal));
  }

  const Code* code() const { return code_; }

  Instruction* data() { return code_->data(); }

  const Instruction* data() const { return code_->data(); }

  JSVal* stacktop() { return GetStackBase(); }

  JSVal* locals() { return GetLocal(); }

  JSEnv* lexical_env() { return lexical_env_; }

  void set_lexical_env(JSEnv* lex) { lexical_env_ = lex; }

  JSEnv* variable_env() { return variable_env_; }

  JSVal callee() const { return callee_; }

  Code* code_;
  Instruction* prev_pc_;
  JSEnv* variable_env_;
  JSEnv* lexical_env_;
  Frame* prev_;
  JSVal callee_;
  JSVal ret_;
  uint16_t argc_;
  uint16_t dynamic_env_level_;
  uint16_t localc_;
  bool constructor_call_;
};

} } }  // namespace iv::lv5::railgun
#endif  // IV_LV5_RAILGUN_FRAME_H_
