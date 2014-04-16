#ifndef IV_STRING_UTILS_H_
#define IV_STRING_UTILS_H_
#include <string>
#include <algorithm>
#include <iv/platform.h>
#if defined(IV_ENABLE_JIT)
#include <iv/third_party/mie/string.hpp>
#endif
namespace iv {
namespace core {

template<typename Iter, typename Iter2>
inline Iter Search(Iter i, Iter iz, Iter2 j, Iter2 jz) {
  return std::search(i, iz, j, jz);
}

#if defined(IV_ENABLE_JIT)
template<>
inline const char* Search<const char*, const char*>(
    const char* i, const char* iz,
    const char* j, const char* jz) {
  static const bool enabled = mie::isAvailableSSE42();
  if (!enabled) {
    return std::search(i, iz, j, jz);
  }
  return mie::findStr(i, iz, j, jz - j);
}
#endif

template<typename char_type>
inline const char_type* FindCharFallback(const char_type* begin,
                                         const char_type* end,
                                         char_type ch) {
  return std::char_traits<char_type>::find(begin, end - begin, ch);
}

template<typename char_type>
inline const char_type* FindChar(const char_type* begin,
                                 const char_type* end,
                                 char_type ch) {
  return FindCharFallback(begin, end, ch);
}

#if defined(IV_ENABLE_JIT)
template<>
inline const char* FindChar<char>(const char* begin,
                                  const char* end,
                                  char ch) {
  static const bool enabled = mie::isAvailableSSE42();
  if (!enabled) {
    return FindCharFallback(begin, end, ch);
  }
  const char* result = mie::findChar(begin, end, ch);
  if (result == end) {
    return nullptr;
  }
  return result;
}
#endif

template<typename char_type>
inline const char_type* FindCharAnyFallback(const char_type* begin,
                                            const char_type* end,
                                            const char_type* key,
                                            size_t key_size) {
  while (begin != end) {
    const char_type c = *begin;
    for (size_t i = 0; i < key_size; i++) {
      if (c == key[i]) {
        return begin;
      }
    }
    begin++;
  }
  return end;
}

template<typename char_type>
inline const char_type* FindCharAny(const char_type* begin,
                                    const char_type* end,
                                    const char_type* key,
                                    size_t key_size) {
  return FindCharAnyFallback(begin, end, key, key_size);
}

#if defined(IV_ENABLE_JIT)
template<>
inline const char* FindCharAny<char>(const char* begin,
                                     const char* end,
                                     const char* key,
                                     size_t key_size) {
  static const bool enabled = mie::isAvailableSSE42();
  if (!enabled) {
    return FindCharAnyFallback(begin, end, key, key_size);
  }
  const char* result = mie::findChar_any(begin, end, key, key_size);
  if (result == end) {
    return nullptr;
  }
  return result;
}
#endif

} }  // namespace iv::core
#endif  // IV_STRING_UTILS_H_
