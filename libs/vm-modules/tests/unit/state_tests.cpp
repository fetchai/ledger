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

#include "vm/address.hpp"
#include "vm/array.hpp"
#include "vm/fixed.hpp"
#include "vm/map.hpp"
#include "vm_modules/core/byte_array_wrapper.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

using namespace fetch::vm;
using fetch::vm_modules::ByteArrayWrapper;

namespace {

class StateTests : public ::testing::Test
{
public:
  std::ostringstream out;
  VmTestToolkit      toolkit{&out};
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

TEST_F(StateTests, AddressSerialisationTest)
{
  static char const *ser_src = R"(
    function main()
      State<Address>("addr").set(Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB"));
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("addr", _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Address
      return State<Address>("addr").get();
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Exists("addr"));
  EXPECT_CALL(toolkit.observer(), Read("addr", _, _));

  ASSERT_TRUE(toolkit.Compile(deser_src));

  Variant res;
  ASSERT_TRUE(toolkit.Run(&res));

  auto const addr = res.Get<Ptr<Address>>();
  EXPECT_EQ("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB", addr->AsString()->string());
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
  EXPECT_EQ(std::string{"aaa"}, retval->PopFrontOne().Get<Ptr<String>>()->string());
  EXPECT_EQ(std::string{"bbb"}, retval->PopFrontOne().Get<Ptr<String>>()->string());
  EXPECT_EQ(std::string{"ccc"}, retval->PopFrontOne().Get<Ptr<String>>()->string());
}

template <typename T>
std::enable_if_t<!IsPtr<T>::value> ArrayFromVariant(Variant const &array, int32_t expected_size,
                                                    Ptr<Array<T>> &out)
{
  out = array.Get<Ptr<Array<T>>>();
  ASSERT_TRUE(out);
  ASSERT_EQ(expected_size, out->Count());
}

template <typename T>
std::enable_if_t<IsPtr<T>::value && std::is_same<IArray, std::decay_t<GetManagedType<T>>>::value>
ArrayFromVariant(Variant const &array, int32_t expected_size, Ptr<Array<T>> &out)
{
  out = array.Get<Ptr<Array<T>>>();
  ASSERT_TRUE(out);
  ASSERT_EQ(expected_size, out->Count());
}

TEST_F(StateTests, test_serialisation_of_complex_type_2)
{
  static char const *ser_src = R"(
    function main()
      var ref_array = Array<Array<Array<String>>>(2);
      ref_array[0] = Array<Array<String>>(2);
      ref_array[1] = Array<Array<String>>(2);

      ref_array[0][0] = Array<String>(1);
      ref_array[0][1] = Array<String>(1);

      ref_array[1][0] = Array<String>(2);
      ref_array[1][1] = Array<String>(2);

      ref_array[0][0][0] = "aaa";
      ref_array[0][1][0] = "bbb";

      ref_array[1][0][0] = "ccc";
      ref_array[1][0][1] = "ddd";

      ref_array[1][1][0] = "eee";
      ref_array[1][1][1] = "fff";

      var state = State<Array<Array<Array<String>>>>("my array");
      state.set(ref_array);
    endfunction
  )";

  std::string const state_name{"my array"};
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Array<Array<Array<String>>>
      var state = State<Array<Array<Array<String>>>>("my array");
      return state.get();
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(1);

  ASSERT_TRUE(toolkit.Compile(deser_src));

  Variant output;
  ASSERT_TRUE(toolkit.Run(&output));

  ASSERT_FALSE(output.IsPrimitive());

  Ptr<Array<Ptr<IArray>>> arr;
  ArrayFromVariant(output, int32_t{2}, arr);

  Ptr<Array<Ptr<IArray>>> arr_0;
  ArrayFromVariant(arr->PopFrontOne(), int32_t{2}, arr_0);

  Ptr<Array<Ptr<IArray>>> arr_1;
  ArrayFromVariant(arr->PopFrontOne(), int32_t{2}, arr_1);

  Ptr<Array<Ptr<IArray>>> arr_0_0;
  ArrayFromVariant(arr_0->PopFrontOne(), int32_t{1}, arr_0_0);
  Ptr<Array<Ptr<IArray>>> arr_0_1;
  ArrayFromVariant(arr_0->PopFrontOne(), int32_t{1}, arr_0_1);

  Ptr<Array<Ptr<IArray>>> arr_1_0;
  ArrayFromVariant(arr_1->PopFrontOne(), int32_t{2}, arr_1_0);
  Ptr<Array<Ptr<IArray>>> arr_1_1;
  ArrayFromVariant(arr_1->PopFrontOne(), int32_t{2}, arr_1_1);

  EXPECT_EQ(std::string{"aaa"}, arr_0_0->PopFrontOne().Get<Ptr<String>>()->string());
  EXPECT_EQ(std::string{"bbb"}, arr_0_1->PopFrontOne().Get<Ptr<String>>()->string());
  EXPECT_EQ(std::string{"ccc"}, arr_1_0->PopFrontOne().Get<Ptr<String>>()->string());
  EXPECT_EQ(std::string{"ddd"}, arr_1_0->PopFrontOne().Get<Ptr<String>>()->string());
  EXPECT_EQ(std::string{"eee"}, arr_1_1->PopFrontOne().Get<Ptr<String>>()->string());
  EXPECT_EQ(std::string{"fff"}, arr_1_1->PopFrontOne().Get<Ptr<String>>()->string());
}

TEST_F(StateTests, test_serialisation_of_structured_data)
{
  static char const *ser_src = R"(
    function main(buffer : Buffer)

      var arr_i32 = Array<Int32>(1);
      arr_i32[0] = 10i32;

      var arr_i64 = Array<Int64>(1);
      arr_i64[0] = 14i64;

      var arr_u32 = Array<UInt32>(1);
      arr_u32[0] = 180u32;

      var arr_u64 = Array<UInt64>(1);
      arr_u64[0] = 200u64;

      var data = StructuredData();
      data.set("string", "bar");
      data.set("i32", 256i32);
      data.set("u32", 512u32);
      data.set("i64", 1024i64);
      data.set("u64", 2048u64);
      data.set("address", Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB"));
      data.set("uint256", UInt256(12297829382473034410u64));
      data.set("buffer", buffer);
      data.set("arr_i32", arr_i32);
      data.set("arr_i64", arr_i64);
      data.set("arr_u32", arr_u32);
      data.set("arr_u64", arr_u64);

      var state = State<StructuredData>("state_data");
      state.set(data);
    endfunction
  )";

  ConstByteArray expected_buffer{"QWERTYUIOPasdfghjkl"};

  std::string const state_name{"state_data"};
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(
      toolkit.RunWithParams(nullptr, std::numeric_limits<ChargeAmount>::max(),
                            toolkit.vm().CreateNewObject<ByteArrayWrapper>(expected_buffer)));

  static char const *deser_src = R"(
    function main(buffer : Buffer) : Buffer
      var retrieved_state = State<StructuredData>("state_data");
      var data = retrieved_state.get();

      assert(data.getString("string") == "bar");
      assert(data.getInt32("i32") == 256i32);
      assert(data.getUInt32("u32") == 512u32);
      assert(data.getInt64("i64") == 1024i64);
      assert(data.getUInt64("u64") == 2048u64);
      assert(data.getAddress("address") == Address("MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB"));
      printLn("data.getUInt256(\"uint256\") = " + toString(data.getUInt256("uint256")));
      printLn("UInt256(12297829382473034410u64) = " + toString(UInt256(12297829382473034410u64)));
      assert(data.getUInt256("uint256") == UInt256(12297829382473034410u64));
      //assert(data.getBuffer("buffer") == buffer);

      var arr_i32 = data.getArrayInt32("arr_i32");
      assert(arr_i32.count() == 1);
      assert(arr_i32[0] == 10i32);

      var arr_i64 = data.getArrayInt64("arr_i64");
      assert(arr_i64.count() == 1);
      assert(arr_i64[0] == 14i64);

      var arr_u32 = data.getArrayUInt32("arr_u32");
      assert(arr_u32.count() == 1);
      assert(arr_u32[0] == 180u32);

      var arr_u64 = data.getArrayUInt64("arr_u64");
      assert(arr_u64.count() == 1);
      assert(arr_u64[0] == 200u64);

      return data.getBuffer("buffer");
    endfunction
  )";

