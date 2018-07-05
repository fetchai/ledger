#ifndef FETCH_EXECUTION_MANAGER_PROTOCOL_HPP
#define FETCH_EXECUTION_MANAGER_PROTOCOL_HPP

#include "network/service/protocol.hpp"
#include "ledger/execution_manager_interface.hpp"

namespace fetch {
namespace ledger {

class ExecutionManagerRpcProtocol : public fetch::service::Protocol {
public:
  enum {
    RPC_ID_EXECUTE = 1
  };

  explicit ExecutionManagerRpcProtocol(ExecutionManagerInterface &manager)
    : manager_(manager) {

    // define the RPC endpoints
    Expose(RPC_ID_EXECUTE, &manager_, &ExecutionManagerInterface::Execute);
  }

private:

  ExecutionManagerInterface &manager_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_PROTOCOL_HPP
