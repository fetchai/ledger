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

namespace {

class ArrayTests : public ::testing::Test
{
public:
  VmTestToolkit toolkit;
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

  ASSERT_EQ(toolkit.stdout(), "2-5");
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

  ASSERT_EQ(toolkit.stdout(), "0-0");
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

  ASSERT_EQ(toolkit.stdout(), "[1, 2, 42]");
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

  ASSERT_EQ(toolkit.stdout(), "1-3");
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

TEST_F(ArrayTests, pop_back_removes_the_last_element_and_returns_it)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_back();

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "30-[10, 20]");
}

TEST_F(ArrayTests, pop_back_works_with_arrays_of_objects)
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
      var popped = data.pop_back();

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

  ASSERT_EQ(toolkit.stdout(), "3-2-[30]-[10]-[20]");
}

TEST_F(ArrayTests, pop_back_fails_if_array_is_empty)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(0);
      data.pop_back();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests,
       when_passed_an_integer_N_pop_back_removes_the_last_N_elements_and_returns_them_as_an_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_back(2);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[20, 30]-[10]");
}

TEST_F(ArrayTests, when_passed_an_integer_N_pop_back_works_for_arrays_of_objects)
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
      var popped = data.pop_back(2);

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

  ASSERT_EQ(toolkit.stdout(), "3-1-2-[20]-[30]-[10]");
}

TEST_F(ArrayTests, when_passed_zero_pop_back_does_not_mutate_its_array_and_returns_an_empty_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_back(0);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[]-[10, 20, 30]");
}

TEST_F(ArrayTests, when_passed_a_negative_number_pop_back_fails)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_back(-3);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests, pop_front_removes_the_first_element_and_returns_it)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_front();

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "10-[20, 30]");
}

TEST_F(ArrayTests, pop_front_works_with_arrays_of_objects)
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
      var popped = data.pop_front();

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

  ASSERT_EQ(toolkit.stdout(), "3-2-[10]-[20]-[30]");
}

TEST_F(ArrayTests, pop_front_fails_if_array_is_empty)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(0);
      data.pop_front();
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_FALSE(toolkit.Run());
}

TEST_F(ArrayTests,
       when_passed_an_integer_N_pop_front_removes_the_last_N_elements_and_returns_them_as_an_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_front(2);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[10, 20]-[30]");
}

TEST_F(ArrayTests, when_passed_an_integer_N_pop_front_works_for_arrays_of_objects)
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
      var popped = data.pop_front(2);

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

  ASSERT_EQ(toolkit.stdout(), "3-1-2-[10]-[20]-[30]");
}

TEST_F(ArrayTests, when_passed_zero_pop_front_does_not_mutate_its_array_and_returns_an_empty_array)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_front(0);

      print(popped);
      print('-');
      print(data);
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(toolkit.stdout(), "[]-[10, 20, 30]");
}

TEST_F(ArrayTests, when_passed_a_negative_number_pop_front_fails)
{
  static char const *TEXT = R"(
    function main()
      var data = Array<Int32>(3);
      data[0] = 10;
      data[1] = 20;
      data[2] = 30;

      var popped = data.pop_front(-3);
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

  ASSERT_EQ(toolkit.stdout(), "[30, 20, 10]");
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

  ASSERT_EQ(toolkit.stdout(), "[]");
}

}  // namespace
