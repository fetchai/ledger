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
      var state = State<Address>("addr");
      state.set(data);
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("addr", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(StateTests, AddressDeserializeTest)
{
  static char const *TEXT = R"(
    function store_address(state_name: String, value: Address)
      var state = State<Address>(state_name);
      state.set(value);
    endfunction

    function main() : Address
      var state_name = "addr";
      var ref_address = Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB");

      store_address(state_name, ref_address);

      var state = State<Address>(state_name);
      return state.get();
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("addr", _, _));
  EXPECT_CALL(toolkit.observer(), Exists("addr"));
  EXPECT_CALL(toolkit.observer(), Read("addr", _, _));

  ASSERT_TRUE(toolkit.Compile(TEXT));
  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const addr = res.Get<Ptr<Address>>();
  EXPECT_EQ("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB", addr->AsString()->str);
}

TEST_F(StateTests, MapDeserializeTest)
{
  static char const *ser_src = R"(
    function main()
      var data = Map<String, String>();
      var state = State<Map<String, String>>("map");
      state.set(data);
  endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("map", _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Map<String, String>
      var state = State<Map<String, String>>("map");
      return state.get(Map<String, String>());
  endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Exists("map"));
  EXPECT_CALL(toolkit.observer(), Read("map", _, _));

  ASSERT_TRUE(toolkit.Compile(deser_src));
  Variant ret;
  ASSERT_TRUE(toolkit.Run(&ret));
  auto const map{ret.Get<Ptr<IMap>>()};
  EXPECT_TRUE(static_cast<bool>(map));
}

TEST_F(StateTests, ArrayDeserializeTest)
{
  static char const *ser_src = R"(
    function main()
      var data = Array<Float64>(3);
      data[0] = 0.1;
      data[1] = 2.3;
      data[2] = 4.5;

      State<Array<Float64>>("state").set(data);
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("state", _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Array<Float64>
      var state = State<Array<Float64>>("state");
      return state.get(Array<Float64>(0));
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Exists("state"));
  EXPECT_CALL(toolkit.observer(), Read("state", _, _));

  ASSERT_TRUE(toolkit.Compile(deser_src));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));
  ASSERT_TRUE(!res.IsPrimitive());

  auto array{res.Get<Ptr<IArray>>()};
  ASSERT_TRUE(static_cast<bool>(array));
  ASSERT_EQ(int32_t{3}, array->Count());
  EXPECT_EQ(0.1, array->PopFrontOne().Get<double>());
  EXPECT_EQ(2.3, array->PopFrontOne().Get<double>());
  EXPECT_EQ(4.5, array->PopFrontOne().Get<double>());
}

// Regression test for issue 1072: used to segfault prior to fix
TEST_F(StateTests, querying_state_constructed_from_null_address_fails_gracefully)
{
  static char const *TEXT = R"(
    function main() : Float64
      var nullAddress : Address;
      var supply = State<Float64>(nullAddress);
      supply.set(3.7);
      return supply.get(0.0);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(StateTests, querying_state_constructed_from_null_string_fails_gracefully)
{
  static char const *TEXT = R"(
    function main() : Float64
      var nullName : String;
      var supply = State<Float64>(nullName);
      supply.set(3.7);
      return supply.get(0.0);
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
      State<Array<Array<UInt64>>>("state_label").set(default_array);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(StateTests, test_serialisation_of_complex_type)
{
  static char const *ser_src = R"(
    function main()
      var ref_array = Array<String>(3);
      ref_array[0] = "aaa";
      ref_array[1] = "bbb";
      ref_array[2] = "ccc";

      var state = State<Array<String>>("my array");
      state.set(ref_array);
    endfunction
  )";

  std::string const state_name{"my array"};
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Array<String>
      var retrieved_state = State<Array<String>>("my array");
      return retrieved_state.get(Array<String>(0));
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(deser_src));

  Variant output;
  ASSERT_TRUE(toolkit.Run(&output));
  ASSERT_FALSE(output.IsPrimitive());
  auto retval{output.Get<Ptr<IArray>>()};
  ASSERT_TRUE(static_cast<bool>(retval));
  ASSERT_EQ(int32_t{3}, retval->Count());
  EXPECT_EQ(std::string{"aaa"}, retval->PopFrontOne().Get<Ptr<String>>()->str);
  EXPECT_EQ(std::string{"bbb"}, retval->PopFrontOne().Get<Ptr<String>>()->str);
  EXPECT_EQ(std::string{"ccc"}, retval->PopFrontOne().Get<Ptr<String>>()->str);
}

}  // namespace
}  // namespace vm
}  // namespace fetch
