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

#include "core/byte_array/encoders.hpp"

#include "fake_execution_manager.hpp"
#include "fake_storage_unit.hpp"

using fetch::byte_array::ToBase64;

FakeExecutionManager::FakeExecutionManager(FakeStorageUnit &storage)
  : storage_{storage}
{
  FETCH_UNUSED(storage_);
}

FakeExecutionManager::ScheduleStatus FakeExecutionManager::Execute(Block::Body const &block)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Execute called. State: ", ToBase64(block.merkle_hash));

  // if we have
  if (current_hash_.size())
  {
    return ScheduleStatus::ALREADY_RUNNING;
  }

  // update the current hash
  current_hash_        = block.hash.Copy();
  current_merkle_root_ = block.merkle_hash.Copy();
  current_polls_       = 2;

  // For the purposes of testing, after execution, we will set the state here to be in line with the
  // block state
  storage_.SetCurrentHash(current_merkle_root_);

  return ScheduleStatus::SCHEDULED;
}

FakeExecutionManager::BlockHash FakeExecutionManager::LastProcessedBlock()
{
  FETCH_LOG_INFO(LOGGING_NAME, "LastProcessedBlock called");

  return last_processed_;
}

FakeExecutionManager::State FakeExecutionManager::GetState()
{
  FETCH_LOG_INFO(LOGGING_NAME, "GetState called");

  // decrement the
  bool execution_complete{false};
  if (current_polls_)
  {
    --current_polls_;

    if (current_polls_ == 0)
    {
      execution_complete = true;
    }
  }

  // simulate a poll interval delay before execution is complete
  if (execution_complete)
  {
    last_processed_ = current_hash_.Copy();

    current_hash_ = BlockHash{};
  }

  // evaluate the state
  return (current_polls_ ? State::ACTIVE : State::IDLE);
}

bool FakeExecutionManager::Abort()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Abort called");

  return false;
}

void FakeExecutionManager::SetLastProcessedBlock(BlockHash hash)
{
  last_processed_ = hash;
}
