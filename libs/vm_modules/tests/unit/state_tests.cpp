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

#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <cstddef>

namespace {

class StateTests : public ::testing::Test
{
public:
  VmTestToolkit toolkit;
};

TEST_F(StateTests, SanityCheck)
{
  static char const *TEXT = R"(
    function main()
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
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

  EXPECT_CALL(toolkit.observer(), Exists("addr"));
  EXPECT_CALL(toolkit.observer(), Write("addr", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(StateTests, AddressDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB");
      var state = State<Address>("addr", data);
    endfunction
  )";

  toolkit.AddState(
      "addr",
      "000000000000000020000000000000002f351e415c71722c379baac9394a947b8a303927b8b8421fb9466ed"
      "3db1f5683");

  EXPECT_CALL(toolkit.observer(), Exists("addr"));
  EXPECT_CALL(toolkit.observer(), Read("addr", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
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

  EXPECT_CALL(toolkit.observer(), Exists("map"));
  EXPECT_CALL(toolkit.observer(), Write("map", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(StateTests, MapDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Map<String, String>();
      var state = State<Map<String, String>>("map", data);
  endfunction
  )";

  toolkit.AddState("map", "0000000000000000");

  EXPECT_CALL(toolkit.observer(), Exists("map"));
  EXPECT_CALL(toolkit.observer(), Read("map", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
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

  EXPECT_CALL(toolkit.observer(), Exists("state"));
  EXPECT_CALL(toolkit.observer(), Write("state", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(StateTests, ArrayDeserializeTest)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Float32>(10);
      var state = State<Array<Float32>>("state", Array<Float32>(0));
    endfunction
  )";

  toolkit.AddState(
      "state",
      "0c000a000000000000000000000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000");

  EXPECT_CALL(toolkit.observer(), Exists("state"));
  EXPECT_CALL(toolkit.observer(), Read("state", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

}  // namespace
