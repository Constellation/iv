# -*- coding: utf-8 -*-
import sys
import string
import re
category = {
  "Lu": 2,
  "Ll": 3,
  "Lt": 4,
  "Lm": 5,
  "Lo": 6,
  "Mn": 7,
  "Me": 8,
  "Mc": 9,
  "Nd": 10,
  "Nl": 11,
  "No": 12,
  "Zs": 13,
  "Zl": 14,
  "Zp": 15,
  "Cc": 16,
  "Cf": 17,
  "Co": 18,
  "Cs": 19,
  "Pd": 20,
  "Ps": 21,
  "Pe": 22,
  "Pc": 23,
  "Po": 24,
  "Sm": 25,
  "Sc": 26,
  "Sk": 27,
  "So": 28,
  "Pi": 29,
  "Pf": 30
}
data = []
dictionary = {}

def main(source, target):
  with open(target, "w") as out:
    template = """
#ifndef _IV_UCDATA_H_
#define _IV_UCDATA_H_
namespace iv {
namespace core {
namespace detail {
struct UnicodeDataKeyword {
};
template<typename T>
class UnicodeData {
 public:
  static const char kCategory[];
};

template<typename T>
const char UnicodeData<T>::kCategory[%d] = {
  %s
};
}  // namespace iv::core::detail
class UC16 {
 public:
  enum Category {
    kUnassigned = 0,
    kGeneralOtherTypes,
    kUpperCaseLetter,
    kLowerCaseLetter,
    kTitleCaseLetter,
    kModifierLetter,
    kOtherLetter,
    kNonSpacingMark,
    kEnclosingMark,
    kCombiningSpacingMark,
    kDecimalDigitNumber,
    kLetterNumber,
    kOtherNumber,
    kSpaceSeparator,
    kLineSeparator,
    kParagraphSeparator,
    kControlChar,
    kFormatChar,
    kPrivateUseChar,
    kSurrogate,
    kDashPunctuation,
    kStartPunctuation,
    kEndPunctuation,
    kConnectorPunctuation,
    kOtherPunctuation,
    kMathSymbol,
    kCurrencySymbol,
    kModifierSymbol,
    kOtherSymbol,
    kInitialPunctuation,
    kFinalPunctuation,
    kCharCategoryCount
  };
};
typedef detail::UnicodeData<detail::UnicodeDataKeyword> UnicodeData;
} }  // namespace iv::core
#endif  // _IV_UCDATA_H_
""".strip()
    with open(source) as uni:
      flag = False
      first = 0
      for line in uni:
        d = string.split(line.strip(), ";")
        val = int(d[0], 16)
        if flag:
          if re.compile("<.+, Last>").match(d[1]):
            print("LAST", d[1]);
            flag = False
            for t in range(first, val+1):
              dictionary[t] = str(category[d[2]])
          else:
            raise "Database Exception"
        else:
          if re.compile("<.+, First>").match(d[1]):
            print("FIRST", d[1]);
            flag = True
            first = val
          else:
            dictionary[val] = str(category[d[2]])
    for i in range(0xffff+1):
      if dictionary.get(i) == None:
        data.append("0")
      else:
        data.append(dictionary[i])
    out.write(template % (len(data), (",".join(data))))

if __name__ == '__main__':
  main(sys.argv[1], sys.argv[2])
