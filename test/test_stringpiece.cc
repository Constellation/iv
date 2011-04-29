#include <gtest/gtest.h>
#include <unicode/uchar.h>
#include "stringpiece.h"

typedef iv::core::BasicStringPiece<UChar> UStringPiece;
typedef std::basic_string<UChar> ustring;

void JoinFilePathStr(const iv::core::StringPiece& dir,
                     const iv::core::StringPiece& base,
                     std::string* out) {
  dir.CopyToString(out);
  out->push_back('/');
  base.AppendToString(out);
}

void JoinFilePathStr(const UStringPiece& dir,
                     const UStringPiece& base,
                     ustring* out) {
  dir.CopyToString(out);
  out->push_back('/');
  base.AppendToString(out);
}


TEST(StringPieceCase, CharTest) {
  using iv::core::StringPiece;
  using std::string;
  const string& dir = "/tmp";
  const string& base = "hoge.c";
  string result("/tmp/hoge.c");
  string joined;

  joined.clear();
  JoinFilePathStr("/tmp", "hoge.c", &joined);
  EXPECT_EQ(result, joined);

  joined.clear();
  JoinFilePathStr(dir, "hoge.c", &joined);
  EXPECT_EQ(result, joined);

  joined.clear();
  JoinFilePathStr("/tmp", base, &joined);
  EXPECT_EQ(result, joined);

  joined.clear();
  JoinFilePathStr(dir, base, &joined);
  EXPECT_EQ(result, joined);
}

TEST(StringPieceCase, EqualTest) {
  using iv::core::StringPiece;
  using std::string;
  EXPECT_EQ(StringPiece("OK"), StringPiece("OK"));
  EXPECT_EQ(StringPiece(string("OK")), StringPiece("OK"));
  EXPECT_EQ(StringPiece("OK"), StringPiece(string("OK")));
  EXPECT_EQ(StringPiece(string("OK")), StringPiece(string("OK")));
  EXPECT_EQ(StringPiece("OK"), StringPiece("OK"));

  EXPECT_NE(StringPiece("NG"), StringPiece("OK"));
  EXPECT_NE(StringPiece(string("NG")), StringPiece("OK"));
  EXPECT_NE(StringPiece("NG"), StringPiece(string("OK")));
  EXPECT_NE(StringPiece(string("NG")), StringPiece(string("OK")));
  EXPECT_NE(StringPiece("NG"), StringPiece("OK"));

  EXPECT_NE(StringPiece("OKK"), StringPiece("OK"));
  EXPECT_NE(StringPiece(string("OKK")), StringPiece("OK"));
  EXPECT_NE(StringPiece("OKK"), StringPiece(string("OK")));
  EXPECT_NE(StringPiece(string("OKK")), StringPiece(string("OK")));
  EXPECT_NE(StringPiece("OKK"), StringPiece("OK"));
}

TEST(StringPieceCase, LTTest) {
  using iv::core::StringPiece;
  using std::string;
  EXPECT_LT(StringPiece("A"), StringPiece("a"));
  EXPECT_LT(StringPiece("AA"), StringPiece("a"));
  EXPECT_LT(StringPiece("A"), StringPiece("aaaaaaaaaaaaaaaaaaaaaaaaa"));

  EXPECT_LT(StringPiece("a"), StringPiece("b"));
  EXPECT_LT(StringPiece("A"), StringPiece("B"));
  EXPECT_LT(StringPiece("A"), StringPiece("BBBBBBBB"));
  EXPECT_LT(StringPiece("AAAAAAAAA"), StringPiece("B"));

  EXPECT_LT(StringPiece("0"), StringPiece("1"));
  EXPECT_LT(StringPiece("0"), StringPiece("2"));
  EXPECT_LT(StringPiece("1"), StringPiece("9"));
  EXPECT_LT(StringPiece("0"), StringPiece("9"));
}

