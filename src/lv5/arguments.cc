#include "interpreter.h"
#include "context.h"
#include "arguments.h"
namespace iv {
namespace lv5 {
Arguments::Arguments(Context* ctx)
  : ctx_(ctx),
    this_binding_(),
    args_() {
}

Arguments::Arguments(Context* ctx,
                     const JSVal& this_binding)
  : ctx_(ctx),
    this_binding_(this_binding),
    args_() {
}

Arguments::Arguments(Context* ctx,
                     std::size_t n)
  : ctx_(ctx),
    this_binding_(),
    args_(n) {
}

Interpreter* Arguments::interpreter() const {
  return ctx_->interp();
}

Context* Arguments::ctx() const {
  return ctx_;
}

} }  // namespace iv::lv5
