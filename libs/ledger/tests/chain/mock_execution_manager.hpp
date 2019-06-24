#pragma once
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

#include "ledger/execution_manager_interface.hpp"

#include "fake_execution_manager.hpp"

#include "gmock/gmock.h"

class MockExecutionManager : public fetch::ledger::ExecutionManagerInterface
{
public:
  using Block  = fetch::ledger::Block;
  using Digest = fetch::ledger::Digest;

  explicit MockExecutionManager(FakeStorageUnit &storage)
    : fake{storage}
  {
    using ::testing::_;
    using ::testing::Invoke;

    ON_CALL(*this, Execute(_)).WillByDefault(Invoke(&fake, &FakeExecutionManager::Execute));
    ON_CALL(*this, SetLastProcessedBlock(_))
        .WillByDefault(Invoke(&fake, &FakeExecutionManager::SetLastProcessedBlock));
    ON_CALL(*this, LastProcessedBlock())
        .WillByDefault(Invoke(&fake, &FakeExecutionManager::LastProcessedBlock));
    ON_CALL(*this, GetState()).WillByDefault(Invoke(&fake, &FakeExecutionManager::GetState));
    ON_CALL(*this, Abort()).WillByDefault(Invoke(&fake, &FakeExecutionManager::Abort));
  }

  MOCK_METHOD1(Execute, ScheduleStatus(Block::Body const &));
  MOCK_METHOD1(SetLastProcessedBlock, void(Digest));
  MOCK_METHOD0(LastProcessedBlock, Digest());
  MOCK_METHOD0(GetState, State());
  MOCK_METHOD0(Abort, bool());

  FakeExecutionManager fake;
};
