#include "interpreter.h"
#include "context.h"
#include "arguments.h"
namespace iv {
namespace lv5 {
Arguments::Arguments(Context* ctx)
  : context_(ctx),
    this_binding_(),
    args_() {
}

Arguments::Arguments(Context* ctx,
                     const JSVal& this_binding)
  : context_(ctx),
    this_binding_(this_binding),
    args_() {
}

Arguments::Arguments(Context* ctx,
                     std::size_t n)
  : context_(ctx),
    this_binding_(),
    args_(n) {
}

Interpreter* Arguments::interpreter() const {
  return context_->interp();
}

Context* Arguments::context() const {
  return context_;
}

} }  // namespace iv::lv5
