#include <gtest/gtest.h>
#include <iv/string_view.h>
#include <iv/ustring.h>

static void JoinFilePathStr(const iv::core::string_view& dir,
                            const iv::core::string_view& base,
                            std::string* out) {
  dir.CopyToString(out);
  out->push_back('/');
  base.AppendToString(out);
}


TEST(string_viewCase, CharTest) {
  using iv::core::string_view;
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

TEST(string_viewCase, EqualTest) {
  using iv::core::string_view;
  using std::string;
  EXPECT_EQ(string_view("OK"), string_view("OK"));
  EXPECT_EQ(string_view(string("OK")), string_view("OK"));
  EXPECT_EQ(string_view("OK"), string_view(string("OK")));
  EXPECT_EQ(string_view(string("OK")), string_view(string("OK")));
  EXPECT_EQ(string_view("OK"), string_view("OK"));

  EXPECT_NE(string_view("NG"), string_view("OK"));
  EXPECT_NE(string_view(string("NG")), string_view("OK"));
  EXPECT_NE(string_view("NG"), string_view(string("OK")));
  EXPECT_NE(string_view(string("NG")), string_view(string("OK")));
  EXPECT_NE(string_view("NG"), string_view("OK"));

  EXPECT_NE(string_view("OKK"), string_view("OK"));
  EXPECT_NE(string_view(string("OKK")), string_view("OK"));
  EXPECT_NE(string_view("OKK"), string_view(string("OK")));
  EXPECT_NE(string_view(string("OKK")), string_view(string("OK")));
  EXPECT_NE(string_view("OKK"), string_view("OK"));

  {
    const iv::core::UString s1 = iv::core::ToUString('0');
    const iv::core::UString s2 = iv::core::ToUString('+');
    EXPECT_FALSE(iv::core::u16string_view(s1) == iv::core::u16string_view(s2));
  }
}

TEST(string_viewCase, LTTest) {
  using iv::core::string_view;
  using std::string;
  EXPECT_LT(string_view("A"), string_view("a"));
  EXPECT_LT(string_view("AA"), string_view("a"));
  EXPECT_LT(string_view("A"), string_view("aaaaaaaaaaaaaaaaaaaaaaaaa"));

  EXPECT_LT(string_view("a"), string_view("b"));
  EXPECT_LT(string_view("A"), string_view("B"));
  EXPECT_LT(string_view("A"), string_view("BBBBBBBB"));
  EXPECT_LT(string_view("AAAAAAAAA"), string_view("B"));

  EXPECT_LT(string_view("0"), string_view("1"));
  EXPECT_LT(string_view("0"), string_view("2"));
  EXPECT_LT(string_view("1"), string_view("9"));
  EXPECT_LT(string_view("0"), string_view("9"));
}

TEST(string_viewCase, LTETest) {
  using iv::core::string_view;
  using std::string;
  EXPECT_LE(string_view("A"), string_view("a"));
  EXPECT_LE(string_view("AA"), string_view("a"));
  EXPECT_LE(string_view("A"), string_view("aaaaaaaaaaaaaaaaaaaaaaaaa"));

  EXPECT_LE(string_view("a"), string_view("b"));
  EXPECT_LE(string_view("A"), string_view("B"));
  EXPECT_LE(string_view("A"), string_view("BBBBBBBB"));
  EXPECT_LE(string_view("AAAAAAAAA"), string_view("B"));

  EXPECT_LE(string_view("0"), string_view("1"));
  EXPECT_LE(string_view("0"), string_view("2"));
  EXPECT_LE(string_view("1"), string_view("9"));
  EXPECT_LE(string_view("0"), string_view("9"));

  EXPECT_LE(string_view("A"), string_view("A"));
  EXPECT_LE(string_view("AA"), string_view("AA"));

  EXPECT_LE(string_view("b"), string_view("b"));
  EXPECT_LE(string_view("B"), string_view("B"));
  EXPECT_LE(string_view("BBBBBBB"), string_view("BBBBBBBB"));
  EXPECT_LE(string_view("AAAAAAAAA"), string_view("AAAAAAAAA"));

  EXPECT_LE(string_view("0"), string_view("0"));
  EXPECT_LE(string_view("1"), string_view("1"));
  EXPECT_LE(string_view("9"), string_view("9"));
}

TEST(string_viewCase, GTTest) {
  using iv::core::string_view;
  using std::string;
  EXPECT_GT(string_view("a"), string_view("AA"));
  EXPECT_GT(string_view("a"), string_view("AA"));
  EXPECT_GT(string_view("aaaaaaaaaaaaaaaaaaaaaaaaa"), string_view("A"));

  EXPECT_GT(string_view("b"), string_view("a"));
  EXPECT_GT(string_view("B"), string_view("A"));
  EXPECT_GT(string_view("BBBBBBBB"), string_view("A"));
  EXPECT_GT(string_view("B"), string_view("AAAAAAAAA"));

  EXPECT_GT(string_view("1"), string_view("0"));
  EXPECT_GT(string_view("2"), string_view("0"));
  EXPECT_GT(string_view("9"), string_view("1"));
  EXPECT_GT(string_view("9"), string_view("0"));
}

TEST(string_viewCase, GTETest) {
  using iv::core::string_view;
  using std::string;
  EXPECT_GE(string_view("a"), string_view("AA"));
  EXPECT_GE(string_view("a"), string_view("AA"));
  EXPECT_GE(string_view("aaaaaaaaaaaaaaaaaaaaaaaaa"), string_view("A"));

  EXPECT_GE(string_view("b"), string_view("a"));
  EXPECT_GE(string_view("B"), string_view("A"));
  EXPECT_GE(string_view("BBBBBBBB"), string_view("A"));
  EXPECT_GE(string_view("B"), string_view("AAAAAAAAA"));

  EXPECT_GE(string_view("1"), string_view("0"));
  EXPECT_GE(string_view("2"), string_view("0"));
  EXPECT_GE(string_view("9"), string_view("1"));
  EXPECT_GE(string_view("9"), string_view("0"));

  EXPECT_GE(string_view("A"), string_view("A"));
  EXPECT_GE(string_view("AA"), string_view("AA"));

  EXPECT_GE(string_view("b"), string_view("b"));
  EXPECT_GE(string_view("B"), string_view("B"));
  EXPECT_GE(string_view("BBBBBBBB"), string_view("BBBBBBB"));
  EXPECT_GE(string_view("AAAAAAAAA"), string_view("AAAAAAAAA"));

  EXPECT_GE(string_view("0"), string_view("0"));
  EXPECT_GE(string_view("1"), string_view("1"));
  EXPECT_GE(string_view("9"), string_view("9"));
}

TEST(string_viewCase, StringCastTest) {
  using iv::core::string_view;
  using std::string;
  const string str = string_view("TEST");
  EXPECT_EQ(string("TEST"), str);
}

TEST(string_viewCase, FindTest) {
  using iv::core::string_view;
  EXPECT_EQ(0u, string_view("TEST").find('T'));
  EXPECT_EQ(1u, string_view("TEST").find('E'));
  EXPECT_EQ(2u, string_view("TEST").find('S'));
  EXPECT_EQ(string_view::npos, string_view("TEST").find('O'));

  EXPECT_EQ(3u, string_view("TEST").find('T', 1));
  EXPECT_EQ(1u, string_view("TEST").find('E', 1));
  EXPECT_EQ(3u, string_view("TEST").find('T', 3));
  EXPECT_EQ(string_view::npos, string_view("TEST").find('S', 3));
  EXPECT_EQ(string_view::npos, string_view("TEST").find('S', 5));

  EXPECT_EQ(0u, string_view("TEST").find("TEST"));
  EXPECT_EQ(1u, string_view("TEST").find("EST"));
  EXPECT_EQ(0u, string_view("TEST").find("TE"));
  EXPECT_EQ(string_view::npos, string_view("TEST").find("TESTING"));

  EXPECT_EQ(1u, string_view("TEST").find("EST", 1));
  EXPECT_EQ(3u, string_view("TEST").find("T", 2));
  EXPECT_EQ(string_view::npos, string_view("TEST").find("T", 1000));
  EXPECT_EQ(string_view::npos, string_view("TEST").find("TEST", 1));
}

TEST(string_viewCase, RFindTest) {
  using iv::core::string_view;
  EXPECT_EQ(3u, string_view("TEST").rfind('T'));
  EXPECT_EQ(1u, string_view("TEST").rfind('E'));
  EXPECT_EQ(2u, string_view("TEST").rfind('S'));
  EXPECT_EQ(string_view::npos, string_view("TEST").rfind('O'));

  EXPECT_EQ(0u, string_view("TEST").rfind('T', 1));
  EXPECT_EQ(1u, string_view("TEST").rfind('E', 1));
  EXPECT_EQ(3u, string_view("TEST").rfind('T', 3));
  EXPECT_EQ(2u, string_view("TEST").rfind('S', 100));
  EXPECT_EQ(string_view::npos, string_view("TEST").rfind('S', 0));
  EXPECT_EQ(string_view::npos, string_view("TEST").rfind('S', 1));

  EXPECT_EQ(0u, string_view("TEST").rfind("TEST"));
  EXPECT_EQ(1u, string_view("TEST").rfind("EST"));
  EXPECT_EQ(0u, string_view("TEST").rfind("TE"));
  EXPECT_EQ(3u, string_view("TEST").rfind("T"));
  EXPECT_EQ(string_view::npos, string_view("TEST").rfind("TESTING"));

  EXPECT_EQ(1u, string_view("TEST").rfind("EST", 1));
  EXPECT_EQ(0u, string_view("TEST").rfind("T", 2));
  EXPECT_EQ(3u, string_view("TEST").rfind("T", 1000));
  EXPECT_EQ(0u, string_view("TEST").rfind("TES", 1));
  EXPECT_EQ(string_view::npos, string_view("TEST").rfind("ST", 1));
}

TEST(string_viewCase, FindFirstOfTest) {
  using iv::core::string_view;
  EXPECT_EQ(0u, string_view("TEST").find_first_of('T'));
  EXPECT_EQ(1u, string_view("TEST").find_first_of('E'));
  EXPECT_EQ(2u, string_view("TEST").find_first_of('S'));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_of('O'));

  EXPECT_EQ(0u, string_view("TEST").find_first_of("TEST"));
  EXPECT_EQ(1u, string_view("TEST").find_first_of("ES"));
  EXPECT_EQ(2u, string_view("TEST").find_first_of("S"));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_of("O"));

  EXPECT_EQ(3u, string_view("TEST").find_first_of('T', 1));
  EXPECT_EQ(1u, string_view("TEST").find_first_of('E', 1));
  EXPECT_EQ(3u, string_view("TEST").find_first_of('T', 3));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_of('S', 100));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_of('S', 3));

  EXPECT_EQ(0u, string_view("TEST").find_first_of("TEST", 0));
  EXPECT_EQ(1u, string_view("TEST").find_first_of("EST", 1));
  EXPECT_EQ(0u, string_view("TEST").find_first_of("TE", 0));
  EXPECT_EQ(3u, string_view("TEST").find_first_of("TE", 2));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_of("O", 2));
}

