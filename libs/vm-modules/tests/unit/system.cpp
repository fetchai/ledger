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
  std::vector<std::string>  args = {"executable", "scriptname"};
  std::vector<char const *> argv;
  argv.reserve(args.size());
  for (auto const &s : args)
  {
    argv.push_back(s.c_str());
  }

  // parse the command line parameters
  System::Parse(static_cast<int>(argv.size()), argv.data());

  System::Bind(toolkit.module());

  static char const *TEXT = R"(
    function main()
      printLn(System.Argc());
        for(i in 0:System.Argc())
          printLn(System.Argv(i));
        endfor
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "1\n" + args.at(0) + "\n");
}

TEST_F(SystemTests, some_args)
{
  std::vector<std::string>  args = {"executable", "scriptname", "etch_arg1",
                                   "--",         "prog_arg1",  "prog_arg2"};
  std::vector<char const *> argv;
  argv.reserve(args.size());
  for (auto const &s : args)
  {
    argv.push_back(s.c_str());
  }

  // parse the command line parameters
  System::Parse(static_cast<int>(argv.size()), argv.data());

  fetch::commandline::ParamsParser const &pp = System::GetParamsParser();
  EXPECT_EQ(pp.arg_size(), 3);
  EXPECT_EQ(pp.GetArg(0), args.at(0));
  EXPECT_EQ(pp.GetArg(1), args.at(1));
  EXPECT_EQ(pp.GetArg(2), args.at(2));

  System::Bind(toolkit.module());

  static char const *TEXT = R"(
    function main()
      printLn(System.Argc());
      for(i in 0:System.Argc())
        printLn(System.Argv(i));
      endfor
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "3\n" + args.at(0) + "\n" + args.at(4) + "\n" + args.at(5) + "\n");
}

TEST_F(SystemTests, only_etch_args)
{
  std::vector<std::string>  args = {"executable", "scriptname", "etch_arg1", "--"};
  std::vector<char const *> argv;
  argv.reserve(args.size());
  for (auto const &s : args)
  {
    argv.push_back(s.c_str());
  }

  // parse the command line parameters
  System::Parse(static_cast<int>(argv.size()), argv.data());

  fetch::commandline::ParamsParser const &pp = System::GetParamsParser();
  EXPECT_EQ(pp.arg_size(), 3);
  EXPECT_EQ(pp.GetArg(0), args.at(0));
  EXPECT_EQ(pp.GetArg(1), args.at(1));
  EXPECT_EQ(pp.GetArg(2), args.at(2));

  System::Bind(toolkit.module());

  static char const *TEXT = R"(
    function main()
      printLn(System.Argc());
      for(i in 0:System.Argc())
        printLn(System.Argv(i));
      endfor
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "1\n" + args.at(0) + "\n");
}

TEST_F(SystemTests, only_program_args)
{
  std::vector<std::string>  args = {"executable", "scriptname", "--", "prog_arg1", "prog_arg2"};
  std::vector<char const *> argv;
  argv.reserve(args.size());
  for (auto const &s : args)
  {
    argv.push_back(s.c_str());
  }

  // parse the command line parameters
  System::Parse(static_cast<int>(argv.size()), argv.data());

  fetch::commandline::ParamsParser const &pp = System::GetParamsParser();
  EXPECT_EQ(pp.arg_size(), 2);
  EXPECT_EQ(pp.GetArg(0), args.at(0));
  EXPECT_EQ(pp.GetArg(1), args.at(1));

  System::Bind(toolkit.module());

  static char const *TEXT = R"(
    function main()
      printLn(System.Argc());
      for(i in 0:System.Argc())
        printLn(System.Argv(i));
      endfor
    endfunction
  )";

  ASSERT_TRUE(toolkit.Compile(TEXT));
  ASSERT_TRUE(toolkit.Run());

  ASSERT_EQ(stdout.str(), "3\n" + args.at(0) + "\n" + args.at(3) + "\n" + args.at(4) + "\n");
}

}  // namespace
