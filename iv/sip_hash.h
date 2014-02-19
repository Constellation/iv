#ifndef IV_SIP_HASH_H_
#define IV_SIP_HASH_H_
#include <cstring>
#include <cstdlib>
#include <iv/detail/cstdint.h>
#include <iv/detail/cinttypes.h>
#include <iv/byteorder.h>
namespace iv {
namespace core {

#define IV_U8TO32_LE(p)\
    (((uint32_t)((p)[0])       ) | ((uint32_t)((p)[1]) <<  8) |\
     ((uint32_t)((p)[2]) <<  16) | ((uint32_t)((p)[3]) << 24))\

#define IV_U32TO8_LE(p, v)\
do {\
    (p)[0] = (uint8_t)((v)      );\
    (p)[1] = (uint8_t)((v) >>  8);\
    (p)[2] = (uint8_t)((v) >> 16);\
    (p)[3] = (uint8_t)((v) >> 24);\
} while (0)

#define IV_U8TO64_LE(p)\
    ((uint64_t)IV_U8TO32_LE(p) | ((uint64_t)IV_U8TO32_LE((p) + 4)) << 32 )

#define IV_U64TO8_LE(p, v) \
do {\
    IV_U32TO8_LE((p),     (uint32_t)((v)      ));\
    IV_U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));\
} while (0)

#define IV_ROTL64(v, s)\
    ((v) << (s)) | ((v) >> (64 - (s)))

#define IV_ROTL64_TO(v, s) ((v) = IV_ROTL64((v), (s)))

#define IV_ADD64_TO(v, s) ((v) += (s))
#define IV_XOR64_TO(v, s) ((v) ^= (s))
#define IV_XOR64_INT(v, x) ((v) ^= (x))

#define IV_SIP_COMPRESS(v0, v1, v2, v3)\
do {\
    IV_ADD64_TO((v0), (v1));\
    IV_ADD64_TO((v2), (v3));\
    IV_ROTL64_TO((v1), 13);\
    IV_ROTL64_TO((v3), 16);\
    IV_XOR64_TO((v1), (v0));\
    IV_XOR64_TO((v3), (v2));\
    IV_ROTL64_TO((v0), 32);\
    IV_ADD64_TO((v2), (v1));\
    IV_ADD64_TO((v0), (v3));\
    IV_ROTL64_TO((v1), 17);\
    IV_ROTL64_TO((v3), 21);\
    IV_XOR64_TO((v1), (v2));\
    IV_XOR64_TO((v3), (v0));\
    IV_ROTL64_TO((v2), 32);\
} while (0)

#define IV_SIP_2_ROUND(m, v0, v1, v2, v3)\
do {\
    IV_XOR64_TO((v3), (m));\
    IV_SIP_COMPRESS(v0, v1, v2, v3);\
    IV_SIP_COMPRESS(v0, v1, v2, v3);\
    IV_XOR64_TO((v0), (m));\
} while (0)

class SipHash {
 public:
  typedef uint8_t key_type[16];
  typedef key_type Key;

  SipHash(key_type key, int c, int d)
    : c_(c),
      d_(d),
      buflen_(0),
      msglen_byte_(0) {
    const uint64_t k0 = IV_U8TO64_LE(key);
    const uint64_t k1 = IV_U8TO64_LE(key + sizeof(uint64_t));
    v_[0] = k0; IV_XOR64_TO(v_[0], InitState(0));  // NOLINT
    v_[1] = k1; IV_XOR64_TO(v_[1], InitState(1));  // NOLINT
    v_[2] = k0; IV_XOR64_TO(v_[2], InitState(2));  // NOLINT
    v_[3] = k1; IV_XOR64_TO(v_[3], InitState(3));  // NOLINT
  }

  inline int Update(uint8_t *data, std::size_t len) {
    uint64_t *end;
    uint64_t *data64;

    msglen_byte_ += (len % 256);
    data64 = reinterpret_cast<uint64_t*>(data);

    PreUpdate(&data, &len);

    end = data64 + (len / sizeof(uint64_t));

#if defined(IV_IS_LITTLE_ENDIAN)
    while (data64 != end) {
      UpdateBlock(*data64++);
    }
#else
    uint64_t m;
    uint8_t *data8 = data;
    for (;
         data8 != reinterpret_cast<uint8_t*>(end);
         data8 += sizeof(uint64_t)) {
      m = IV_U8TO64_LE(data8);
      UpdateBlock(m);
    }
#endif

    PostUpdate(data, len);
    return 1;
  }

  inline int Final(uint8_t **digest, std::size_t* len) {
    uint64_t digest64;
    uint8_t *ret;

    InternalFinal(&digest64);
    if (!(ret = static_cast<uint8_t*>(malloc(sizeof(uint64_t))))) {
      return 0;
    }

    IV_U64TO8_LE(ret, digest64);
    *len = sizeof(uint64_t);
    *digest = ret;

    return 1;
  }

  inline int FinalInteger(uint64_t *digest) {
    InternalFinal(digest);
    return 1;
  }

  inline int Digest(uint8_t *data, std::size_t data_len,
                    uint8_t **digest, std::size_t *digest_len) {
    if (!Update(data, data_len)) {
      return 0;
    }
    return Final(digest, digest_len);
  }

  inline int DigestInteger(uint8_t *data,
                           std::size_t data_len, uint64_t *digest) {
    if (!Update(data, data_len)) {
      return 0;
    }
    return FinalInteger(digest);
  }

  void Dump() {
    for (int v = 0; v < 4; v++) {
      printf("v%d: %lx\n", v, v_[v]);
    }
  }