TEST(string_viewCase, FindLastOfTest) {
  using iv::core::string_view;
  EXPECT_EQ(3u, string_view("TEST").find_last_of('T'));
  EXPECT_EQ(1u, string_view("TEST").find_last_of('E'));
  EXPECT_EQ(2u, string_view("TEST").find_last_of('S'));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_of('O'));

  EXPECT_EQ(3u, string_view("TEST").find_last_of("TEST"));
  EXPECT_EQ(2u, string_view("TEST").find_last_of("ES"));
  EXPECT_EQ(2u, string_view("TEST").find_last_of("S"));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_of("O"));

  EXPECT_EQ(0u, string_view("TEST").find_last_of('T', 1));
  EXPECT_EQ(1u, string_view("TEST").find_last_of('E', 1));
  EXPECT_EQ(3u, string_view("TEST").find_last_of('T', 3));
  EXPECT_EQ(2u, string_view("TEST").find_last_of('S', 100));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_of('S', 1));

  EXPECT_EQ(0u, string_view("TEST").find_last_of("TEST", 0));
  EXPECT_EQ(1u, string_view("TEST").find_last_of("EST", 1));
  EXPECT_EQ(0u, string_view("TEST").find_last_of("TE", 0));
  EXPECT_EQ(1u, string_view("TEST").find_last_of("TE", 2));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_of("O", 2));
}


