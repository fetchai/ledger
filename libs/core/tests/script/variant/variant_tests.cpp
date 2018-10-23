//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/script/variant.hpp"
#include <iostream>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
using namespace fetch::script;

#include <gtest/gtest.h>

TEST(basic_manipulation, variant)
{
  Variant x;
  x = 1;
  EXPECT_EQ(x.type(), fetch::script::VariantType::INTEGER);
  x = "Hello world";
  EXPECT_EQ(x.type(), fetch::script::VariantType::STRING);
  x = nullptr;
  EXPECT_EQ(x.type(), fetch::script::VariantType::NULL_VALUE);
  x = 4.21;
  EXPECT_EQ(x.type(), fetch::script::VariantType::FLOATING_POINT);
  x.MakeUndefined();
  EXPECT_EQ(x.type(), fetch::script::VariantType::UNDEFINED);
}

TEST(basic_manipulation, variant_list)
{
  VariantArray x(6);
  EXPECT_EQ(x.size(), 6);

  x[0] = 1.2;
  x[1] = "Hello world";
  x[2] = 2;
  x[3] = true;
  x[5] = nullptr;

  EXPECT_EQ(x[0].type(), fetch::script::VariantType::FLOATING_POINT);
  EXPECT_EQ(x[1].type(), fetch::script::VariantType::STRING);
  EXPECT_EQ(x[2].type(), fetch::script::VariantType::INTEGER);
  EXPECT_EQ(x[3].type(), fetch::script::VariantType::BOOLEAN);
  EXPECT_EQ(x[4].type(), fetch::script::VariantType::UNDEFINED);
  EXPECT_EQ(x[5].type(), fetch::script::VariantType::NULL_VALUE);
}

TEST(basic_manipulation, variant_object)
{
  Variant obj                 = Variant::Object();
  obj["numberOfTransactions"] = uint32_t(9);
  EXPECT_EQ(obj["numberOfTransactions"].type(), fetch::script::VariantType::INTEGER);
  EXPECT_EQ(obj["numberOfTransactions"].As<int>(), 9);

  obj["numberOfTransactions"] = "Hello world";
  std::cout << obj["numberOfTransactions"].type() << std::endl;
  obj["blah"]  = 9;
  obj["Hello"] = false;
  obj["XX"]    = nullptr;

  std::cout << obj["numberOfTransactions"].type() << std::endl;

  EXPECT_EQ(obj["numberOfTransactions"].type(), fetch::script::VariantType::STRING);
  EXPECT_EQ(obj["numberOfTransactions"].as_byte_array(), "Hello world");

  EXPECT_EQ(obj["blah"].type(), fetch::script::VariantType::INTEGER);
  EXPECT_EQ(obj["blah"].As<int>(), 9);

  EXPECT_EQ(obj["Hello"].type(), fetch::script::VariantType::BOOLEAN);
  EXPECT_EQ(obj["Hello"].As<bool>(), false);

  EXPECT_EQ(obj["XX"].type(), fetch::script::VariantType::NULL_VALUE);

  // Failing test
  Variant result                 = Variant::Object();
  result["numberOfTransactions"] = uint32_t(2);
  result["hash"]                 = "some_hash";

  std::ostringstream stream;
  stream << result;  // failing operation

  // Remove all spaces for string compare
  std::string           asString{stream.str()};
  std::string::iterator end_pos = std::remove(asString.begin(), asString.end(), ' ');
  asString.erase(end_pos, asString.end());

  std::cerr << asString << std::endl;

  EXPECT_EQ(asString.compare("{\"numberOfTransactions\":2,\"hash\":\"some_hash\"}"), 0);
}

TEST(basic_manipulation, nested_variants)
{
  Variant x;
  x.MakeArray(2);
  x[0].MakeArray(3);
  x[0][0] = 1;
  x[0][1] = 3;
  x[0][2] = 7;
  x[1]    = 1.23e-6;

  EXPECT_EQ(x.type(), fetch::script::VariantType::ARRAY);
  EXPECT_EQ(x[0].type(), fetch::script::VariantType::ARRAY);
  EXPECT_EQ(x[0][0].type(), fetch::script::VariantType::INTEGER);
  EXPECT_EQ(x[0][1].type(), fetch::script::VariantType::INTEGER);
  EXPECT_EQ(x[0][2].type(), fetch::script::VariantType::INTEGER);
  EXPECT_EQ(x[1].type(), fetch::script::VariantType::FLOATING_POINT);

  std::cout << x << std::endl;
}

TEST(Streaming_gtest, variant_list)
{
  VariantArray x(6);
  EXPECT_EQ(x.size(), 6);

  x[0] = 1.2;
  x[1] = "Hello world";
  x[2] = 2;
  x[3] = true;
  x[5] = nullptr;

  std::stringstream ss;
  ss.str("");
  ss << x;
  EXPECT_EQ(ss.str(), "[1.2, \"Hello world\", 2, true, (undefined), null]");
}

TEST(Streaming_gtest, nested_variants)
{
  Variant x;

  x.MakeArray(2);
  x[0].MakeArray(3);
  x[0][0] = 1;
  x[0][1] = 3;
  x[0][2] = 7;
  x[1]    = 1.23;

  std::stringstream ss;
  ss.str("");
  ss << x;

  EXPECT_EQ(ss.str(), "[[1, 3, 7], 1.23]");
}
