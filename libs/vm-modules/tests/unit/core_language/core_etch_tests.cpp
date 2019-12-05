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

#include "core/string/replace.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <sstream>

namespace {

using namespace testing;

// TODO(WK): extract test helpers library and move this test suite to the libs/vm
class CoreEtchTests : public Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(CoreEtchTests, in_for_loop_break_exits_the_loop)
{
  static char const *TEXT = R"(
    function main()
      for (i in 0u8:6u8)
        if (i == 2u8)
          break;
        endif
        print(i);
      endfor
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "01 end");
}

TEST_F(CoreEtchTests, in_while_loop_break_exits_the_loop)
{
  static char const *TEXT = R"(
    function main()
      var i = 0u8;
      while (i < 5u8)
        if (i == 2u8)
          break;
        endif
        print(i);
        i = i + 1u8;
      endwhile
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "01 end");
}

TEST_F(CoreEtchTests, in_nested_for_loop_break_exits_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      for (j in 0u8:4u8)
        for (i in 0u8:6u8)
          if (i == 2u8)
            break;
          endif
          print(i);
        endfor
        print('_');
      endfor
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "01_01_01_01_ end");
}

TEST_F(CoreEtchTests, in_nested_while_loop_break_exits_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      var j = 0u8;
      while (j < 3u8)
        var i = 0u8;
        while (i < 5u8)
          if (i == 2u8)
            break;
          endif
          print(i);
          i = i + 1u8;
        endwhile
        print('_');
        j = j + 1u8;
      endwhile
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "01_01_01_ end");
}

TEST_F(CoreEtchTests, in_for_loop_inside_a_while_loop_break_exits_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      var j = 0u8;
      while (j < 3u8)
        for (i in 0u8:5u8)
          if (i == 2u8)
            break;
          endif
          print(i);
        endfor
        print('_');
        j = j + 1u8;
      endwhile
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "01_01_01_ end");
}

TEST_F(CoreEtchTests, in_while_loop_inside_a_for_loop_break_exits_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      for (j in 0u8:4u8)
        var i = 0u8;
        while (i < 5u8)
          if (i == 2u8)
            break;
          endif
          print(i);
          i = i + 1u8;
        endwhile
        print('_');
      endfor
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "01_01_01_01_ end");
}

TEST_F(CoreEtchTests, in_for_loop_continue_skips_to_the_next_iteration)
{
  static char const *TEXT = R"(
    function main()
      for (i in 0u8:6u8)
        print(i);
        if (i > 2u8)
          continue;
        endif
        print('.');
      endfor
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0.1.2.345 end");
}

TEST_F(CoreEtchTests, in_while_loop_continue_skips_to_the_next_iteration)
{
  static char const *TEXT = R"(
    function main()
      var i = 0u8;
      while (i < 5u8)
        print(i);
        i = i + 1u8;
        if (i > 2u8)
          continue;
        endif
        print('.');
      endwhile
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0.1.234 end");
}

TEST_F(CoreEtchTests, in_nested_for_loop_continue_skips_to_the_next_iteration_of_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      for (j in 0u8:3u8)
        for (i in 0u8:6u8)
          print(i);
          if (i > 2u8)
            continue;
          endif
          print('.');
        endfor
        print('_');
      endfor
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0.1.2.345_0.1.2.345_0.1.2.345_ end");
}

TEST_F(CoreEtchTests, in_nested_while_loop_continue_skips_to_the_next_iteration_of_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      var j = 0u8;
      while (j < 3u8)
        var i = 0u8;
        while (i < 5u8)
          print(i);
          i = i + 1u8;
          if (i > 2u8)
            continue;
          endif
          print('.');
        endwhile
        j = j + 1u8;
        print('_');
      endwhile
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0.1.234_0.1.234_0.1.234_ end");
}

TEST_F(CoreEtchTests,
       in_for_loop_inside_a_while_loop_continue_skips_to_the_next_iteration_of_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      var j = 0u8;
      while (j < 3u8)
        for (i in 0u8:6u8)
          print(i);
          if (i > 2u8)
            continue;
          endif
          print('.');
        endfor
        j = j + 1u8;
        print('_');
      endwhile
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0.1.2.345_0.1.2.345_0.1.2.345_ end");
}

TEST_F(CoreEtchTests,
       in_while_loop_inside_a_for_loop_continue_skips_to_the_next_iteration_of_the_inner_loop)
{
  static char const *TEXT = R"(
    function main()
      for (j in 0u8:4u8)
        var i = 0u8;
        while (i < 5u8)
          print(i);
          i = i + 1u8;
          if (i > 2u8)
            continue;
          endif
          print('.');
        endwhile
        print('_');
      endfor
      print(' end');
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0.1.234_0.1.234_0.1.234_0.1.234_ end");
}

TEST_F(CoreEtchTests,
       boolean_or_operator_does_not_execute_second_operand_if_first_operand_evaluates_to_true)
{
  static char const *TEXT = R"(
    function returns_true() : Bool
      print('one');
      return true;
    endfunction

    function returns_false() : Bool
      print('not printed');
      return false;
    endfunction

    function main()
      if (returns_true() || returns_false())
        print('_two');
      endif
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "one_two");
}

