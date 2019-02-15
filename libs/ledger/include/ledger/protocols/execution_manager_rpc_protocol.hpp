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

#include "ledger/chain/main_chain.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "network/service/protocol.hpp"

namespace fetch {
namespace ledger {

class ExecutionManagerRpcProtocol : public fetch::service::Protocol
{
public:
  enum
  {
    EXECUTE = 1,
    LAST_PROCESSED_BLOCK,
    GET_STATE,
    ABORT,
    SET_LAST_PROCESSED_BLOCK
  };

  explicit ExecutionManagerRpcProtocol(ExecutionManagerInterface &manager)
    : manager_(manager)
  {
    // define the RPC endpoints
    Expose(EXECUTE, this, &ExecutionManagerRpcProtocol::Execute);
    Expose(SET_LAST_PROCESSED_BLOCK, &manager_, &ExecutionManagerInterface::SetLastProcessedBlock);
    Expose(LAST_PROCESSED_BLOCK, &manager_, &ExecutionManagerInterface::LastProcessedBlock);
    Expose(GET_STATE, &manager_, &ExecutionManagerInterface::GetState);
    Expose(ABORT, &manager_, &ExecutionManagerInterface::Abort);
  }

private:
  using ScheduleStatus = ExecutionManagerInterface::ScheduleStatus;

  ScheduleStatus Execute(Block::Body const &block_body)
  {
    // since the hash is not serialised we need to recalculate it
    Block full_block{};
    full_block.body = block_body;
    full_block.UpdateDigest();

    return manager_.Execute(full_block.body);
  }

  ExecutionManagerInterface &manager_;
};

}  // namespace ledger
}  // namespace fetch
