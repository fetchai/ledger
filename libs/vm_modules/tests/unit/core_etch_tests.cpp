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

// TODO(WK): extract test helpers library and move this test suite to the libs/vm
class CoreEtchTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(CoreEtchTests, in_for_loop_break_exits_the_loop)
{
  static char const *TEXT = R"(
    function main()
      for (i in 0u8:5u8)
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
      for (j in 0u8:3u8)
        for (i in 0u8:5u8)
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
      for (j in 0u8:3u8)
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
      for (i in 0u8:5u8)
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
      for (j in 0u8:2u8)
        for (i in 0u8:5u8)
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
        for (i in 0u8:5u8)
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
      for (j in 0u8:3u8)
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

}  // namespace
