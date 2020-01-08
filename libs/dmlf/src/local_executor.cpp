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

#include "dmlf/execution/local_executor.hpp"

#include <map>
#include <memory>
#include <sstream>

namespace fetch {
namespace dmlf {

LocalExecutor::LocalExecutor(ExecutionEnginePtr exec_engine)
  : exec_engine_{std::move(exec_engine)}
{}

LocalExecutor::PromiseOfResult LocalExecutor::CreateExecutable(Target const & /*host*/,
                                                               Name const &       execName,
                                                               SourceFiles const &sources)
{
  ExecutionResult result = exec_engine_->CreateExecutable(execName, sources);
  return ExecutionResult::MakeFulfilledPromise(result);
}

LocalExecutor::PromiseOfResult LocalExecutor::DeleteExecutable(Target const & /*host*/,
                                                               Name const &execName)
{
  ExecutionResult result = exec_engine_->DeleteExecutable(execName);
  return ExecutionResult::MakeFulfilledPromise(result);
}

LocalExecutor::PromiseOfResult LocalExecutor::CreateState(Target const & /*host*/,
                                                          Name const &stateName)
{
  ExecutionResult result = exec_engine_->CreateState(stateName);
  return ExecutionResult::MakeFulfilledPromise(result);
}

LocalExecutor::PromiseOfResult LocalExecutor::CopyState(Target const & /*host*/,
                                                        Name const &srcName, Name const &newName)
{
  ExecutionResult result = exec_engine_->CopyState(srcName, newName);
  return ExecutionResult::MakeFulfilledPromise(result);
}

LocalExecutor::PromiseOfResult LocalExecutor::DeleteState(Target const & /*host*/,
                                                          Name const &stateName)
{
  ExecutionResult result = exec_engine_->DeleteState(stateName);
  return ExecutionResult::MakeFulfilledPromise(result);
}

LocalExecutor::PromiseOfResult LocalExecutor::Run(Target const & /*host*/, Name const &execName,
                                                  Name const &       stateName,
                                                  std::string const &entrypoint,
                                                  Params const &     params)
{
  ExecutionResult result = exec_engine_->Run(execName, stateName, entrypoint, params);
  return ExecutionResult::MakeFulfilledPromise(result);
}

}  // namespace dmlf
}  // namespace fetch
