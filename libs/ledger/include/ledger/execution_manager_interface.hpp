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
  using Block     = chain::BlockBody;
  using BlockHash = Block::Digest;

  enum class Status
  {
    COMPLETE = 0,  ///< The block has been successfully applied/executed
    SCHEDULED,     ///< The block has been successfully scheduled

    // Errors
    NOT_STARTED,      ///< The executor has not been started
    ALREADY_RUNNING,  ///< The executor is already running another block
    NO_PARENT_BLOCK,  ///< The executor has not processed the
    UNABLE_TO_PLAN    ///< The execution manager is unable to plan execution,
                      ///< typically because resources issues
  };

  // Construction / Destruction
  ExecutionManagerInterface()          = default;
  virtual ~ExecutionManagerInterface() = default;

  /// @name Execution Manager Interface
  /// @{
  virtual Status    Execute(Block const &block) = 0;
  virtual BlockHash LastProcessedBlock()        = 0;
  virtual bool      IsActive()                  = 0;
  virtual bool      IsIdle()                    = 0;
  virtual bool      Abort()                     = 0;
  /// @}
};

template <typename T>
void Serialize(T &serializer, ExecutionManagerInterface::Status const &status)
{
  serializer << static_cast<uint8_t>(status);
}

template <typename T>
void Deserialize(T &serializer, ExecutionManagerInterface::Status &status)
{
  uint8_t raw = 0xFF;
  serializer >> raw;
  status = static_cast<ExecutionManagerInterface::Status>(raw);
}

}  // namespace ledger
}  // namespace fetch