TEST(string_viewCase, FindFirstNotOfTest) {
  using iv::core::string_view;
  EXPECT_EQ(1u, string_view("TEST").find_first_not_of('T'));
  EXPECT_EQ(0u, string_view("TEST").find_first_not_of('E'));
  EXPECT_EQ(0u, string_view("TEST").find_first_not_of('S'));
  EXPECT_EQ(0u, string_view("TEST").find_first_not_of('O'));

  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of("TEST"));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of("TES"));
  EXPECT_EQ(0u, string_view("TEST").find_first_not_of("ES"));
  EXPECT_EQ(0u, string_view("TEST").find_first_not_of("S"));
  EXPECT_EQ(2u, string_view("TEST").find_first_not_of("TE"));
  EXPECT_EQ(0u, string_view("TEST").find_first_not_of("O"));

  EXPECT_EQ(1u, string_view("TEST").find_first_not_of('T', 1));
  EXPECT_EQ(3u, string_view("TEST").find_first_not_of('S', 2));
  EXPECT_EQ(2u, string_view("TEST").find_first_not_of('E', 2));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of('T', 3));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of('S', 100));
  EXPECT_EQ(3u, string_view("TEST").find_first_not_of('S', 2));
  EXPECT_EQ(3u, string_view("TEST").find_first_not_of('S', 3));

  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of("TEST", 0));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of("EST", 1));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of("TS", 2));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_first_not_of("ETS", 1));
  EXPECT_EQ(2u, string_view("TEST").find_first_not_of("TE", 0));
  EXPECT_EQ(2u, string_view("TEST").find_first_not_of("O", 2));
  EXPECT_EQ(2u, string_view("TEST").find_first_not_of("TE", 2));
}

