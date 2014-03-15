#include <gtest/gtest.h>
#include <string>
#include <iv/string.h>
#include <iv/stringpiece.h>

static void JoinFilePathStr(const iv::core::StringPiece& dir,
                            const iv::core::StringPiece& base,
                            std::string* out) {
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

  {
    const std::u16string s1 = iv::core::ToU16String('0');
    const std::u16string s2 = iv::core::ToU16String('+');
    EXPECT_FALSE(iv::core::U16StringPiece(s1) == iv::core::U16StringPiece(s2));
  }
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

TEST(StringPieceCase, FindTest) {
  using iv::core::StringPiece;
  EXPECT_EQ(0u, StringPiece("TEST").find('T'));
  EXPECT_EQ(1u, StringPiece("TEST").find('E'));
  EXPECT_EQ(2u, StringPiece("TEST").find('S'));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find('O'));

  EXPECT_EQ(3u, StringPiece("TEST").find('T', 1));
  EXPECT_EQ(1u, StringPiece("TEST").find('E', 1));
  EXPECT_EQ(3u, StringPiece("TEST").find('T', 3));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find('S', 3));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find('S', 5));

  EXPECT_EQ(0u, StringPiece("TEST").find("TEST"));
  EXPECT_EQ(1u, StringPiece("TEST").find("EST"));
  EXPECT_EQ(0u, StringPiece("TEST").find("TE"));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find("TESTING"));

  EXPECT_EQ(1u, StringPiece("TEST").find("EST", 1));
  EXPECT_EQ(3u, StringPiece("TEST").find("T", 2));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find("T", 1000));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find("TEST", 1));
}

TEST(StringPieceCase, RFindTest) {
  using iv::core::StringPiece;
  EXPECT_EQ(3u, StringPiece("TEST").rfind('T'));
  EXPECT_EQ(1u, StringPiece("TEST").rfind('E'));
  EXPECT_EQ(2u, StringPiece("TEST").rfind('S'));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").rfind('O'));

  EXPECT_EQ(0u, StringPiece("TEST").rfind('T', 1));
  EXPECT_EQ(1u, StringPiece("TEST").rfind('E', 1));
  EXPECT_EQ(3u, StringPiece("TEST").rfind('T', 3));
  EXPECT_EQ(2u, StringPiece("TEST").rfind('S', 100));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").rfind('S', 0));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").rfind('S', 1));

  EXPECT_EQ(0u, StringPiece("TEST").rfind("TEST"));
  EXPECT_EQ(1u, StringPiece("TEST").rfind("EST"));
  EXPECT_EQ(0u, StringPiece("TEST").rfind("TE"));
  EXPECT_EQ(3u, StringPiece("TEST").rfind("T"));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").rfind("TESTING"));

  EXPECT_EQ(1u, StringPiece("TEST").rfind("EST", 1));
  EXPECT_EQ(0u, StringPiece("TEST").rfind("T", 2));
  EXPECT_EQ(3u, StringPiece("TEST").rfind("T", 1000));
  EXPECT_EQ(0u, StringPiece("TEST").rfind("TES", 1));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").rfind("ST", 1));
}

TEST(StringPieceCase, FindFirstOfTest) {
  using iv::core::StringPiece;
  EXPECT_EQ(0u, StringPiece("TEST").find_first_of('T'));
  EXPECT_EQ(1u, StringPiece("TEST").find_first_of('E'));
  EXPECT_EQ(2u, StringPiece("TEST").find_first_of('S'));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_of('O'));

  EXPECT_EQ(0u, StringPiece("TEST").find_first_of("TEST"));
  EXPECT_EQ(1u, StringPiece("TEST").find_first_of("ES"));
  EXPECT_EQ(2u, StringPiece("TEST").find_first_of("S"));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_of("O"));

  EXPECT_EQ(3u, StringPiece("TEST").find_first_of('T', 1));
  EXPECT_EQ(1u, StringPiece("TEST").find_first_of('E', 1));
  EXPECT_EQ(3u, StringPiece("TEST").find_first_of('T', 3));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_of('S', 100));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_of('S', 3));

  EXPECT_EQ(0u, StringPiece("TEST").find_first_of("TEST", 0));
  EXPECT_EQ(1u, StringPiece("TEST").find_first_of("EST", 1));
  EXPECT_EQ(0u, StringPiece("TEST").find_first_of("TE", 0));
  EXPECT_EQ(3u, StringPiece("TEST").find_first_of("TE", 2));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_of("O", 2));
}

