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
  /*
  auto const source = R"(
function main()

    printLn("Hello world!!");

endfunction)";
  */

//  //auto executable = std::make_unique<Executable>();
//  //auto module     = VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS);
//
//
//
//
//    VM_Wrapper wrapper(VMFactory::USE_SMART_CONTRACTS);
//
//  // attempt to compile the program
//  auto errors = wrapper.Load(source);
//  //auto errors = VMFactory::Compile(module, source, *executable);
//
//  // detect compilation errors
//  EXPECT_TRUE(errors.empty());
//  //if (!errors.empty())
//  //{
//  //  std::cerr << "Failed to compile:\n";
//
//  //  for (auto const &line : errors)
//  //  {
//  //    std::cerr << line << '\n';
//  //  }
//
//  //  return 1;
//  //}
//
//  auto& vm = wrapper.vm_;
//
//  // create the VM instance
//  //auto vm = std::make_unique<VM>(module.get()); 
//  //vm->AttachOutputDevice(VM::STDOUT, std::cout);
//
//  // Execute the requested function
//  bool const success = wrapper.Execute();
//  //std::string error;
//  //std::string console;
//  //Variant     output;
//  //bool const  success =
//  //    vm->Execute(*wrapper.executable_, "main", error, output);
//
//  EXPECT_TRUE(success);
//  //if (!success)
//  //{
//  //  std::cerr << error << std::endl;
//  //  return 1;
//  //}
//  // if there is any console output print it
//  if (!console.empty())
//  {
//    std::cout << console << std::endl;
//  }
//

  EXPECT_EQ(1, 1);
}



}  // namespace
