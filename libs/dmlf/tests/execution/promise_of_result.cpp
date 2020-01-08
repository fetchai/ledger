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

#include "dmlf/execution/execution_result.hpp"

#include "gtest/gtest.h"

#include <stdexcept>

namespace {

using fetch::dmlf::ExecutionResult;
using fetch::dmlf::ExecutionErrorMessage;

using PromiseOfResult = fetch::dmlf::ExecutionResult::PromiseOfResult;
using State           = fetch::service::details::PromiseImplementation::State;

class PromiseOfResultTests : public ::testing::Test
{
public:
  PromiseOfResult       promise;
  ExecutionResult       result;
  ExecutionResult       fulfillment;
  ExecutionErrorMessage status;

  bool isInitializedAndNotFulfilled()
  {
    return !promise.empty() && !promise && promise.GetState() == State::WAITING;
  }

  bool isFulfilled()
  {
    bool isit = !promise.empty() && promise && promise.GetState() == State::SUCCESS;
    if (isit)
    {
      promise.GetResult(fulfillment);
    }
    return isit;
  }
};

TEST_F(PromiseOfResultTests, make_promise)
{
  promise = ExecutionResult::MakePromise();
  ASSERT_TRUE(isInitializedAndNotFulfilled());
}

TEST_F(PromiseOfResultTests, make_fulfilled_with_result)
{
  promise = ExecutionResult::MakeFulfilledPromise(result);
  ASSERT_TRUE(isFulfilled());
}

TEST_F(PromiseOfResultTests, make_fulfilled_with_status)
{
  promise = ExecutionResult::MakeFulfilledPromise(status);
  ASSERT_TRUE(isFulfilled());
}

TEST_F(PromiseOfResultTests, make_fulfilled_success)
{
  promise = ExecutionResult::MakeFulfilledPromiseSuccess();
  ASSERT_TRUE(isFulfilled());
}

TEST_F(PromiseOfResultTests, make_fulfilled_error)
{
  promise = ExecutionResult::MakeFulfilledPromiseError(ExecutionErrorMessage::Code::BAD_TARGET,
                                                       std::string{});
  ASSERT_TRUE(isFulfilled());
}

TEST_F(PromiseOfResultTests, fulfill)
{
  promise = ExecutionResult::MakePromise();
  ASSERT_TRUE(isInitializedAndNotFulfilled());

  ExecutionResult::FulfillPromise(promise, result);
  ASSERT_TRUE(isFulfilled());
}

}  // namespace
