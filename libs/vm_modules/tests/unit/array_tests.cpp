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

namespace {

class ArrayTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(ArrayTests, count_returns_the_number_of_elements_in_the_array)
{
  static char const *TEXT = R"(
    function main()
      print(Array<UInt32>(2).count());
      print('-');
      print(Array<Array<UInt32>>(5).count());
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "2-5");
}

TEST_F(ArrayTests, count_returns_zero_if_the_array_is_empty)
{
  static char const *TEXT = R"(
    function main()
      print(Array<UInt32>(0).count());
      print('-');
      print(Array<Array<UInt32>>(0).count());
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0-0");
}

TEST_F(ArrayTests, append_adds_one_element_at_the_end_of_the_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<UInt32>(2);
      data[0] = 1u32;
      data[1] = 2u32;

      data.append(42u32);

      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[1, 2, 42]");
}

TEST_F(ArrayTests, append_is_statically_type_safe_with_numeric_arrays)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<UInt32>(1);
      data.append(2u16);
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(ArrayTests, append_accepts_objects)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Array<Int32>>(1);

      print(data.count());
      print('-');

      data.append(Array<Int32>(1));
      data.append(Array<Int32>(2));
      print(data.count());
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "1-3");
}

TEST_F(ArrayTests, append_is_statically_type_safe_with_object_arrays)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Array<UInt32>>(1);
      data[0] = Array<UInt32>(1);
      data.append(Array<Int16>(1));
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(ArrayTests, popBack_removes_the_last_element_and_returns_it)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popBack();

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "30-[10, 20]");
}

TEST_F(ArrayTests, popBack_works_with_arrays_of_objects)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Array<Int32>>(3);
      data[0] = Array<Int32>(1);
      data[1] = Array<Int32>(1);
      data[2] = Array<Int32>(1);
      data[0][0]=10; data[1][0]=20; data[2][0]=30;

      print(data.count());
      print('-');
      var popped = data.popBack();

      print(data.count());
      print('-');
      print(popped);
      print('-');
      print(data[0]);
      print('-');
      print(data[1]);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "3-2-[30]-[10]-[20]");
}

TEST_F(ArrayTests, popBack_fails_if_array_is_empty)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(0);
      data.popBack();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests,
       when_passed_an_integer_N_popBack_removes_the_last_N_elements_and_returns_them_as_an_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popBack(2);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[20, 30]-[10]");
}

TEST_F(ArrayTests, when_passed_an_integer_N_popBack_works_for_arrays_of_objects)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Array<Int32>>(3);
      data[0] = Array<Int32>(1);
      data[1] = Array<Int32>(1);
      data[2] = Array<Int32>(1);
      data[0][0]=10; data[1][0]=20; data[2][0]=30;

      print(data.count());
      print('-');
      var popped = data.popBack(2);

      print(data.count());
      print('-');
      print(popped.count());
      print('-');
      print(popped[0]);
      print('-');
      print(popped[1]);
      print('-');
      print(data[0]);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "3-1-2-[20]-[30]-[10]");
}

TEST_F(ArrayTests, when_passed_zero_popBack_does_not_mutate_its_array_and_returns_an_empty_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popBack(0);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[]-[10, 20, 30]");
}

TEST_F(ArrayTests, when_passed_a_negative_number_popBack_fails)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popBack(-3);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests, popFront_removes_the_first_element_and_returns_it)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popFront();

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "10-[20, 30]");
}

TEST_F(ArrayTests, popFront_works_with_arrays_of_objects)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Array<Int32>>(3);
      data[0] = Array<Int32>(1);
      data[1] = Array<Int32>(1);
      data[2] = Array<Int32>(1);
      data[0][0] = 10; data[1][0] = 20; data[2][0] = 30;

      print(data.count());
      print('-');
      var popped = data.popFront();

      print(data.count());
      print('-');
      print(popped);
      print('-');
      print(data[0]);
      print('-');
      print(data[1]);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "3-2-[10]-[20]-[30]");
}

TEST_F(ArrayTests, popFront_fails_if_array_is_empty)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(0);
      data.popFront();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests,
       when_passed_an_integer_N_popFront_removes_the_last_N_elements_and_returns_them_as_an_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popFront(2);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[10, 20]-[30]");
}

