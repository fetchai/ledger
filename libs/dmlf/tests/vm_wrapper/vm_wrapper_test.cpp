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

#include "gtest/gtest.h"
#include "gtest/gtest.h"

#include "dmlf/vm_wrapper_systemcommand.hpp"
#include "dmlf/vm_wrapper_python3.hpp"

#include "variant/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class VMWrapperTests : public ::testing::Test
{
public:
  void SetUp() override
  {
  }

};

TEST_F(VMWrapperTests, sys_run)
{
  fetch::dmlf::VmWrapperSystemcommand vm;

  std::vector<std::string> outputs;
  std::vector<std::string> errors;

  vm.SetStdout([&outputs](const std::string &s){ outputs.push_back(s); });
  vm.SetStderr([&errors](const std::string &s){ errors.push_back(s); });

  vm.Setup(fetch::dmlf::VmWrapperInterface::Flags());
  EXPECT_EQ(vm.status(), fetch::dmlf::VmWrapperInterface::WAITING);
  vm.Load("/bin/ls");
  EXPECT_EQ(vm.status(), fetch::dmlf::VmWrapperInterface::COMPILED);
  vm.Execute("", fetch::dmlf::VmWrapperSystemcommand::Params());

  EXPECT_EQ(vm.status(), fetch::dmlf::VmWrapperInterface::COMPLETED);
  EXPECT_EQ(errors.size(), 0);
  EXPECT_GT(outputs.size(), 1);
}

TEST_F(VMWrapperTests, py_run)
{
  fetch::dmlf::VmWrapperPython3 vm;

  std::vector<std::string> outputs;
  std::vector<std::string> errors;

  vm.SetStdout([&outputs](const std::string &s){ outputs.push_back(s); });
  vm.SetStderr([&errors](const std::string &s){ errors.push_back(s); });

  vm.Setup(fetch::dmlf::VmWrapperInterface::Flags());
  EXPECT_EQ(vm.status(), fetch::dmlf::VmWrapperInterface::WAITING);

  vm.Load("print('hello')");
  EXPECT_EQ(vm.status(), fetch::dmlf::VmWrapperInterface::COMPILED);

}



}  // namespace
