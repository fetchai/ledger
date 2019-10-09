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

#include "gtest/gtest.h"

#include "dmlf/vm_wrapper_etch.hpp"

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

namespace {

using namespace fetch::vm;

using Params = fetch::dmlf::VmWrapperInterface::Params;
using Status = fetch::dmlf::VmWrapperInterface::Status;

}  // namespace

TEST(VmDmlfTests, etch_simpleHelloWorld)
{
  // load the contents of the script file
  auto const source = R"(
function main()

  printLn("Hello world!!");

endfunction)";

  fetch::dmlf::VmWrapperEtch vm;
  EXPECT_EQ(vm.status(), Status::UNCONFIGURED);

  vm.Setup(fetch::dmlf::VmWrapperInterface::Flags());
  EXPECT_EQ(vm.status(), Status::WAITING);

  std::vector<std::string> result;
  auto outputTest = [&result](std::string line) { result.emplace_back(std::move(line)); };
  vm.SetStdout(outputTest);

  auto errors = vm.Load(source);
  EXPECT_EQ(vm.status(), Status::COMPILED);
  EXPECT_TRUE(errors.empty());

  vm.Execute("main", Params());
  EXPECT_EQ(vm.status(), Status::COMPLETED);

  EXPECT_EQ(result.size(), 1);
  EXPECT_EQ(result[0], "Hello world!!");
}

TEST(VmDmlfTests, etch_doubleHelloWorld)
{
  // load the contents of the script file
  auto const source = R"(
function main()

  printLn("Hello world!!");

endfunction)";

  fetch::dmlf::VmWrapperEtch vm;
  EXPECT_EQ(vm.status(), Status::UNCONFIGURED);

  vm.Setup(fetch::dmlf::VmWrapperInterface::Flags());
  EXPECT_EQ(vm.status(), Status::WAITING);

  std::vector<std::string> result;
  auto outputTest = [&result](std::string line) { result.emplace_back(std::move(line)); };
  vm.SetStdout(outputTest);

  auto errors = vm.Load(source);
  EXPECT_EQ(vm.status(), Status::COMPILED);
  EXPECT_TRUE(errors.empty());

  vm.Execute("main", Params());
  EXPECT_EQ(vm.status(), Status::COMPLETED);

  auto const source2 = R"(
function main()

  printLn("Hello world again!!!");

endfunction)";

  errors = vm.Load(source2);
  EXPECT_EQ(vm.status(), Status::COMPILED);
  EXPECT_TRUE(errors.empty());

  vm.Execute("main", Params());
  EXPECT_EQ(vm.status(), Status::COMPLETED);

  EXPECT_EQ(result.size(), 2);
  EXPECT_EQ(result[0], "Hello world!!");
  EXPECT_EQ(result[1], "Hello world again!!!");
}

}  // namespace