TEST_F(CoreEtchTests,
       boolean_and_operator_does_not_execute_second_operand_if_first_operand_evaluates_to_false)
{
  static char const *TEXT = R"(
    function returns_true() : Bool
      print('not printed');
      return true;
    endfunction

    function returns_false() : Bool
      print('one');
      return false;
    endfunction

    function main()
      if (returns_false() && returns_true())
        print('not printed');
      endif
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "one");
}

TEST_F(CoreEtchTests,
       boolean_or_operator_executes_both_operands_if_first_operand_evaluates_to_false)
{
  static char const *TEXT = R"(
    function returns_true() : Bool
      print('two');
      return true;
    endfunction

    function returns_false() : Bool
      print('one_');
      return false;
    endfunction

    function main()
      if (returns_false() || returns_true())
        print('_three');
      endif
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "one_two_three");
}

TEST_F(CoreEtchTests,
       boolean_and_operator_executes_both_operands_if_first_operand_evaluates_to_true)
{
  static char const *TEXT = R"(
    function returns_true() : Bool
      print('one_');
      return true;
    endfunction

    function returns_false() : Bool
      print('two');
      return false;
    endfunction

    function main()
      if (returns_true() && returns_false())
        print('not printed');
      endif
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "one_two");
}

TEST_F(CoreEtchTests, range_for_loop_excludes_end_of_range)
{
  static char const *TEXT = R"(
    function main()
      for (i in 0:3)
        print(i);
      endfor

      print('_');

      for (i in 1:6:2)
        print(i);
      endfor
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "012_135");
}

TEST_F(CoreEtchTests, range_with_equal_bounds_is_empty)
{
  static char const *TEXT = R"(
    function main()
      for (i in 1:1)
        print("Not printed " + toString(i));
      endfor
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "");
}

class CoreEtchValidNumericLiteralsTests : public TestWithParam<std::string>
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

std::vector<std::string> const numeric_literal_value_templates = {
    "0{}i32",      "12{}u32",    "0.0{}",      "0.00{}",       "0.000{}",   "12.0{}",
    "0.01{}",      "0.00123{}",  "0.0{}f",     "0.00{}f",      "0.000{}f",  "12.0{}f",
    "0.01{}f",     "0.00123{}f", "0{}fp32",    "12{}fp32",     "0.0{}fp32", "0.00{}fp32",
    "0.000{}fp32", "12.0{}fp32", "0.01{}fp32", "0.00123{}fp32"};

TEST_P(CoreEtchValidNumericLiteralsTests, valid_numeric_literals)
{
  std::string const TEXT = std::string(R"(
    function main()
      var x = )") + GetParam() +
                           R"(;
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

std::vector<std::string> render(std::string const &x)
{
  auto values = numeric_literal_value_templates;
  for (auto &value : values)
  {
    fetch::string::Replace(value, "{}", x);
  }
  return values;
}

INSTANTIATE_TEST_CASE_P(ValidNumericLiterals, CoreEtchValidNumericLiteralsTests,
                        ValuesIn(render("")), );

INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationUppercaseZeroImplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("E0")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationUppercaseZeroExplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("E+0")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationUppercaseZeroExplicitMinus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("E-0")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationLowercaseZeroImplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("e0")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationLowercaseZeroExplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("e+0")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationLowercaseZeroExplicitMinus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("e-0")), );

INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationUppercaseNonZeroImplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("E12")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationUppercaseNonZeroExplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("E+12")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationUppercaseNonZeroExplicitMinus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("E-12")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationLowercaseNonZeroImplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("e12")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationLowercaseNonZeroExplicitPlus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("e+12")), );
INSTANTIATE_TEST_CASE_P(ValidNumericLiteralsEngineeringNotationLowercaseNonZeroExplicitMinus,
                        CoreEtchValidNumericLiteralsTests, ValuesIn(render("e-12")), );

class CoreEtchInvalidNumericLiteralsTests : public TestWithParam<std::string>
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_P(CoreEtchInvalidNumericLiteralsTests, invalid_numeric_literals)
{
  std::string const TEXT = std::string(R"(
    function main()
      var x = )") + GetParam() +
                           R"(;
    endfunction
  )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

std::vector<std::string> const invalid_numeric_literal_values = {
    "i32",     "0u31",     "00",      "00u32",    "01",        "01u32",  "001",     "001u32",
    "00.0",    "00.01",    ".0",      ".01",      ".1",        "0.",     "00.",     "12.",
    "00.0u32", "00.01u32", ".0u32",   ".01u32",   ".1u32",     "0.u32",  "00.u32",  "12.u32",
    "00fp64",  "01fp64",   "001fp64", "00.0fp64", "00.01fp64", ".0fp64", ".01fp64", ".1fp64",
    "0.fp64",  "00.fp64",  "12.fp64", "0f",       "12f",       "00f",    "01f",     "001f",
    "00.0f",   "00.01f",   ".0f",     ".01f",     ".1f",       "0.f",    "00.f",    "12.f",
    "1e",      "1e+",      "1e-",     "1e1.1",    "1e+1.1",    "1e-1.1", "1E",      "1E+",
    "1E-",     "1E1.1",    "1E+1.1",  "1E-1.1"};

INSTANTIATE_TEST_CASE_P(InvalidNumericLiterals, CoreEtchInvalidNumericLiteralsTests,
                        ValuesIn(invalid_numeric_literal_values), );

}  // namespace
