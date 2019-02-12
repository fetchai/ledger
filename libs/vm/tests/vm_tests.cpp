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

#include "vm/vm_factory.hpp"
#include <gtest/gtest.h>

using namespace fetch;
using namespace fetch::vm;

class VMTests : public ::testing::Test
{
protected:
  using Module = std::shared_ptr<fetch::vm::Module>;
  using VM     = std::unique_ptr<fetch::vm::VM>;

  VMTests()
  {}

  void SetUp() override
  {
    // storage_.reset(new underlying_storage_type);
    // executor_ = std::make_unique<underlying_executor_type>(storage_);
  }

  template <typename T>
  void AddBinding(std::string const &name, T function)
  {
    module->CreateFreeFunction(name, function);
  }

  bool Compile(std::string const &source)
  {
    std::vector<std::string> errors = VMFactory::Compile(module, source, script);

    for (auto const &error : errors)
    {
      std::cerr << error << std::endl;
    }

    return errors.size() == 0;
  }

  bool Execute(std::string const &function = "main")
  {
    vm = VMFactory::GetVM(module);
    std::string        error;
    fetch::vm::Variant output;

    // Execute our fn
    if (!vm->Execute(script, function, error, output))
    {
      std::cerr << "Runtime error: " << error << std::endl;
      return false;
    }

    return true;
  }

  Module            module = VMFactory::GetModule();
  VM                vm;
  fetch::vm::Script script;
};

TEST_F(VMTests, CheckCompileAndExecute)
{
  const std::string source =
      " function main() "
      "   Print(\"Hello, world\");"
      " endfunction ";

  bool res = Compile(source);

  EXPECT_EQ(res, true);

  res = Execute();

  EXPECT_EQ(res, true);
}

static int32_t binding_called_count = 0;

static void CustomBinding(fetch::vm::VM * /*vm*/)
{
  binding_called_count++;
}

TEST_F(VMTests, CheckCustomBinding)
{
  const std::string source =
      " function main() "
      "   CustomBinding();"
      " endfunction ";

  EXPECT_EQ(binding_called_count, 0);

  AddBinding("CustomBinding", &CustomBinding);

  bool res = Compile(source);

  EXPECT_EQ(res, true);

  for (std::size_t i = 0; i < 3; ++i)
  {
    res = Execute();
    EXPECT_EQ(res, true);
  }

  EXPECT_EQ(binding_called_count, 3);
}
