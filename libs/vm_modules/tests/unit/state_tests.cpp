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

namespace {

class StateTests : public VmTestSuite
{
protected:
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

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_TRUE(Run());
}

// Regression test for issue 1072: used to segfault prior to fix
TEST_F(StateTests, querying_resource_from_nonexistent_address_fails_gracefully)
{
  static char const *TEXT = R"(
    function main()
      // null default just to be able to call the following init
      var ownerAddressVoid : Address;

      var ownerAddress = State<Address>('does_not_exist', ownerAddressVoid);
      var supply = State<Float64>(ownerAddress.get(), 0.0);
      supply.get();
    endfunction
  )";

  EXPECT_CALL(*observer_, Exists("does_not_exist"));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_FALSE(Run());
}

TEST_F(StateTests, Crash)//???
{
  static char const *TEXT = R"(
    function main()
      var null_str : String;
//print(null_str.length())
      var string_state = State<String>('does_not_exist', null_str);
      var supply = State<Float64>(string_state.get(), 0.0);
      supply.get();
    endfunction
  )";

  EXPECT_CALL(*observer_, Exists("does_not_exist"));

  ASSERT_TRUE(Compile(TEXT));
  ASSERT_FALSE(Run());
}

}  // namespace