TEST_F(ArrayTests, when_passed_an_integer_N_popFront_works_for_arrays_of_objects)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Array<Int32>>(3);
      data[0] = Array<Int32>(1);
      data[1] = Array<Int32>(1);
      data[2] = Array<Int32>(1);
      data[0][0]=10; data[1][0]=20; data[2][0]=30;

      print(data.count());
      print('-');
      var popped = data.popFront(2);

      print(data.count());
      print('-');
      print(popped.count());
      print('-');
      print(popped[0]);
      print('-');
      print(popped[1]);
      print('-');
      print(data[0]);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "3-1-2-[10]-[20]-[30]");
}

TEST_F(ArrayTests, when_passed_zero_popFront_does_not_mutate_its_array_and_returns_an_empty_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popFront(0);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[]-[10, 20, 30]");
}

TEST_F(ArrayTests, when_passed_a_negative_number_popFront_fails)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.popFront(-3);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests, reverse_inverts_the_order_of_elements)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      data.reverse();

      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[30, 20, 10]");
}

TEST_F(ArrayTests, reverse_of_an_empty_array_is_a_noop)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(0);
      data.reverse();

      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[]");
}

TEST_F(ArrayTests, extend_appends_the_elements_of_the_argument_array_in_order)
{
  static char const *TEXT = R"(
    function main()
      var data1 = Array<Int32>(3);
      data1[0] = 1;
      data1[1] = 2;
      data1[2] = 3;
      var data2 = Array<Int32>(2);
      data2[0] = 5;
      data2[1] = 4;

      data1.extend(data2);

      print(data1);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[1, 2, 3, 5, 4]");
}

TEST_F(ArrayTests, extend_called_with_an_empty_array_is_a_noop)
{
  static char const *TEXT = R"(
    function main()
      var data1 = Array<Int32>(3);
      data1[0] = 1;
      data1[1] = 2;
      data1[2] = 3;
      var data2 = Array<Int32>(0);

      data1.extend(data2);

      print(data1);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[1, 2, 3]");
}

TEST_F(ArrayTests, extend_fails_if_called_with_an_array_of_different_type)
{
  static char const *TEXT = R"(
    function main()
      var data1 = Array<Int32>(1);
      data1[0] = 1;
      var data2 = Array<UInt64>(1);
      data2[0] = 1;

      data1.extend(data2);

      print(data1);
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

TEST_F(ArrayTests, extend_does_not_mutate_its_argument)
{
  static char const *TEXT = R"(
    function main()
      var data1 = Array<Int32>(2);
      data1[0] = 10;
      data1[1] = 20;
      var data2 = Array<Int32>(3);
      data2[0] = 50;
      data2[1] = 40;
      data2[2] = 30;

      data1.extend(data2);

      print(data2);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[50, 40, 30]");
}

TEST_F(ArrayTests, erase_removes_the_element_pointed_to_by_the_index)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(5);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;
      data[3] = 40;
      data[4] = 50;

      data.erase(3);
      print(data);
      data.erase(1);
      print(data);
      data.erase(0);
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "[10, 20, 30, 50][10, 30, 50][30, 50]");
}

TEST_F(ArrayTests, erase_fails_if_index_exceeds_size)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(2);
      data[0] = 10;
      data[1] = 20;

      data.erase(3);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests, erase_fails_if_index_is_equal_to_size)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(2);
      data[0] = 10;
      data[1] = 20;

      data.erase(data.count());
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests, erase_fails_if_array_is_empty)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(0);

      data.erase(0);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests, erase_fails_if_index_is_negative)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(2);
      data[0] = 10;
      data[1] = 20;

      data.erase(-2);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests, array_expressions)
{
  static char const *TEXT = R"(
function main()
   var w = [[], [42; 3], [], [2]];
   for(i in 0:w.count() - 1)
     print(i);
     print('->');
     print(w[i]);
     print(';');
   endfor
   var x: Array<Float64> = [];
   print(x);
   x = [3.14; w[3][0]];
   print(x);
endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0->[];1->[42, 42, 42];2->[];3->[2];[][3.14, 3.14]");
}

TEST_F(ArrayTests, failed_array_expressions)
{
  static char const *untyped_empty_array = R"(
function main()
  var w = [];
endfunction
)";
  ASSERT_FALSE(toolkit.Compile(untyped_empty_array));

  static char const *mismatched_type = R"(
function main()
  var x: Int32 = [];
endfunction
)";
  ASSERT_FALSE(toolkit.Compile(mismatched_type));

  static char const *heterogeneous_elements = R"(
function main()
  var w = [1, 3.14, [0]];
endfunction
)";
  ASSERT_FALSE(toolkit.Compile(heterogeneous_elements));
}

}  // namespace
