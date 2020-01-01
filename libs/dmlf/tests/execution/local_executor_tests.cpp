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

#include "gtest/gtest.h"

#include "dmlf/execution/basic_vm_engine.hpp"
#include "dmlf/execution/local_executor.hpp"

#include "variant/variant.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using fetch::dmlf::BasicVmEngine;
using fetch::dmlf::ExecutionEngineInterface;
using fetch::dmlf::ExecutionResult;
using fetch::dmlf::LocalExecutor;
using fetch::vm::SourceFile;

using PromiseOfResult = fetch::dmlf::ExecutionResult::PromiseOfResult;
using Params          = fetch::dmlf::LocalExecutor::Params;
using VariantType     = fetch::variant::Variant::Type;

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
  std::shared_ptr<LocalExecutor>            executor;
  std::shared_ptr<ExecutionEngineInterface> engine;
  SourceFile                                source_file;
  ExecutionResult                           result;
  PromiseOfResult                           result_promise;
  const std::string                         host;

  void SetUp() override
  {
    engine   = std::make_shared<BasicVmEngine>();
    executor = std::make_shared<LocalExecutor>(engine);
  }

  PromiseOfResult CreateExecutable(std::string const &exec_name, SourceFile const &source_file_loc)
  {
    return executor->CreateExecutable(host, exec_name, {source_file_loc});
  }

  PromiseOfResult DeleteExecutable(std::string const &exec_name)
  {
    return executor->DeleteExecutable(host, exec_name);
  }

  PromiseOfResult CreateState(std::string const &state_name)
  {
    return executor->CreateState(host, state_name);
  }

  PromiseOfResult DeleteState(std::string const &state_name)
  {
    return executor->DeleteState(host, state_name);
  }

  PromiseOfResult RunExecutable(std::string const &exec_name, std::string const &state_name)
  {
    return executor->Run(host, exec_name, state_name, "main", Params{});
  }

  bool IsSuccessfullyFulfilled(PromiseOfResult &promise)
  {
    promise.GetResult(result);
    return result.succeeded() && result.console().empty() &&
           result.output().type() == VariantType::STRING;
  }

  bool IsSuccessfullyFulfilledWithOutput(PromiseOfResult &promise, std::string const &output)
  {
    promise.GetResult(result);
    return result.succeeded() && result.console() == output &&
           result.output().type() == VariantType::STRING;
  }
};

TEST_F(LocalExecutorTests, create_state)
{
  result_promise = CreateState("State");
  ASSERT_TRUE(IsSuccessfullyFulfilled(result_promise));

  result_promise = CreateState("State");
  ASSERT_FALSE(IsSuccessfullyFulfilled(result_promise));
}

TEST_F(LocalExecutorTests, delete_state)
{
  result_promise = DeleteState("State");
  ASSERT_FALSE(IsSuccessfullyFulfilled(result_promise));

  CreateState("State").Wait();
  result_promise = DeleteState("State");
  ASSERT_TRUE(IsSuccessfullyFulfilled(result_promise));
}

TEST_F(LocalExecutorTests, create_executable)
{
  // TODO(tfr): It is unclear why running the same code twice would yeild two different results
  // Some explanation would be good.
  result_promise = CreateExecutable("HelloWorld", SourceFile{"hello_world.etch", hello_world_etch});
  ASSERT_TRUE(IsSuccessfullyFulfilled(result_promise));

  result_promise = CreateExecutable("HelloWorld", SourceFile{"hello_world.etch", hello_world_etch});
  ASSERT_FALSE(IsSuccessfullyFulfilled(result_promise));
}

TEST_F(LocalExecutorTests, delete_executable)
{
  result_promise = DeleteExecutable("HelloWorld");
  ASSERT_FALSE(IsSuccessfullyFulfilled(result_promise));

  CreateExecutable("HelloWorld", SourceFile{"hello_world.etch", hello_world_etch}).Wait();
  result_promise = DeleteExecutable("HelloWorld");
  ASSERT_TRUE(IsSuccessfullyFulfilled(result_promise));
}

TEST_F(LocalExecutorTests, void_function_void)
{
  CreateExecutable("HelloWorld", SourceFile{"hello_world.etch", hello_world_etch}).Wait();
  CreateState("State").Wait();

  result_promise = RunExecutable("HelloWorld", "State");
  ASSERT_TRUE(IsSuccessfullyFulfilledWithOutput(result_promise, hello_world_etch_output));
}
}  // namespace
