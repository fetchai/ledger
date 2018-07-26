
#include "testing/unittest.hpp"
#include "vectorise/meta/log2.hpp"
#include <iostream>

#define TEST_LOG_EX__EQ__X(X) fetch::meta::Log2<(1ull << X)>::value == X

#define TEST_LOG_EX_PLUS_Y__EQ__X(X, Y) \
  fetch::meta::Log2<(1ull << X) + Y>::value == X
int main()
{
  SCENARIO("Testing exact exponents")
  {
    SECTION("0 TO 9")
    {
      EXPECT(TEST_LOG_EX__EQ__X(0));
      EXPECT(TEST_LOG_EX__EQ__X(1));
      EXPECT(TEST_LOG_EX__EQ__X(2));
      EXPECT(TEST_LOG_EX__EQ__X(3));
      EXPECT(TEST_LOG_EX__EQ__X(4));
      EXPECT(TEST_LOG_EX__EQ__X(5));
      EXPECT(TEST_LOG_EX__EQ__X(6));
      EXPECT(TEST_LOG_EX__EQ__X(7));
      EXPECT(TEST_LOG_EX__EQ__X(8));
      EXPECT(TEST_LOG_EX__EQ__X(9));
    };

    SECTION("10 TO 19")
    {
      EXPECT(TEST_LOG_EX__EQ__X(10));
      EXPECT(TEST_LOG_EX__EQ__X(11));
      EXPECT(TEST_LOG_EX__EQ__X(12));
      EXPECT(TEST_LOG_EX__EQ__X(13));
      EXPECT(TEST_LOG_EX__EQ__X(14));
      EXPECT(TEST_LOG_EX__EQ__X(15));
      EXPECT(TEST_LOG_EX__EQ__X(16));
      EXPECT(TEST_LOG_EX__EQ__X(17));
      EXPECT(TEST_LOG_EX__EQ__X(18));
      EXPECT(TEST_LOG_EX__EQ__X(19));
    };

    SECTION("20 TO 29")
    {
      EXPECT(TEST_LOG_EX__EQ__X(20));
      EXPECT(TEST_LOG_EX__EQ__X(21));
      EXPECT(TEST_LOG_EX__EQ__X(22));
      EXPECT(TEST_LOG_EX__EQ__X(23));
      EXPECT(TEST_LOG_EX__EQ__X(24));
      EXPECT(TEST_LOG_EX__EQ__X(25));
      EXPECT(TEST_LOG_EX__EQ__X(26));
      EXPECT(TEST_LOG_EX__EQ__X(27));
      EXPECT(TEST_LOG_EX__EQ__X(28));
      EXPECT(TEST_LOG_EX__EQ__X(29));
    };

    SECTION("30 TO 39")
    {
      EXPECT(TEST_LOG_EX__EQ__X(30));
      EXPECT(TEST_LOG_EX__EQ__X(31));
      EXPECT(TEST_LOG_EX__EQ__X(32));
      EXPECT(TEST_LOG_EX__EQ__X(33));
      EXPECT(TEST_LOG_EX__EQ__X(34));
      EXPECT(TEST_LOG_EX__EQ__X(35));
      EXPECT(TEST_LOG_EX__EQ__X(36));
      EXPECT(TEST_LOG_EX__EQ__X(37));
      EXPECT(TEST_LOG_EX__EQ__X(38));
      EXPECT(TEST_LOG_EX__EQ__X(39));
    };

    SECTION("40 TO 49")
    {
      EXPECT(TEST_LOG_EX__EQ__X(40));
      EXPECT(TEST_LOG_EX__EQ__X(41));
      EXPECT(TEST_LOG_EX__EQ__X(42));
      EXPECT(TEST_LOG_EX__EQ__X(43));
      EXPECT(TEST_LOG_EX__EQ__X(44));
      EXPECT(TEST_LOG_EX__EQ__X(45));
      EXPECT(TEST_LOG_EX__EQ__X(46));
      EXPECT(TEST_LOG_EX__EQ__X(47));
      EXPECT(TEST_LOG_EX__EQ__X(48));
      EXPECT(TEST_LOG_EX__EQ__X(49));
    };

    SECTION("50 TO 59")
    {
      EXPECT(TEST_LOG_EX__EQ__X(50));
      EXPECT(TEST_LOG_EX__EQ__X(51));
      EXPECT(TEST_LOG_EX__EQ__X(52));
      EXPECT(TEST_LOG_EX__EQ__X(53));
      EXPECT(TEST_LOG_EX__EQ__X(54));
      EXPECT(TEST_LOG_EX__EQ__X(55));
      EXPECT(TEST_LOG_EX__EQ__X(56));
      EXPECT(TEST_LOG_EX__EQ__X(57));
      EXPECT(TEST_LOG_EX__EQ__X(58));
      EXPECT(TEST_LOG_EX__EQ__X(59));
    };

    SECTION("60 TO 63")
    {
      EXPECT(TEST_LOG_EX__EQ__X(60));
      EXPECT(TEST_LOG_EX__EQ__X(61));
      EXPECT(TEST_LOG_EX__EQ__X(62));
      EXPECT(TEST_LOG_EX__EQ__X(63));
    };
  };

  SCENARIO("Testing exact exponents")
  {
    SECTION("Randomly selected tests")
    {
      EXPECT(TEST_LOG_EX_PLUS_Y__EQ__X(0, 0));
      EXPECT(TEST_LOG_EX_PLUS_Y__EQ__X(1, 0));
      EXPECT(TEST_LOG_EX_PLUS_Y__EQ__X(2, 1));
      EXPECT(TEST_LOG_EX_PLUS_Y__EQ__X(3, 2));
      EXPECT(TEST_LOG_EX_PLUS_Y__EQ__X(3, 2));
      EXPECT(TEST_LOG_EX_PLUS_Y__EQ__X(3, 7));
      EXPECT(TEST_LOG_EX_PLUS_Y__EQ__X(6, (1ull << 6) - 1));
    };
  };

  return 0;
}
