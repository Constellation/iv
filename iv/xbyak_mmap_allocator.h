#ifndef IV_XBYAK_MMAP_ALLOCATOR_H_
#define IV_XBYAK_MMAP_ALLOCATOR_H_
#include <cassert>
#include <unordered_map>
#include <iv/third_party/xbyak/xbyak.h>
#include <iv/platform.h>
#include <iv/utils.h>
#include <iv/os_allocator.h>
namespace iv {
namespace core {

class XbyakMMapAllocator : public Xbyak::Allocator {
#if !defined(IV_OS_WIN)
 public:
  static const std::size_t kPageSize = 0x1000;

  virtual uint8_t* alloc(std::size_t size) {
    const std::size_t rounded = IV_ROUNDUP(size, kPageSize);
    void* ptr = OSAllocator::Allocate(rounded);
    if (!ptr) {
      return nullptr;
    }
    memory.insert(std::make_pair(ptr, rounded));
    return reinterpret_cast<uint8_t*>(ptr);
  }

  virtual void free(uint8_t* ptr) {
    if (!ptr) {
      return;
    }
    const auto iter = memory.find(reinterpret_cast<void*>(ptr));
    assert(iter != memory.end());
    OSAllocator::Deallocate(iter->first, iter->second);
    memory.erase(iter);
  }

 private:
  std::unordered_map<void*, std::size_t> memory;
#endif
};

} }  // namespace iv::core
#endif  // IV_XBYAK_MMAP_ALLOCATOR_H_
