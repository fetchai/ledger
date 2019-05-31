//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "vm_test_suite.hpp"

#include "gmock/gmock.h"

#include <memory>

namespace {

class StateTests : public VmTestSuite
{
};

TEST_F(StateTests, SanityCheck)
{
  static char const *TEXT = R"(
    function main()
    endfunction
  )";

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, AddressSerializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB");
      var state = State<Address>("addr", data);
      state.set(data);
    endfunction
  )";

  EXPECT_CALL(*observer_, Exists("addr"));
  EXPECT_CALL(*observer_, Write("addr", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, AddressDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB");
      var state = State<Address>("addr", data);
    endfunction
  )";

  AddState("addr",
           "000000000000000020000000000000002f351e415c71722c379baac9394a947b8a303927b8b8421fb9466ed"
           "3db1f5683");

  EXPECT_CALL(*observer_, Exists("addr"));
  EXPECT_CALL(*observer_, Read("addr", _, _));
  EXPECT_CALL(*observer_, Write("addr", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, MapSerializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Map<String, String>();
      var state = State<Map<String, String>>("map", data);
      state.set(data);
  endfunction
  )";

  EXPECT_CALL(*observer_, Exists("map"));
  EXPECT_CALL(*observer_, Write("map", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, MapDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Map<String, String>();
      var state = State<Map<String, String>>("map", data);
  endfunction
  )";

  AddState("map", "0000000000000000");

  EXPECT_CALL(*observer_, Exists("map"));
  EXPECT_CALL(*observer_, Read("map", _, _));
  EXPECT_CALL(*observer_, Write("map", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, ArraySerializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Float32>(10);
      var state = State<Array<Float32>>("state", Array<Float32>(0));
      state.set(data);
    endfunction
  )";

  EXPECT_CALL(*observer_, Exists("state"));
  EXPECT_CALL(*observer_, Write("state", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

TEST_F(StateTests, ArrayDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Float32>(10);
      var state = State<Array<Float32>>("state", Array<Float32>(0));
    endfunction
  )";

  AddState("state",
           "0c000a000000000000000000000000000000000000000000000000000000000000000000000000000000000"
           "0000000000000");

  EXPECT_CALL(*observer_, Exists("state"));
  EXPECT_CALL(*observer_, Read("state", _, _));
  EXPECT_CALL(*observer_, Write("state", _, _));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

// Regression test for issue 1072: used to segfault prior to fix
TEST_F(StateTests, querying_resource_using_a_null_address_fails_gracefully)
{
  static char const *TEXT = R"(
    function main()
      var nullAddress : Address;
      var supply = State<Float64>(nullAddress, 0.0);
      supply.get();
    endfunction
  )";

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_FALSE(Run());
}

TEST_F(StateTests, querying_resource_from_using_a_null_string_as_the_name_fails_gracefully)
{
  static char const *TEXT = R"(
    function main()
      var nullName : String;
      var supply = State<Float64>(nullName, 0.0);
      supply.get();
    endfunction
  )";

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_FALSE(Run());
}

}  // namespace