  toolkit.setStdout(std::cout);
  EXPECT_CALL(toolkit.observer(), Exists(state_name));
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(2);

  ASSERT_TRUE(toolkit.Compile(deser_src));

  Variant buffer;
  ASSERT_TRUE(
      toolkit.RunWithParams(&buffer, std::numeric_limits<ChargeAmount>::max(),
                            toolkit.vm().CreateNewObject<ByteArrayWrapper>(expected_buffer)));

  auto const acquired_buffer{buffer.Get<Ptr<ByteArrayWrapper>>()};
  EXPECT_TRUE(acquired_buffer);
  EXPECT_EQ(expected_buffer, acquired_buffer->byte_array());
}

TEST_F(StateTests,
       primitive_state_variables_bound_to_the_same_resource_give_consistent_view_of_the_storage)
{
  static char const *TEXT = R"(
    function main()
      var a = State<Int32>("account");
      var b = State<Int32>("account");
      a.set(1);
      b.set(2);
      print(toString(a.get()));
      print(".");
      print(toString(b.get()));
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("account", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Read("account", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Exists("account")).Times(2);

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(out.str(), "2.2");
}

TEST_F(StateTests,
       pointer_state_variables_bound_to_the_same_resource_give_consistent_view_of_the_storage)
{
  static char const *TEXT = R"(
    function main()
      var a = State<String>("name");
      var b = State<String>("name");
      a.set("Alice");
      b.set("Bob");
      print(a.get());
      print(".");
      print(b.get());
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("name", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Read("name", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Exists("name")).Times(2);

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(out.str(), "Bob.Bob");
}

TEST_F(
    StateTests,
    primitive_sharded_state_variables_bound_to_the_same_resource_give_consistent_view_of_the_storage)
{
  static char const *TEXT = R"(
    function main()
      var a = ShardedState<Int32>("account");
      var b = ShardedState<Int32>("account");
      a.set("balance", 1);
      b.set("balance", 2);
      print(toString(a.get("balance")));
      print(".");
      print(toString(b.get("balance")));
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("account.balance", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Read("account.balance", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Exists("account.balance")).Times(2);

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(out.str(), "2.2");
}

TEST_F(
    StateTests,
    pointer_sharded_state_variables_bound_to_the_same_resource_give_consistent_view_of_the_storage)
{
  static char const *TEXT = R"(
    function main()
      var a = ShardedState<String>("personal_info");
      var b = ShardedState<String>("personal_info");
      a.set("name", "Alice");
      b.set("name", "Bob");
      print(a.get("name"));
      print(".");
      print(b.get("name"));
    endfunction
  )";

  EXPECT_CALL(toolkit.observer(), Write("personal_info.name", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Read("personal_info.name", _, _)).Times(2);
  EXPECT_CALL(toolkit.observer(), Exists("personal_info.name")).Times(2);

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(out.str(), "Bob.Bob");
}

TEST_F(StateTests, test_serialisation_of_fixed_point32)
{
  static char const *ser_src = R"(
    function main()
      var ref_array = Array<Fixed32>(3);
      ref_array[0] = 1.0fp32;
      ref_array[1] = 101.01fp32;
      ref_array[2] = 10101.0101fp32;

      var state = State<Array<Fixed32>>("my array");
      state.set(ref_array);
    endfunction
  )";

  std::string const state_name{"my array"};
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Array<Fixed32>
      var retrieved_state = State<Array<Fixed32>>("my array");
      return retrieved_state.get(Array<Fixed32>(0));
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

  EXPECT_EQ(fetch::fixed_point::fp32_t{1.0},
            retval->PopFrontOne().Get<fetch::fixed_point::fp32_t>());
  EXPECT_EQ(fetch::fixed_point::fp32_t{101.01},
            retval->PopFrontOne().Get<fetch::fixed_point::fp32_t>());
  EXPECT_EQ(fetch::fixed_point::fp32_t{10101.0101},
            retval->PopFrontOne().Get<fetch::fixed_point::fp32_t>());
}

TEST_F(StateTests, test_serialisation_of_fixed_point64)
{
  static char const *ser_src = R"(
    function main()
      var ref_array = Array<Fixed64>(3);
      ref_array[0] = 1.0fp64;
      ref_array[1] = 101.01fp64;
      ref_array[2] = 10101.0101fp64;

      var state = State<Array<Fixed64>>("my array");
      state.set(ref_array);
    endfunction
  )";

  std::string const state_name{"my array"};
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Array<Fixed64>
      var retrieved_state = State<Array<Fixed64>>("my array");
      return retrieved_state.get(Array<Fixed64>(0));
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
  EXPECT_EQ(fetch::fixed_point::fp64_t{1.0},
            retval->PopFrontOne().Get<fetch::fixed_point::fp64_t>());
  EXPECT_EQ(fetch::fixed_point::fp64_t{101.01},
            retval->PopFrontOne().Get<fetch::fixed_point::fp64_t>());
  EXPECT_EQ(fetch::fixed_point::fp64_t{10101.0101},
            retval->PopFrontOne().Get<fetch::fixed_point::fp64_t>());
}

TEST_F(StateTests, test_serialisation_of_fixed_point128)
{
  static char const *ser_src = R"(
    function main()
      var ref_array = Array<Fixed128>(3);
      ref_array[0] = 1.0fp128;
      ref_array[1] = 101.01fp128;
      ref_array[2] = 10101.0101fp128;

      var state = State<Array<Fixed128>>("my array");
      state.set(ref_array);
    endfunction
  )";

  std::string const state_name{"my array"};
  EXPECT_CALL(toolkit.observer(), Write(state_name, _, _));

  ASSERT_TRUE(toolkit.Compile(ser_src));
  ASSERT_TRUE(toolkit.Run());

  static char const *deser_src = R"(
    function main() : Array<Fixed128>
      var retrieved_state = State<Array<Fixed128>>("my array");
      return retrieved_state.get(Array<Fixed128>(0));
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
  EXPECT_EQ(fetch::fixed_point::fp128_t{1.0},
            retval->PopFrontOne().Get<Ptr<fetch::vm::Fixed128>>()->data_);
  EXPECT_EQ(fetch::fixed_point::fp128_t{101.01},
            retval->PopFrontOne().Get<Ptr<fetch::vm::Fixed128>>()->data_);
  EXPECT_EQ(fetch::fixed_point::fp128_t{10101.0101},
            retval->PopFrontOne().Get<Ptr<fetch::vm::Fixed128>>()->data_);
}

}  // namespace
