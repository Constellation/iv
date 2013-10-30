#include <gtest/gtest.h>
#include <vector>
#include <iv/detail/memory.h>
#include <iv/detail/unique_ptr.h>
#include <iv/intrusive_list.h>
namespace {

class A : public iv::core::IntrusiveListBase {
 public:
  explicit A(int v) : value_(v) { }
  int value() const { return value_; }
 private:
  int value_;
};

}  // namespace anonymous

TEST(IntrusiveListCase, IntrusiveListTest) {
  iv::core::IntrusiveList<A> head;

  EXPECT_TRUE(head.empty());

  iv::core::unique_ptr<A> a(new A(0));
  head.push_back(*a);

  EXPECT_EQ(head.size(), 1u);
  EXPECT_EQ(a.get(), &head.front());
  EXPECT_TRUE(head.begin() == head.cbegin());

  head.erase(head.begin());

  EXPECT_TRUE(head.empty());
  EXPECT_TRUE(head.begin() == head.end());
}

TEST(IntrusiveListCase, IntrusiveListElementsTest) {
  typedef std::vector<std::shared_ptr<A> > Vector;
  typedef iv::core::IntrusiveList<A> List;
  Vector vec;
  for (int i = 0; i < 1000; ++i) {
    vec.push_back(std::shared_ptr<A>(new A(i)));
  }

  {
    List head;
    EXPECT_TRUE(head.empty());
    for (Vector::const_iterator it = vec.begin(),
         last = vec.end(); it != last; ++it) {
      head.push_back(**it);
    }
    EXPECT_EQ(1000u, head.size());
    EXPECT_EQ(0, head.front().value());
    EXPECT_EQ(999, head.back().value());

    {
      int i = 0;
      for (List::const_iterator it = head.cbegin(),
           last = head.cend(); it != last; ++it, ++i) {
        EXPECT_EQ(i, it->value());
      }
    }

    {
      int i = 999;
      for (List::const_reverse_iterator it = head.crbegin(),
           last = head.crend(); it != last; ++it, --i) {
        EXPECT_EQ(i, (*it).value());
      }
    }

    head.clear();
    EXPECT_TRUE(head.empty());
    for (Vector::const_iterator it = vec.begin(),
         last = vec.end(); it != last; ++it) {
      EXPECT_TRUE(!(*it)->IsLinked());
    }
  }

  {
    List head;
    EXPECT_TRUE(head.empty());
    for (Vector::const_iterator it = vec.begin(),
         last = vec.end(); it != last; ++it) {
      head.push_front(**it);
    }
    EXPECT_EQ(1000u, head.size());
    EXPECT_EQ(999, head.front().value());
    EXPECT_EQ(0, head.back().value());

    {
      int i = 999;
      for (List::const_iterator it = head.cbegin(),
           last = head.cend(); it != last; ++it, --i) {
        EXPECT_EQ(i, it->value());
      }
    }

    {
      int i = 0;
      for (List::const_reverse_iterator it = head.crbegin(),
           last = head.crend(); it != last; ++it, ++i) {
        EXPECT_EQ(i, (*it).value());
      }
    }

    head.clear();
    EXPECT_TRUE(head.empty());
    for (Vector::const_iterator it = vec.begin(),
         last = vec.end(); it != last; ++it) {
      EXPECT_TRUE(!(*it)->IsLinked());
    }
  }
}