  static inline uint64_t Hash24(key_type key, uint8_t *data, uint64_t len) {
    uint64_t k0, k1;
    uint64_t v0, v1, v2, v3;
    uint64_t m, last;
    uint8_t *end = data + len - (len % sizeof(uint64_t));

    k0 = IV_U8TO64_LE(key);
    k1 = IV_U8TO64_LE(key + sizeof(uint64_t));

    v0 = k0; IV_XOR64_TO(v0, InitState(0));  // NOLINT
    v1 = k1; IV_XOR64_TO(v1, InitState(1));  // NOLINT
    v2 = k0; IV_XOR64_TO(v2, InitState(2));  // NOLINT
    v3 = k1; IV_XOR64_TO(v3, InitState(3));  // NOLINT

#if defined(IV_IS_LITTLE_ENDIAN) && defined(IV_UNALIGNED_WORD_ACCESS)
    uint64_t *data64 = reinterpret_cast<uint64_t*>(data);
    while (data64 != reinterpret_cast<uint64_t*>(end)) {
      m = *data64++;
      IV_SIP_2_ROUND(m, v0, v1, v2, v3);
    }
#else
    for (; data != end; data += sizeof(uint64_t)) {
      m = IV_U8TO64_LE(data);
      IV_SIP_2_ROUND(m, v0, v1, v2, v3);
    }
#endif

    last = len << 56;
#define IV_OR_BYTE(n) (last |= ((uint64_t) end[n]) << ((n) * 8))
    switch (len % sizeof(uint64_t)) {
      case 7:
        IV_OR_BYTE(6);
      case 6:
        IV_OR_BYTE(5);
      case 5:
        IV_OR_BYTE(4);
      case 4:
#if defined(IV_IS_LITTLE_ENDIAN) && defined(IV_UNALIGNED_WORD_ACCESS)
        last |= static_cast<uint64_t>((reinterpret_cast<uint32_t*>(end))[0]);
        break;
#else
        IV_OR_BYTE(3);
#endif
      case 3:
        IV_OR_BYTE(2);
      case 2:
        IV_OR_BYTE(1);
      case 1:
        IV_OR_BYTE(0);
        break;
      case 0:
        break;
#undef IV_OR_BYTE
    }

    IV_SIP_2_ROUND(last, v0, v1, v2, v3);

    IV_XOR64_INT(v2, 0xff);

    IV_SIP_COMPRESS(v0, v1, v2, v3);
    IV_SIP_COMPRESS(v0, v1, v2, v3);
    IV_SIP_COMPRESS(v0, v1, v2, v3);
    IV_SIP_COMPRESS(v0, v1, v2, v3);

    IV_XOR64_TO(v0, v1);
    IV_XOR64_TO(v0, v2);
    IV_XOR64_TO(v0, v3);
    return v0;
  }

 private:
  void Round(int n) {
    for (int i = 0; i < n; i++) {
      IV_SIP_COMPRESS(v_[0], v_[1], v_[2], v_[3]);
    }
  }

  inline void UpdateBlock(uint64_t m) {
    IV_XOR64_TO(v_[3], m);
    Round(c_);
    IV_XOR64_TO(v_[0], m);
  }

  void PreUpdate(uint8_t **pdata, std::size_t *plen) {
    if (!buflen_) {
      return;
    }

    const int to_read = sizeof(uint64_t) - buflen_;
    std::memcpy(buf_ + buflen_, *pdata, to_read);
    const uint64_t m = IV_U8TO64_LE(buf_);
    UpdateBlock(m);
    *pdata += to_read;
    *plen -= to_read;
    buflen_ = 0;
  }

  void PostUpdate(uint8_t *data, std::size_t len) {
    const uint8_t r = len % sizeof(uint64_t);
    if (r) {
      std::memcpy(buf_, data + len - r, r);
      buflen_ = r;
    }
  }

  void PadFinalBlock() {
    // pad with 0's and finalize with msg_len mod 256
    for (std::size_t i = buflen_; i < sizeof(uint64_t); i++) {
      buf_[i] = 0x00;
    }
    buf_[sizeof(uint64_t) - 1] = msglen_byte_;
  }

  void InternalFinal(uint64_t *digest) {
    PadFinalBlock();

    const uint64_t m = IV_U8TO64_LE(buf_);
    UpdateBlock(m);

    IV_XOR64_INT(v_[2], 0xff);

    Round(d_);

    *digest = v_[0];
    IV_XOR64_TO(*digest, v_[1]);
    IV_XOR64_TO(*digest, v_[2]);
    IV_XOR64_TO(*digest, v_[3]);
  }

  static inline uint64_t InitState(std::size_t index) {
    static const char sip_init_state_bin[] =
        "uespemos""modnarod""arenegyl""setybdet";
    return (*(uint64_t (*)[4])sip_init_state_bin)[index];
  }

  // state
  int c_;
  int d_;
  uint64_t v_[4];
  uint8_t buf_[sizeof(uint64_t)];
  uint8_t buflen_;
  uint8_t msglen_byte_;
};

#undef IV_SIP_COMPRESS
#undef IV_XOR64_INT
#undef IV_XOR64_TO
#undef IV_ADD64_TO
#undef IV_ROTL64_TO
#undef IV_ROTL64
#undef IV_U64TO8_LE
#undef IV_U8TO64_LE
#undef IV_U32TO8_LE
#undef IV_U8TO32_LE
#undef IV_SIP_2_ROUND
} }  // namespace iv::core
#endif  // IV_SIP_HASH_H_
