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

#include "ledger/chain/block.hpp"

namespace fetch {
namespace ledger {

class ExecutionManagerInterface
{
public:
  enum class ScheduleStatus
  {
    SCHEDULED = 0,  ///< The block has been scheduled for execution

    // Errors
    NOT_STARTED,      ///< The executor has not been started
    ALREADY_RUNNING,  ///< The executor is already running another block
    NO_PARENT_BLOCK,  ///< The executor has not processed the
    UNABLE_TO_PLAN    ///< The execution manager is unable to plan execution,
                      ///< typically because resources issues
  };

  enum class State
  {
    IDLE = 0,  ///< The execution manager is waiting for new blocks to execute
    ACTIVE,    ///< The execution manager is in the process of executing a block

    // Stalled states
    TRANSACTIONS_UNAVAILABLE,  ///< The execution manager has stalled because transactions are
                               ///< unavailable

    // Error States
    EXECUTION_ABORTED,  ///< Execution has been stopped on user request
    EXECUTION_FAILED    ///< Execution has failed for a fundamental reason, the block can be
                        ///< considered as bad
  };

  // Construction / Destruction
  ExecutionManagerInterface()          = default;
  virtual ~ExecutionManagerInterface() = default;

  /// @name Execution Manager Interface
  /// @{
  virtual ScheduleStatus Execute(Block::Body const &block)              = 0;
  virtual void           SetLastProcessedBlock(v2::Digest block_digest) = 0;
  virtual v2::Digest     LastProcessedBlock()                           = 0;
  virtual State          GetState()                                     = 0;
  virtual bool           Abort()                                        = 0;
  /// @}
};

template <typename T>
void Serialize(T &serializer, ExecutionManagerInterface::ScheduleStatus const &status)
{
  serializer << static_cast<uint8_t>(status);
}

template <typename T>
void Deserialize(T &serializer, ExecutionManagerInterface::ScheduleStatus &status)
{
  uint8_t raw = 0xFF;
  serializer >> raw;
  status = static_cast<ExecutionManagerInterface::ScheduleStatus>(raw);
}

template <typename T>
void Serialize(T &serializer, ExecutionManagerInterface::State const &status)
{
  serializer << static_cast<uint8_t>(status);
}

template <typename T>
void Deserialize(T &serializer, ExecutionManagerInterface::State &status)
{
  uint8_t raw = 0xFF;
  serializer >> raw;
  status = static_cast<ExecutionManagerInterface::State>(raw);
}

/**
 * Convert schedule status into a string
 *
 * @param status The status to convert
 * @return The string representation of the status
 */
inline char const *ToString(ExecutionManagerInterface::ScheduleStatus status)
{
  using ScheduleStatus = ExecutionManagerInterface::ScheduleStatus;

  char const *reason = "Unknown";
  switch (status)
  {
  case ScheduleStatus::SCHEDULED:
    reason = "Scheduled";
    break;
  case ScheduleStatus::NOT_STARTED:
    reason = "Not Started";
    break;
  case ScheduleStatus::ALREADY_RUNNING:
    reason = "Already Running";
    break;
  case ScheduleStatus::NO_PARENT_BLOCK:
    reason = "No Parent Block";
    break;
  case ScheduleStatus::UNABLE_TO_PLAN:
    reason = "Unable to Plan";
    break;
  }

  return reason;
}

/**
 * Convert the Execution Manager state into a string
 *
 * @param state The state to be converted
 * @return The string representation of the state
 */
inline char const *ToString(ExecutionManagerInterface::State state)
{
  using State = ExecutionManagerInterface::State;

  char const *text = "Unknown";
  switch (state)
  {
  case State::IDLE:
    text = "Idle";
    break;
  case State::ACTIVE:
    text = "Active";
    break;
  case State::TRANSACTIONS_UNAVAILABLE:
    text = "Transaction(s) Unavailable";
    break;
  case State::EXECUTION_ABORTED:
    text = "Execution Aborted";
    break;
  case State::EXECUTION_FAILED:
    text = "Execution Failed";
    break;
  }

  return text;
}

}  // namespace ledger
}  // namespace fetch
