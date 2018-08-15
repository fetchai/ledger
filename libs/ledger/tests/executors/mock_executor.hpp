#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "fake_executor.hpp"
#include "ledger/executor_interface.hpp"

#include <gmock/gmock.h>

class MockExecutor : public fetch::ledger::ExecutorInterface
{
public:
  MockExecutor()
  {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, Execute(_, _, _)).WillByDefault(Invoke(&fake_, &FakeExecutor::Execute));
  }

  MOCK_METHOD3(Execute, Status(tx_digest_type const &, std::size_t, lane_set_type const &));

private:
  FakeExecutor fake_;
};
