#include <gtest/gtest.h>
#include <iv/detail/array.h>
#include <iv/detail/memory.h>
#include <iv/segmented_vector.h>

TEST(SegmentedVectorCase, IntTest) {
  typedef iv::core::SegmentedVector<int, 8> IntVector;
  IntVector vector;
  for (int i = 0; i < 10; ++i) {
    vector.push_back(i);
  }
  EXPECT_EQ(10u, vector.size());

  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(i, vector[i]);
  }
  EXPECT_EQ(vector.SegmentFor(0), vector.SegmentFor(7));
  EXPECT_NE(vector.SegmentFor(0), vector.SegmentFor(8));

  {
    {
      int i = 0;
      for (IntVector::iterator it = vector.begin(),
           last = vector.end(); it != last; ++it, ++i) {
        EXPECT_EQ(i, *it);
        EXPECT_EQ(vector[i], *it);
      }
    }

    {
      int i = 0;
      for (IntVector::const_iterator it = vector.begin(),
           last = vector.end(); it != last; ++it, ++i) {
        EXPECT_EQ(i, *it);
        EXPECT_EQ(vector[i], *it);
      }
    }

    {
      int i = 0;
      for (IntVector::const_iterator it = vector.cbegin(),
           last = vector.cend(); it != last; ++it, ++i) {
        EXPECT_EQ(i, *it);
        EXPECT_EQ(vector[i], *it);
      }
    }
  }

  {
    {
      int i = 9;
      for (IntVector::reverse_iterator it = vector.rbegin(),
           last = vector.rend(); it != last; ++it, --i) {
        EXPECT_EQ(i, *it);
        EXPECT_EQ(vector[i], *it);
      }
    }

    {
      int i = 9;
      for (IntVector::const_reverse_iterator it = vector.rbegin(),
           last = vector.rend(); it != last; ++it, --i) {
        EXPECT_EQ(i, *it);
        EXPECT_EQ(vector[i], *it);
      }
    }

    {
      int i = 9;
      for (IntVector::const_reverse_iterator it = vector.crbegin(),
           last = vector.crend(); it != last; ++it, --i) {
        EXPECT_EQ(i, *it);
        EXPECT_EQ(vector[i], *it);
      }
    }
  }

  vector.shrink_to_fit();
  EXPECT_EQ(10u, vector.size());
  vector.clear();
  EXPECT_EQ(0u, vector.size());
  vector.shrink_to_fit();
  EXPECT_EQ(0u, vector.size());
}

namespace {

class A {
 public:
  A(int* pointer) : p(pointer) {
  }

  ~A() {
    *p = 100;
  }

  int* pointer() const { return p; }

  int* p;
};

}  // namespace anonymous

TEST(SegmentedVectorCase, ClassTest) {
  typedef iv::core::SegmentedVector<std::shared_ptr<A>, 8> Vector;
  Vector vector;
  std::vector<int> is(10, 0);

  EXPECT_EQ(10u, is.size());

  for (int i = 0; i < 10; ++i) {
    vector.push_back(std::shared_ptr<A>(new A(is.data() + i)));
  }

  EXPECT_EQ(10u, vector.size());

  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(is.data() + i, vector[i]->pointer());
    EXPECT_EQ(0, is[i]);
  }

  vector.clear();
  EXPECT_EQ(0u, vector.size());

  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(100, is[i]);
  }
}
