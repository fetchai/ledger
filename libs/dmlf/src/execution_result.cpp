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

#include "dmlf/execution/execution_result.hpp"

namespace fetch {
namespace dmlf {

ExecutionResult::PromiseOfResult ExecutionResult::MakePromise()
{
  fetch::network::PromiseOf<ExecutionResult> promise{service::MakePromise()};
  return promise;
}

void ExecutionResult::FulfillPromise(PromiseOfResult promise, ExecutionResult fulfiller)
{
  serializers::MsgPackSerializer serializer;
  serializer << fulfiller;
  promise.GetInnerPromise()->Fulfill(serializer.data());
}

ExecutionResult::PromiseOfResult ExecutionResult::MakeFulfilledPromise(Error error)
{
  auto promise = ExecutionResult::MakePromise();
  auto result  = ExecutionResult{Variant{}, error, std::string{}};

  ExecutionResult::FulfillPromise(promise, result);

  return promise;
}

ExecutionResult::PromiseOfResult ExecutionResult::MakeFulfilledPromise(ExecutionResult fulfiller)
{
  auto promise = ExecutionResult::MakePromise();
  ExecutionResult::FulfillPromise(promise, fulfiller);
  return promise;
}

ExecutionResult::PromiseOfResult ExecutionResult::MakeFulfilledPromiseSuccess()
{
  auto success = Error{ErrorStage::ENGINE, ErrorCode::SUCCESS, std::string{}};
  return ExecutionResult::MakeFulfilledPromise(success);
}

ExecutionResult::PromiseOfResult ExecutionResult::MakeFulfilledPromiseError(
    ErrorCode error_code, std::string error_message)
{
  auto error = Error{ErrorStage::ENGINE, error_code, error_message};
  return ExecutionResult::MakeFulfilledPromise(error);
}

}  // namespace dmlf
}  // namespace fetch