TEST(StringPieceCase, LTETest) {
  using iv::core::StringPiece;
  using std::string;
  EXPECT_LE(StringPiece("A"), StringPiece("a"));
  EXPECT_LE(StringPiece("AA"), StringPiece("a"));
  EXPECT_LE(StringPiece("A"), StringPiece("aaaaaaaaaaaaaaaaaaaaaaaaa"));

  EXPECT_LE(StringPiece("a"), StringPiece("b"));
  EXPECT_LE(StringPiece("A"), StringPiece("B"));
  EXPECT_LE(StringPiece("A"), StringPiece("BBBBBBBB"));
  EXPECT_LE(StringPiece("AAAAAAAAA"), StringPiece("B"));

  EXPECT_LE(StringPiece("0"), StringPiece("1"));
  EXPECT_LE(StringPiece("0"), StringPiece("2"));
  EXPECT_LE(StringPiece("1"), StringPiece("9"));
  EXPECT_LE(StringPiece("0"), StringPiece("9"));

  EXPECT_LE(StringPiece("A"), StringPiece("A"));
  EXPECT_LE(StringPiece("AA"), StringPiece("AA"));

  EXPECT_LE(StringPiece("b"), StringPiece("b"));
  EXPECT_LE(StringPiece("B"), StringPiece("B"));
  EXPECT_LE(StringPiece("BBBBBBB"), StringPiece("BBBBBBBB"));
  EXPECT_LE(StringPiece("AAAAAAAAA"), StringPiece("AAAAAAAAA"));

  EXPECT_LE(StringPiece("0"), StringPiece("0"));
  EXPECT_LE(StringPiece("1"), StringPiece("1"));
  EXPECT_LE(StringPiece("9"), StringPiece("9"));
}

TEST(StringPieceCase, GTTest) {
  using iv::core::StringPiece;
  using std::string;
  EXPECT_GT(StringPiece("a"), StringPiece("AA"));
  EXPECT_GT(StringPiece("a"), StringPiece("AA"));
  EXPECT_GT(StringPiece("aaaaaaaaaaaaaaaaaaaaaaaaa"), StringPiece("A"));

  EXPECT_GT(StringPiece("b"), StringPiece("a"));
  EXPECT_GT(StringPiece("B"), StringPiece("A"));
  EXPECT_GT(StringPiece("BBBBBBBB"), StringPiece("A"));
  EXPECT_GT(StringPiece("B"), StringPiece("AAAAAAAAA"));

  EXPECT_GT(StringPiece("1"), StringPiece("0"));
  EXPECT_GT(StringPiece("2"), StringPiece("0"));
  EXPECT_GT(StringPiece("9"), StringPiece("1"));
  EXPECT_GT(StringPiece("9"), StringPiece("0"));
}

TEST(StringPieceCase, GTETest) {
  using iv::core::StringPiece;
  using std::string;
  EXPECT_GE(StringPiece("a"), StringPiece("AA"));
  EXPECT_GE(StringPiece("a"), StringPiece("AA"));
  EXPECT_GE(StringPiece("aaaaaaaaaaaaaaaaaaaaaaaaa"), StringPiece("A"));

  EXPECT_GE(StringPiece("b"), StringPiece("a"));
  EXPECT_GE(StringPiece("B"), StringPiece("A"));
  EXPECT_GE(StringPiece("BBBBBBBB"), StringPiece("A"));
  EXPECT_GE(StringPiece("B"), StringPiece("AAAAAAAAA"));

  EXPECT_GE(StringPiece("1"), StringPiece("0"));
  EXPECT_GE(StringPiece("2"), StringPiece("0"));
  EXPECT_GE(StringPiece("9"), StringPiece("1"));
  EXPECT_GE(StringPiece("9"), StringPiece("0"));

  EXPECT_GE(StringPiece("A"), StringPiece("A"));
  EXPECT_GE(StringPiece("AA"), StringPiece("AA"));

  EXPECT_GE(StringPiece("b"), StringPiece("b"));
  EXPECT_GE(StringPiece("B"), StringPiece("B"));
  EXPECT_GE(StringPiece("BBBBBBBB"), StringPiece("BBBBBBB"));
  EXPECT_GE(StringPiece("AAAAAAAAA"), StringPiece("AAAAAAAAA"));

  EXPECT_GE(StringPiece("0"), StringPiece("0"));
  EXPECT_GE(StringPiece("1"), StringPiece("1"));
  EXPECT_GE(StringPiece("9"), StringPiece("9"));
}

TEST(StringPieceCase, StringCastTest) {
  using iv::core::StringPiece;
  using std::string;
  const string str = StringPiece("TEST");
  EXPECT_EQ(string("TEST"), str);
}
