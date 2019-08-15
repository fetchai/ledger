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
  int argc = 6;

  char **argv   = new char *[static_cast<uint32_t>(argc)];
  char   arg0[] = "executable";
  char   arg1[] = "scriptname";
  char   arg2[] = "etch_arg1";
  char   arg3[] = "--";
  char   arg4[] = "prog_arg1";
  char   arg5[] = "prog_arg2";
  argv[0]       = arg0;
  argv[1]       = arg1;
  argv[2]       = arg2;
  argv[3]       = arg3;
  argv[4]       = arg4;
  argv[5]       = arg5;

  // parse the command line parameters
  System::Parse(argc, argv);

  fetch::commandline::ParamsParser const &pp = System::GetParamParser();
  EXPECT_TRUE(pp.arg_size() == 3);
  EXPECT_TRUE(pp.GetArg(0) == arg0);
  EXPECT_TRUE(pp.GetArg(1) == arg1);
  EXPECT_TRUE(pp.GetArg(2) == arg2);

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

  ASSERT_EQ(stdout.str(), "3\n" + std::string(arg0) + "\n" + arg4 + "\n" + arg5 + "\n");

  delete[] argv;
}

}  // namespace
