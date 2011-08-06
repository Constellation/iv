#ifndef _IV_LV5_RAILGUN_FRAME_H_
#define _IV_LV5_RAILGUN_FRAME_H_
#include <cstddef>
#include <iterator>
#include "utils.h"
#include "static_assert.h"
#include "lv5/jsval.h"
#include "lv5/railgun/fwd.h"
#include "lv5/railgun/code.h"
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
    return reinterpret_cast<JSVal*>(this) + (IV_ROUNDUP(sizeof(Frame), sizeof(JSVal)) / sizeof(JSVal));
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

  JSVal* arguments_begin() {
    return reinterpret_cast<JSVal*>(this) - argc_;
  }

  JSVal* arguments_end() {
    return GetFrameBase();
  }

  reverse_iterator arguments_rbegin() {
    return reverse_iterator(arguments_end());
  }

  reverse_iterator arguments_rend() {
    return reverse_iterator(arguments_begin());
  }

  static std::size_t GetFrameSize(std::size_t n) {
    return n + (IV_ROUNDUP(sizeof(Frame), sizeof(JSVal)) / sizeof(JSVal));
  }

  const Code* code() const {
    return code_;
  }

  Instruction* data() {
    return code_->data();
  }

  const Instruction* data() const {
    return code_->data();
  }

  JSVal* stacktop() {
    return GetStackBase();
  }

  JSVal* locals() {
    return GetLocal();
  }

  JSEnv* lexical_env() {
    return lexical_env_;
  }

  void set_lexical_env(JSEnv* lex) {
    lexical_env_ = lex;
  }

  JSEnv* variable_env() {
    return variable_env_;
  }

  const JSVals& constants() const {
    return code_->constants();
  }

  Code* code_;
  Instruction* prev_pc_;
  JSEnv* variable_env_;
  JSEnv* lexical_env_;
  Frame* prev_;
  JSVal ret_;
  uint16_t argc_;
  uint16_t dynamic_env_level_;
  uint16_t localc_;
  bool constructor_call_;
};

#undef IV_ROUNDUP

} } }  // namespace iv::lv5::railgun
#endif  // _IV_LV5_RAILGUN_FRAME_H_
