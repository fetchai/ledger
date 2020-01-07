//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include <sstream>

namespace {

using namespace testing;

class EtchCommentTests : public Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(EtchCommentTests, single_line_comment_at_file_scope)
{
  static char const *TEXT = R"(
    // ignored comment
    // ignored comment

    function main()
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, single_line_comment_at_function_scope)
{
  static char const *TEXT = R"(
    function main()
      // ignored comment
      // ignored comment
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, single_line_comment_at_beginning_of_file)
{
  static char const *TEXT = R"(// foo
    function main()
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, single_line_comment_at_end_of_file)
{
  static char const *TEXT = R"(
    function main()
    endfunction
  // foo)";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, single_line_empty_comment_at_beginning_of_file)
{
  static char const *TEXT = R"(//
    function main()
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, single_line_empty_comment_at_end_of_file)
{
  static char const *TEXT = R"(
    function main()
    endfunction
  //)";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, multiline_comment_at_file_scope)
{
  static char const *TEXT = R"(
    /* ignored comment
       ignored comment */

    function main()
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, multiline_comment_at_function_scope)
{
  static char const *TEXT = R"(
    function main()
      /* ignored comment
         ignored comment */
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, multiline_comment_at_beginning_of_file)
{
  static char const *TEXT = R"(/* foo */
    function main()
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, multiline_comment_at_end_of_file)
{
  static char const *TEXT = R"(
    function main()
    endfunction
  /* foo */)";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());
}

TEST_F(EtchCommentTests, unterminated_multiline_comment_fails_compilation)
{
  static char const *TEXT = R"(
    function main()
    endfunction
  /* foo )";

  ASSERT_FALSE(toolkit.Compile(TEXT));
}

}  // namespace
