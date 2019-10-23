#include "gtest/gtest.h"
#include "oef-base/utils/Uri.hpp"



class OefBaseUtilsTests : public testing::Test
{
};

TEST_F(OefBaseUtilsTests, UriTest)
{
  Uri uri("tcp://127.0.0.1:10000");

  EXPECT_EQ(uri.proto, "tcp");
  EXPECT_EQ(uri.host, "127.0.0.1");
  EXPECT_EQ(uri.port, 10000);
  EXPECT_EQ(uri.path, "");
}

TEST_F(OefBaseUtilsTests, UriTest2)
{
  Uri uri("127.0.0.1:10000");
  uri.diagnostic();

  EXPECT_EQ(uri.proto, "");
  EXPECT_EQ(uri.host, "127.0.0.1");
  EXPECT_EQ(uri.port, 10000);
  EXPECT_EQ(uri.path, "");
}

