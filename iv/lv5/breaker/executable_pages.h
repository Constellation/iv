// This page is used in PolyIC or other stubs
#ifndef IV_BREAKER_EXECUTABLE_PAGES_H_
#define IV_BREAKER_EXECUTABLE_PAGES_H_
#include <vector>
#include <new>
#include <iv/detail/array.h>
#include <iv/functor.h>
#include <iv/lv5/breaker/fwd.h>
namespace iv {
namespace lv5 {
namespace breaker {

template<std::size_t PageSize = 4096>
class ExecutablePages {
 public:
  class Page : protected std::array<char, PageSize> {
   public:
    typedef std::array<char, PageSize> container_type;
    using container_type::size;
    using container_type::data;

    Page() {
      Xbyak::CodeArray::protect(data(), size(), true);
    }

    ~Page() {
      Xbyak::CodeArray::protect(data(), size(), false);
    }
  };

  typedef std::vector<Page*> Pages;

  ExecutablePages()
    : pages_(),
      cursor_(PageSize) {
  }

  ~ExecutablePages() {
    for (typename Pages::iterator it = pages_.begin(),
         last = pages_.end(); it != last; ++it) {
      Page* page = *it;
      page->~Page();
      Xbyak::AlignedFree(page);
    }
  }

  struct Buffer {
    char* ptr;
    std::size_t size;
  };

  Buffer Gain(std::size_t reserve) {
    assert(reserve <= PageSize);

    Buffer buffer = {
      NULL,
      reserve
    };

    if ((cursor() + reserve) <= PageSize) {
      buffer.ptr = pages_.back()->data() + cursor();
      cursor_ += reserve;
      return buffer;
    }

    void* mem = Xbyak::AlignedMalloc(PageSize, PageSize);
    Page* page = new(mem)Page;
    pages_.push_back(page);
    buffer.ptr = page->data();
    cursor_ = reserve;
    return buffer;
  }

 private:
  std::size_t cursor() const { return cursor_; }

  Pages pages_;
  std::size_t cursor_;
};

} } }  // namespace iv::lv5::breaker
#endif  // IV_BREAKER_EXECUTABLE_PAGES_H_
