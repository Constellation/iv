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
  EXPECT_EQ(joined, result);

  joined.clear();
  JoinFilePathStr(dir, "hoge.c", &joined);
  EXPECT_EQ(joined, result);

  joined.clear();
  JoinFilePathStr("/tmp", base, &joined);
  EXPECT_EQ(joined, result);

  joined.clear();
  JoinFilePathStr(dir, base, &joined);
  EXPECT_EQ(joined, result);
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
