#ifndef FETCH_EXECUTION_MANAGER_PROTOCOL_HPP
#define FETCH_EXECUTION_MANAGER_PROTOCOL_HPP

#include "network/service/protocol.hpp"
#include "ledger/execution_manager_interface.hpp"

namespace fetch {
namespace ledger {

class ExecutionManagerRpcProtocol : public fetch::service::Protocol {
public:
  enum {
    EXECUTE = 1,
    LAST_PROCESSED_BLOCK,
    IS_ACTIVE,
    IS_IDLE,
    ABORT
  };

  explicit ExecutionManagerRpcProtocol(ExecutionManagerInterface &manager)
    : manager_(manager) {

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

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_PROTOCOL_HPP
