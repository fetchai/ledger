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

#include <sstream>

namespace {

using namespace fetch::vm;

class ContractToContractTests : public ::testing::Test
{
public:
  std::stringstream stdout;
  VmTestToolkit     toolkit{&stdout};
};

TEST_F(ContractToContractTests, simple_invocation_handler)  //???
{
  static char const *TEXT1 = R"(
    contract c2_interface
      function eleven() : UInt8;
    endcontract

    function main()
      contract c2 = c2_interface("contract_name_here");

      assert(c2.eleven() == 11u8);
    endfunction
  )";

  static char const *TEXT2 = R"(
    function eleven() : UInt8
      return 11u8;
    endfunction
  )";

  EXPECT_TRUE(toolkit.Compile(TEXT1));

  toolkit.vm().SetContractInvocationHandler(
      [](VM *vm, std::string const & /* identity */, Executable::Contract const & /* contract */,
         Executable::Function const &function, fetch::vm::VariantArray /* parameters */,
         std::string &error, Variant &output) -> bool {
        auto     module = fetch::vm_modules::VMFactory::GetModule(VMFactory::USE_SMART_CONTRACTS);
        Compiler compiler{module.get()};
        VM       vm2{module.get()};

        std::vector<std::string> errors{};

        IR ir;

        fetch::vm::SourceFiles files = {{"default.etch", TEXT2}};
        if (!compiler.Compile(files, "default_ir", ir, errors))
        {
          return false;
        }

        // build the executable
        Executable exe;
        vm2.SetIOObserver(vm->GetIOObserver());
        vm2.AttachOutputDevice(fetch::vm::VM::STDOUT, vm->GetOutputDevice(fetch::vm::VM::STDOUT));

        if (!vm2.GenerateExecutable(ir, "default_exe", exe, errors))
        {
          return false;
        }

        if (!vm2.Execute(exe, function.name, error, output))
        {
          return false;
        }

        return true;
      });

  EXPECT_TRUE(toolkit.Run());
}

}  // namespace
