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

using fetch::vm::Address;
using fetch::vm::IArray;
using fetch::vm::Ptr;

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

template <typename T>
std::enable_if_t<!IsPtr<T>::value> ArrayFromVariant(Variant const &array, int32_t expected_size,
                                                    Ptr<Array<T>> &out)
{
  out = array.Get<Ptr<Array<T>>>();
  ASSERT_TRUE(out);
  ASSERT_EQ(expected_size, out->Count());
}

template <typename T>
std::enable_if_t<IsPtr<T>::value &&
                 std::is_same<IArray, std::decay_t<typename GetManagedType<T>::type>>::value>
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
  EXPECT_CALL(toolkit.observer(), Read(state_name, _, _)).Times(2);

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

  EXPECT_EQ(std::string{"aaa"}, arr_0_0->PopFrontOne().Get<Ptr<String>>()->str);
  EXPECT_EQ(std::string{"bbb"}, arr_0_1->PopFrontOne().Get<Ptr<String>>()->str);
  EXPECT_EQ(std::string{"ccc"}, arr_1_0->PopFrontOne().Get<Ptr<String>>()->str);
  EXPECT_EQ(std::string{"ddd"}, arr_1_0->PopFrontOne().Get<Ptr<String>>()->str);
  EXPECT_EQ(std::string{"eee"}, arr_1_1->PopFrontOne().Get<Ptr<String>>()->str);
  EXPECT_EQ(std::string{"fff"}, arr_1_1->PopFrontOne().Get<Ptr<String>>()->str);
}