TEST(StringPieceCase, FindLastOfTest) {
  using iv::core::StringPiece;
  EXPECT_EQ(3u, StringPiece("TEST").find_last_of('T'));
  EXPECT_EQ(1u, StringPiece("TEST").find_last_of('E'));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_of('S'));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_of('O'));

  EXPECT_EQ(3u, StringPiece("TEST").find_last_of("TEST"));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_of("ES"));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_of("S"));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_of("O"));

  EXPECT_EQ(0u, StringPiece("TEST").find_last_of('T', 1));
  EXPECT_EQ(1u, StringPiece("TEST").find_last_of('E', 1));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_of('T', 3));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_of('S', 100));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_of('S', 1));

  EXPECT_EQ(0u, StringPiece("TEST").find_last_of("TEST", 0));
  EXPECT_EQ(1u, StringPiece("TEST").find_last_of("EST", 1));
  EXPECT_EQ(0u, StringPiece("TEST").find_last_of("TE", 0));
  EXPECT_EQ(1u, StringPiece("TEST").find_last_of("TE", 2));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_of("O", 2));
}


TEST(StringPieceCase, FindFirstNotOfTest) {
  using iv::core::StringPiece;
  EXPECT_EQ(1u, StringPiece("TEST").find_first_not_of('T'));
  EXPECT_EQ(0u, StringPiece("TEST").find_first_not_of('E'));
  EXPECT_EQ(0u, StringPiece("TEST").find_first_not_of('S'));
  EXPECT_EQ(0u, StringPiece("TEST").find_first_not_of('O'));

  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of("TEST"));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of("TES"));
  EXPECT_EQ(0u, StringPiece("TEST").find_first_not_of("ES"));
  EXPECT_EQ(0u, StringPiece("TEST").find_first_not_of("S"));
  EXPECT_EQ(2u, StringPiece("TEST").find_first_not_of("TE"));
  EXPECT_EQ(0u, StringPiece("TEST").find_first_not_of("O"));

  EXPECT_EQ(1u, StringPiece("TEST").find_first_not_of('T', 1));
  EXPECT_EQ(3u, StringPiece("TEST").find_first_not_of('S', 2));
  EXPECT_EQ(2u, StringPiece("TEST").find_first_not_of('E', 2));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of('T', 3));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of('S', 100));
  EXPECT_EQ(3u, StringPiece("TEST").find_first_not_of('S', 2));
  EXPECT_EQ(3u, StringPiece("TEST").find_first_not_of('S', 3));

  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of("TEST", 0));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of("EST", 1));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of("TS", 2));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_first_not_of("ETS", 1));
  EXPECT_EQ(2u, StringPiece("TEST").find_first_not_of("TE", 0));
  EXPECT_EQ(2u, StringPiece("TEST").find_first_not_of("O", 2));
  EXPECT_EQ(2u, StringPiece("TEST").find_first_not_of("TE", 2));
}

TEST(StringPieceCase, FindLastNotOfTest) {
  using iv::core::StringPiece;
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of('T'));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_not_of('E'));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_not_of('S'));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_not_of('O'));

  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of("TEST"));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of("TES"));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of("EST"));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_not_of("ES"));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_not_of("S"));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of("TE"));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_not_of("O"));

  EXPECT_EQ(1u, StringPiece("TEST").find_last_not_of('T', 1));
  EXPECT_EQ(1u, StringPiece("TEST").find_last_not_of('S', 2));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of('E', 2));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of('T', 3));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of('T', 0));
  EXPECT_EQ(3u, StringPiece("TEST").find_last_not_of('S', 100));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of('T', 100));
  EXPECT_EQ(1u, StringPiece("TEST").find_last_not_of('S', 1));
  EXPECT_EQ(0u, StringPiece("TEST").find_last_not_of('S', 0));

  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of("TEST", 0));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of("EST", 1));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of("TE", 1));
  EXPECT_EQ(StringPiece::npos, StringPiece("TEST").find_last_not_of("ETS", 1));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of("TE", 3));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of("O", 2));
  EXPECT_EQ(2u, StringPiece("TEST").find_last_not_of("TE", 2));
}
