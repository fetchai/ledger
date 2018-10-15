#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
    IS_ACTIVE,
    IS_IDLE,
    ABORT
  };

  explicit ExecutionManagerRpcProtocol(ExecutionManagerInterface &manager)
    : manager_(manager)
  {
    // define the RPC endpoints
    Expose(EXECUTE, this, &ExecutionManagerRpcProtocol::Execute);
    Expose(LAST_PROCESSED_BLOCK, &manager_, &ExecutionManagerInterface::LastProcessedBlock);
    Expose(IS_ACTIVE, &manager_, &ExecutionManagerInterface::IsActive);
    Expose(IS_IDLE, &manager_, &ExecutionManagerInterface::IsIdle);
    Expose(ABORT, &manager_, &ExecutionManagerInterface::Abort);
  }

private:
  using Status    = ExecutionManagerInterface::Status;
  using Block     = ExecutionManagerInterface::Block;
  using FullBlock = chain::MainChain::BlockType;

  Status Execute(Block const &block)
  {
    // since the hash is not serialised we need to recalculate it
    FullBlock ful_block{};
    ful_block.SetBody(block);
    ful_block.UpdateDigest();

    return manager_.Execute(ful_block.body());
  }

  ExecutionManagerInterface &manager_;
};

}  // namespace ledger
}  // namespace fetch
