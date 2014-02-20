// diagram::Compiler
//
// This compiler parses railgun::opcodes, inlines functions, and optimizes them
// with LLVM.

#include <llvm/IR/LLVMContext.h>
