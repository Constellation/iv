#ifndef IV_PLATFORM_IO_H_
#define IV_PLATFORM_IO_H_
#include <iv/utils.h>
#include <iv/stringpiece.h>
namespace iv {
namespace core {
namespace io {

inline std::FILE* OpenFile(const std::string& filename,
                           const std::string& flags) {
#if defined(IV_OS_WIN)
  std::FILE* result = nullptr;
  const errno_t err = fopen_s(&result, filename.c_str(), flags.c_str());
  if (err != 0) {
    return nullptr;
  }
  return result;
#else
  return std::fopen(filename.c_str(), flags.c_str());
#endif
}

inline bool io::ReadFile(core::StringPiece filename,
                     std::vector<char>* out, bool output_error = true) {
  if (std::FILE* fp = OpenFile(filename, "rb")) {
    std::fseek(fp, 0L, SEEK_END);
    const std::size_t filesize = std::ftell(fp);
    if (filesize) {
      std::rewind(fp);
      const std::size_t offset = out->size();
      out->resize(offset + filesize);
      if (std::fread(out->data() + offset, filesize, 1, fp) < 1) {
        const std::string err = "lv5 can't read \"" + filename + "\"";
        std::perror(err.c_str());
        std::fclose(fp);
        return false;
      }
    }
    std::fclose(fp);
    return true;
  } else {
    if (output_error) {
      const std::string err = "lv5 can't open \"" + filename + "\"";
      std::perror(err.c_str());
    }
    return false;
  }
}

} } }  // namespace iv::core::io
#endif  // IV_PLATFORM_IO_H_