TEST(string_viewCase, FindLastNotOfTest) {
  using iv::core::string_view;
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of('T'));
  EXPECT_EQ(3u, string_view("TEST").find_last_not_of('E'));
  EXPECT_EQ(3u, string_view("TEST").find_last_not_of('S'));
  EXPECT_EQ(3u, string_view("TEST").find_last_not_of('O'));

  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of("TEST"));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of("TES"));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of("EST"));
  EXPECT_EQ(3u, string_view("TEST").find_last_not_of("ES"));
  EXPECT_EQ(3u, string_view("TEST").find_last_not_of("S"));
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of("TE"));
  EXPECT_EQ(3u, string_view("TEST").find_last_not_of("O"));

  EXPECT_EQ(1u, string_view("TEST").find_last_not_of('T', 1));
  EXPECT_EQ(1u, string_view("TEST").find_last_not_of('S', 2));
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of('E', 2));
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of('T', 3));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of('T', 0));
  EXPECT_EQ(3u, string_view("TEST").find_last_not_of('S', 100));
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of('T', 100));
  EXPECT_EQ(1u, string_view("TEST").find_last_not_of('S', 1));
  EXPECT_EQ(0u, string_view("TEST").find_last_not_of('S', 0));

  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of("TEST", 0));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of("EST", 1));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of("TE", 1));
  EXPECT_EQ(string_view::npos, string_view("TEST").find_last_not_of("ETS", 1));
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of("TE", 3));
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of("O", 2));
  EXPECT_EQ(2u, string_view("TEST").find_last_not_of("TE", 2));
}
