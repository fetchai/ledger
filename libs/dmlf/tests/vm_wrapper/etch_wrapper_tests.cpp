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

namespace {

namespace {

using fetch::vm_modules::VMFactory;
using namespace fetch::vm;

}  // namespace

TEST(VmDmlfTests, directHelloWorld)
{
  // load the contents of the script file
  auto const source = R"(
function main()

  printLn("Hello world!!");

endfunction)";

  fetch::dmlf::VmWrapperEtch vm;

  vm.Setup(fetch::dmlf::VmWrapperInterface::Flags());

  auto errors = vm.Load(source);

  EXPECT_TRUE(errors.empty());

  vm.Execute("main", fetch::dmlf::VmWrapperInterface::Params());

}



}  // namespace
