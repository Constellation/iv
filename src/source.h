#ifndef _IV_SOURCE_H_
#define _IV_SOURCE_H_
#include <cstddef>
#include <cassert>
#include <string>
#include "ustring.h"
#include "ustringpiece.h"

namespace iv {
namespace core {

class BasicSource {
 public:
  static const int kEOS = -1;
  virtual ~BasicSource() = 0;

  virtual uc16 Get(std::size_t pos) const = 0;
  virtual std::size_t size() const = 0;
  virtual const std::string& filename() const = 0;
  virtual const UString& source() const = 0;
  virtual UStringPiece SubString(std::size_t n,
                                 std::size_t len = std::string::npos) const = 0;
};

inline BasicSource::~BasicSource() { }

} }  // namespace iv::core
#endif  // _IV_SOURCE_H_