// TEST_F(StateTests, append1)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<UInt32>(2);
//      data[1] = 1u32;
//
//      print(data.count());
//      print('-');
//
//      data.append(2u32);
//
//      print(data.count());
//      print('-');
//
//      print(data);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "2-3-[0, 1, 2]");
//}
//
// TEST_F(StateTests, append2)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<UInt32>(1);
//      data.append(2i32);
//      print(data);
//    endfunction
//  )";
//
//  ASSERT_FALSE(Compile(TEXT));
//}
//
// TEST_F(StateTests, append3)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array< Array<Int32>>(1);
//
//      print(data.count());
//      print('-');
//
//      data.append(Array<Int32>(1));
//      data.append(Array<Int32>(2));
//      print(data.count());
//      print('-');
//
//      data[0] = Array<Int32>(1);
//      data[0].append(123);
//      print(data[0].count());
//      print('-');
//      print(data.count());
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "1-3-2-3");
//}
//
// TEST_F(StateTests, append4)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array< Array<UInt32>>(1);
//      data[0] = Array<UInt32>(1);
//      data.append(Array<Int16>(1));
//    endfunction
//  )";
//
//  ASSERT_FALSE(Compile(TEXT));
//}
//
// TEST_F(StateTests, pop_back1)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<Int32>(3);
//      data[0] = 10; data[1]=20; data[2]=30;
//
//      print(data.count());
//      print('-');
//      var popped = data.pop_back();
//
//      print(data.count());
//      print('-');
//      print(popped);
//      print('-');
//      print(data);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "3-2-30-[10, 20]");
//}
//
// TEST_F(StateTests, pop_back2)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<Array<Int32>>(3);
//      data[0] = Array<Int32>(1); data[1] = Array<Int32>(1); data[2] = Array<Int32>(1);
//      data[0][0]=10; data[1][0]=20; data[2][0]=30;
//
//      print(data.count());
//      print('-');
//      var popped = data.pop_back();
//
//      print(data.count());
//      print('-');
//      print(popped);
//      print('-');
//      print(data[0]);
//      print('-');
//      print(data[1]);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "3-2-[30]-[10]-[20]");
//}
//
// TEST_F(StateTests, pop_back3)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<Int32>(1);
//      data.pop_back();
//      data.pop_back();
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_FALSE(Run());  //???assert on error
//}
//
// TEST_F(StateTests, pop_front1)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<Int32>(3);
//      data[0] = 10; data[1] = 20; data[2] = 30;
//
//      print(data.count());
//      print('-');
//      var popped = data.pop_front();
//
//      print(data.count());
//      print('-');
//      print(popped);
//      print('-');
//      print(data);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "3-2-10-[20, 30]");
//}
//
// TEST_F(StateTests, pop_front2)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<Array<Int32>>(3);
//      data[0] = Array<Int32>(1); data[1] = Array<Int32>(1); data[2] = Array<Int32>(1);
//      data[0][0] = 10; data[1][0] = 20; data[2][0] = 30;
//
//      print(data.count());
//      print('-');
//      var popped = data.pop_front();
//
//      print(data.count());
//      print('-');
//      print(popped);
//      print('-');
//      print(data[0]);
//      print('-');
//      print(data[1]);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "3-2-[10]-[20]-[30]");
//}
//
// TEST_F(StateTests, pop_front3)
//{
//  static char const *TEXT = R"(
//    function main()
//      var data = Array<Int32>(1);
//      data.pop_front();
//      data.pop_front();
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_FALSE(Run());  //???assert on error
//}
//
// TEST_F(StateTests, string_length1)  //???surplus to reqs
//{
//  static char const *TEXT = R"(
//    function main()
//      var text1 = 'abc';
//      var text2 = 'abcdefgh';
//      print(text1.length());
//      print('-');
//      print(text2.length());
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "3-8");
//}
//
// TEST_F(StateTests, trim1)  //???trim should be mutating or constant?
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = ' abc def  ';
//      text.trim();
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "abc def");
//}
//
// TEST_F(StateTests, trim2)  //???trim should be mutating or constant?
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abc def';
//      text.trim();
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "abc def");
//}
//
// TEST_F(StateTests, trim3)  //???trim should be mutating or constant?
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = '   abc def';
//      text.trim();
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "abc def");
//}
//
// TEST_F(StateTests, trim4)  //???trim should be mutating or constant?
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abc def  ';
//      text.trim();
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "abc def");
//}
//
// TEST_F(StateTests, trim5)  //???trim should be mutating or constant?
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = '   ';
//      text.trim();
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "");
//}
//
// TEST_F(StateTests, trim6)  //???trim should be mutating or constant?
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = '';
//      text.trim();
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "");
//}
//
// TEST_F(StateTests, find1)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var output = Array<Int32>(3);
//      output[0] = text.find('bb');
//      output[1] = text.find('bc');
//      output[2] = text.find('x');
//
//      print(output);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "[1, 3, -1]");
//}
//
// TEST_F(StateTests, find2)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var index = text.find('');
//
//      print(index);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "-1");
//}
//
// TEST_F(StateTests, find3)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = '';
//
// var index = text.find('');
//
//      print(index);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "-1");
//}
//
// TEST_F(StateTests, find4)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = '';
//
// var index = text.find('xyz');
//
//      print(index);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "-1");
//}
//
// TEST_F(StateTests, reverse1)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'xyz';
//
// text.reverse();
//
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "zyx");
//}
//
// TEST_F(StateTests, reverse2)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = '';
//
// text.reverse();
//
//      print(text);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_TRUE(stdout().empty());
//}
//
// TEST_F(StateTests, substring1)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var sub = text.substr(1, 3);
//      print(sub);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "bb");
//}
//
// TEST_F(StateTests, substring2)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var sub = text.substr(0, 0);
//      print(sub);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "");
//}
//
// TEST_F(StateTests, substring3)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var sub = text.substr(1, 1);
//      print(sub);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "");
//}
//
// TEST_F(StateTests, substring4)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var sub = text.substr(0, 5);
//      print(sub);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_TRUE(Run());
//
//  ASSERT_EQ(stdout(), "abbbc");
//}
//
// TEST_F(StateTests, substring5)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var sub = text.substr(-1, 1);
//      print(sub);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_FALSE(Run());
//}
//
// TEST_F(StateTests, substring6)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var sub = text.substr(0, 10000);
//      print(sub);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_FALSE(Run());
//}
//
// TEST_F(StateTests, substring7)
//{
//  static char const *TEXT = R"(
//    function main()
//      var text = 'abbbc';
//
// var sub = text.substr(3, 2);
//      print(sub);
//    endfunction
//  )";
//
//  ASSERT_TRUE(Compile(TEXT));
//  ASSERT_FALSE(Run());
//}
//
////TEST_F(StateTests, split1)
////{
////  static char const *TEXT = R"(
////    function main()
////      var text = 'a-b--c--d';
////
////var output= text.split('--');
//////var output= 'a-b--c--d'.split('--');// ???
////      print(output);
//////      print('-');
//////      print(output.count());//???cant print?
////    endfunction
////  )";
////
////  ASSERT_TRUE(Compile(TEXT));
////  ASSERT_TRUE(Run());
////
////  ASSERT_EQ(stdout(), "[a-b, c, d]");
////}
////
////TEST_F(StateTests, split2)
////{
////  static char const *TEXT = R"(
////    function main()
////      var text = 'a-b--c--d';
////
////var output= text.split('-');
//////var output= 'a-b--c--d'.split('-');// ???
////      print(output);
//////      print('-');
//////      print(output.count());//???cant print?
////    endfunction
////  )";
////
////  ASSERT_TRUE(Compile(TEXT));
////  ASSERT_TRUE(Run());
////
////  ASSERT_EQ(stdout(), "[a-b, c, d]");
////}
//
////???is there whitespace other than space in etch?
//
//// TEST_F(StateTests, extend1)
////{
////  static char const *TEXT = R"(
////    function main()
////      var data1 = Array<Int32>(2);
////      var data2 = Array<Int32>(3);
////      data1[0]=1; data1[1]=2;
////      data2[0]=5; data2[1]=4; data2[2]=3;
////
////      data1.extend(data2);
////
////   print(data1);
////   endfunction
////  )";
////
////  ASSERT_TRUE(Compile(TEXT));
////  ASSERT_TRUE(Run());
////
////  ASSERT_EQ(stdout(), "[1, 2, 5, 4, 3]");
////}
//
//// TEST_F(StateTests, anomalous_print_behaviour)
////{
////  static char const *TEXT = R"(
////    function main()
////      var data1 = Array<UInt32>(0);
////      var data2 = Array< Array<UInt32>>(1);
//////      data2[0]=Array<UInt32>(0);
//////      print(data1);//prints []
////      print(data2[0]);//prints nothing unless data2[0]=Array<UInt32>(0);
////// but Array<UInt32>(1) prints as [0]
////    endfunction
////  )";
////
////  ASSERT_TRUE(Compile(TEXT));
////  ASSERT_TRUE(Run());
////
////   ASSERT_EQ(stdout(), "[0, 1, 2]");
////}
////
////
//// TEST_F(StateTests, Bar_bad_alloc)
////{
////  static char const *TEXT = R"(
////    function main()
////      var data = Array<UInt32>(-1u32);//???std::bad_alloc
////    endfunction
////  )";
////
////  ASSERT_TRUE(Compile(TEXT));
////  ASSERT_TRUE(Run());
////
////   ASSERT_EQ(stdout(), "[0, 1, 2]");
////}
////???join on arrays, split on strings, reverse on both?
////
////???documentation - identifiers cannot begin with a digit
////???docs - integer literals default to Int32
////write print tests
////???sprawdzic - czy 1u ma typ UInt32?
////???are strings iterable?
//// cannot print array of Int16
////???is fn overloading supported in etch?
////???extend, popbackmulti and popfrontmulti
////???move some array stuff to cpp file
////???benchmarks?
//// int32 functions try not to lose precision
}  // namespace
