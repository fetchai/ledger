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

#include "dmlf/execution/local_executor.hpp"
#include "dmlf/execution/basic_vm_engine.hpp"

#include "variant/variant.hpp"
#include "vm/variant.hpp"
#include "vm/vm.hpp"
#include "vm_modules/vm_factory.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using fetch::dmlf::BasicVmEngine;
using fetch::dmlf::ExecutionResult;
using fetch::dmlf::ExecutionEngineInterface;
using fetch::dmlf::LocalExecutor;
using fetch::vm::SourceFile;
using fetch::vm::TypeIds::Unknown;

using PromiseOfResult = fetch::dmlf::ExecutionResult::PromiseOfResult;

namespace {

std::string const hello_world_etch = R"(
function main()

  printLn("Hello world!!");

endfunction
)";

std::string const hello_world_etch_output = "Hello world!!\n";

}  // namespace

class LocalExecutorTests : public ::testing::Test
{
public:
  std::shared_ptr<LocalExecutor> executor;
  std::shared_ptr<ExecutionEngineInterface>   engine;
  SourceFile      source_file;
  ExecutionResult result;
  PromiseOfResult result_promise;
  const std::string host;
  
  void SetUp() override
  {
    engine = std::make_shared<BasicVmEngine>();
    executor = std::make_shared<LocalExecutor>(engine);
  }

  void CreateExecutable(std::string exec_name, SourceFile source_file)
  {
    result_promise = executor->CreateExecutable(host, exec_name, {source_file});
  }
  
  void RunExecutable(std::string exec_name)
  {
    result_promise = executor->Run(host, exec_name, std::string{}, "main");
  }
  
  bool OperationSuccessful()
  {
    result = result_promise.Get();
    return result.succeeded() && result.console().empty() && result.output().type_id == Unknown;
  }

  bool OperationSuccessfulWithOutput(std::string output)
  {
    result = result_promise.Get();
    return result.succeeded() && result.console() == output && result.output().type_id == Unknown;
  }

};

TEST_F(LocalExecutorTests, create_executable)
{
  CreateExecutable("HelloWorld", SourceFile{"hello_world.etch", hello_world_etch});
  ASSERT_TRUE(OperationSuccessful());
}

TEST_F(LocalExecutorTests, void_function_void)
{
  CreateExecutable("HelloWorld", SourceFile{"hello_world.etch", hello_world_etch});
  RunExecutable("HelloWorld");
  ASSERT_TRUE(OperationSuccessfulWithOutput(hello_world_etch_output));
}
}  // namespace
