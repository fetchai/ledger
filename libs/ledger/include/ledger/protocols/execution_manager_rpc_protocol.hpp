#pragma once

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

  explicit ExecutionManagerRpcProtocol(ExecutionManagerInterface &manager) : manager_(manager)
  {

    // define the RPC endpoints
    Expose(EXECUTE, &manager_, &ExecutionManagerInterface::Execute);
    Expose(LAST_PROCESSED_BLOCK, &manager_, &ExecutionManagerInterface::LastProcessedBlock);
    Expose(IS_ACTIVE, &manager_, &ExecutionManagerInterface::IsActive);
    Expose(IS_IDLE, &manager_, &ExecutionManagerInterface::IsIdle);
    Expose(ABORT, &manager_, &ExecutionManagerInterface::Abort);
  }

private:
  ExecutionManagerInterface &manager_;
};

}  // namespace ledger
}  // namespace fetch
