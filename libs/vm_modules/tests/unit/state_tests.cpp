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

namespace fetch {
namespace vm {

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
    function store_address(state_name: String, value: Address)
      var state = State<Address>(state_name, value);
    endfunction

    function main() : Address
      var state_name = "addr";
      var ref_address = Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB");

      store_address(state_name, ref_address);

      var state = State<Address>(state_name, Address());
      return state.get();
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Exists("addr")).Times(2);
  EXPECT_CALL(toolkit.observer(), Read("addr", _, _));
  EXPECT_CALL(toolkit.observer(), Write("addr", _, _)).Times(2);

  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const addr = res.Get<Ptr<Address>>();
  EXPECT_EQ("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB", addr->AsString()->str);
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
  EXPECT_CALL(toolkit.observer(), Write("map", _, _));

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
  EXPECT_CALL(toolkit.observer(), Write("state", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

// Regression test for issue 1072: used to segfault prior to fix
TEST_F(StateTests, querying_state_constructed_from_null_address_fails_gracefully)
{
  static char const *TEXT = R"(
    function main()
      var nullAddress : Address;
      var supply = State<Float64>(nullAddress, 0.0);
      supply.get();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(StateTests, querying_state_constructed_from_null_string_fails_gracefully)
{
  static char const *TEXT = R"(
    function main()
      var nullName : String;
      var supply = State<Float64>(nullName, 0.0);
      supply.get();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant output;
  ASSERT_FALSE(toolkit.Run(&output));
}

TEST_F(StateTests, serialising_compound_object_with_null_values_does_not_segfault)
{
  static char const *TEXT = R"(
    function main()
      var default_array = Array<Array<UInt64>>(2);
      var bids = State<Array<Array<UInt64>>>("state_label", default_array);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(StateTests, DISABLED_test_serialisation_of_complex_type)
{
  static char const *TEXT = R"(
    function store_array_in_state(state_name:String, state_value: Array<String>)
      var state = State<Array<String>>(state_name, state_value);
    endfunction

    function main() : Array<String>
      var ref_array = Array<String>(3);
      ref_array[0] = "aaa";
      ref_array[1] = "bbb";
      ref_array[2] = "ccc";

      var ref_state_name = "my array";

      store_array_in_state(ref_state_name, ref_array);

      var retrieved_state = State<Array<String>>(ref_state_name, Array<String>(0));
      return retrieved_state.get();
    endfunction
  )";

  std::string const state_name{"my array"};
  EXPECT_CALL(toolkit.observer(), Exists(state_name)).Times(2);
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _));
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));

  Variant output;
  ASSERT_TRUE(toolkit.Run(&output));
  ASSERT_FALSE(output.IsPrimitive());

  auto retval{output.Get<Ptr<IArray>>()};
  ASSERT_TRUE(static_cast<bool>(retval));

  // EXPECT_EQ("PASSED", retval->str);
}

}  // namespace
}  // namespace vm
}  // namespace fetch
