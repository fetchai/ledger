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

#include "vm_modules/core/system.hpp"
#include "vm_test_toolkit.hpp"

#include "gmock/gmock.h"

#include <string>

namespace {

using System = fetch::vm_modules::System;

class SystemTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(SystemTests, no_args)
{
  System::Bind(toolkit.module());

  static char const *TEXT = R"(
    function main()
      print(System.Argc());
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "0");
}

TEST_F(SystemTests, some_args)
{
  std::vector<std::string>  foo = {"executable", "scriptname", "etch_arg1",
                                  "--",         "prog_arg1",  "prog_arg2"};
  std::vector<const char *> argv;
  for (auto const &s : foo)
  {
    argv.push_back(s.c_str());
  }

  // parse the command line parameters
  System::Parse(static_cast<int>(argv.size()), argv.data());

  fetch::commandline::ParamsParser const &pp = System::GetParamParser();
  EXPECT_TRUE(pp.arg_size() == 3);
  EXPECT_TRUE(pp.GetArg(0) == foo.at(0));
  EXPECT_TRUE(pp.GetArg(1) == foo.at(1));
  EXPECT_TRUE(pp.GetArg(2) == foo.at(2));

  System::Bind(toolkit.module());

  static char const *TEXT = R"(
    function main()
      printLn(System.Argc());
      printLn(System.Argv(0));
      printLn(System.Argv(1));
      printLn(System.Argv(2));
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "3\n" + foo.at(0) + "\n" + foo.at(4) + "\n" + foo.at(5) + "\n");
}

}  // namespace
