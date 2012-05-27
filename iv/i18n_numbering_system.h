#ifndef IV_I18N_NUMBERING_SYSTEM_H_
#define IV_I18N_NUMBERING_SYSTEM_H_
#include <iv/detail/array.h>
#include <iv/detail/unordered_map.h>
namespace iv {
namespace core {
namespace i18n {

// Data from i18n draft section 12.3.2 table 2
struct NumberingSystemData {
  const char* name;
  uint16_t mapping[10];
};

typedef std::array<NumberingSystemData, 20> NumberingSystemDataArray;
static const NumberingSystemDataArray kNumberingSystemData = { {
  { "arab",     { 0x0660, 0x0661, 0x0662, 0x0663, 0x0664, 0x0665, 0x0666, 0x0667, 0x0668, 0x0669 } },  // NOLINT
  { "arabext",  { 0x06F0, 0x06F1, 0x06F2, 0x06F3, 0x06F4, 0x06F5, 0x06F6, 0x06F7, 0x06F8, 0x06F9 } },  // NOLINT
  { "beng",     { 0x09E6, 0x09E7, 0x09E8, 0x09E9, 0x09EA, 0x09EB, 0x09EC, 0x09ED, 0x09EE, 0x09EF } },  // NOLINT
  { "deva",     { 0x0966, 0x0967, 0x0968, 0x0969, 0x096A, 0x096B, 0x096C, 0x096D, 0x096E, 0x096F } },  // NOLINT
  { "fullwide", { 0xFF10, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17, 0xFF18, 0xFF19 } },  // NOLINT
  { "gujr",     { 0x0AE6, 0x0AE7, 0x0AE8, 0x0AE9, 0x0AEA, 0x0AEB, 0x0AEC, 0x0AED, 0x0AEE, 0x0AEF } },  // NOLINT
  { "guru",     { 0x0A66, 0x0A67, 0x0A68, 0x0A69, 0x0A6A, 0x0A6B, 0x0A6C, 0x0A6D, 0x0A6E, 0x0A6F } },  // NOLINT
  { "hanidec",  { 0x3007, 0x4E00, 0x4E8C, 0x4E09, 0x56DB, 0x4E94, 0x516D, 0x4E03, 0x516B, 0x4E5D } },  // NOLINT
  { "khmr",     { 0x17E0, 0x17E1, 0x17E2, 0x17E3, 0x17E4, 0x17E5, 0x17E6, 0x17E7, 0x17E8, 0x17E9 } },  // NOLINT
  { "knda",     { 0x0CE6, 0x0CE7, 0x0CE8, 0x0CE9, 0x0CEA, 0x0CEB, 0x0CEC, 0x0CED, 0x0CEE, 0x0CEF } },  // NOLINT
  { "laoo",     { 0x0ED0, 0x0ED1, 0x0ED2, 0x0ED3, 0x0ED4, 0x0ED5, 0x0ED6, 0x0ED7, 0x0ED8, 0x0ED9 } },  // NOLINT
  { "latn",     { 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039 } },  // NOLINT
  { "mlym",     { 0x0D66, 0x0D67, 0x0D68, 0x0D69, 0x0D6A, 0x0D6B, 0x0D6C, 0x0D6D, 0x0D6E, 0x0D6F } },  // NOLINT
  { "mong",     { 0x1810, 0x1811, 0x1812, 0x1813, 0x1814, 0x1815, 0x1816, 0x1817, 0x1818, 0x1819 } },  // NOLINT
  { "mymr",     { 0x1040, 0x1041, 0x1042, 0x1043, 0x1044, 0x1045, 0x1046, 0x1047, 0x1048, 0x1049 } },  // NOLINT
  { "orya",     { 0x0B66, 0x0B67, 0x0B68, 0x0B69, 0x0B6A, 0x0B6B, 0x0B6C, 0x0B6D, 0x0B6E, 0x0B6F } },  // NOLINT
  { "tamldec",  { 0x0BE6, 0x0BE7, 0x0BE8, 0x0BE9, 0x0BEA, 0x0BEB, 0x0BEC, 0x0BED, 0x0BEE, 0x0BEF } },  // NOLINT
  { "telu",     { 0x0C66, 0x0C67, 0x0C68, 0x0C69, 0x0C6A, 0x0C6B, 0x0C6C, 0x0C6D, 0x0C6E, 0x0C6F } },  // NOLINT
  { "thai",     { 0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54, 0x0E55, 0x0E56, 0x0E57, 0x0E58, 0x0E59 } },  // NOLINT
  { "tibt",     { 0x0F20, 0x0F21, 0x0F22, 0x0F23, 0x0F24, 0x0F25, 0x0F26, 0x0F27, 0x0F28, 0x0F29 } },  // NOLINT
} };

class NumberingSystem {
 public:
  enum {
    SENTINEL = 0,  // this is sentinel value
    ARAB,
    ARABEXT,
    BENG,
    DEVA,
    FULLWIDE,
    GUJR,
    GURU,
    HANIDEC,
    KHMR,
    KNDA,
    LAOO,
    LATN,
    MLYM,
    MONG,
    MYMR,
    ORYA,
    TAMLDEC,
    TELU,
    THAI,
    TIBT,
    NUM_OF_NUMBERING_SYSTEM
  };

  typedef uint8_t Type;

  typedef Type Candidates[NUM_OF_NUMBERING_SYSTEM];

  IV_STATIC_ASSERT(NUM_OF_NUMBERING_SYSTEM < UINT8_MAX);

  typedef NumberingSystemData Data;
  typedef std::unordered_map<std::string, const Data*> NumberingSystemMap;  // NOLINT

  static const Data* Lookup(StringPiece name) {
    const NumberingSystemMap::const_iterator it = Map().find(name);
    if (it != Map().end()) {
      return it->second;
    }
    return NULL;
  }

  static const Data* Lookup(Type type) {
    assert(type < NUM_OF_NUMBERING_SYSTEM);
    assert(type != SENTINEL);
    return kNumberingSystemData.data() + (type - 1);
  }

 private:
  static const NumberingSystemMap& Map() {
    static const NumberingSystemMap map = InitMap();
    return map;
  }

  static NumberingSystemMap InitMap() {
    NumberingSystemMap map;
    for (const NumberingSystemData*it = kNumberingSystemData.data(),  // NOLINT
         *last = kNumberingSystemData.data() + kNumberingSystemData.size();
         it != last; ++it) {
      map.insert(std::make_pair(it->name, it));
    }
    return map;
  }
};

} } }  // namespace iv::core::i18n
#endif  // IV_I18N_NUMBERING_SYSTEM_H_
