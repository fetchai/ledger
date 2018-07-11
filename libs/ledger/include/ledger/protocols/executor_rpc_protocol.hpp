#ifndef FETCH_EXECUTOR_RPC_PROTOCOL_HPP
#define FETCH_EXECUTOR_RPC_PROTOCOL_HPP

#include "network/service/protocol.hpp"
#include "ledger/executor.hpp"

namespace fetch {
namespace ledger {

class ExecutorRpcProtocol : public service::Protocol {
public:
  enum {
    EXECUTE = 1
  };

  explicit ExecutorRpcProtocol(ExecutorInterface& executor)
    : executor_{executor} {
    Expose(EXECUTE, &executor_, &ExecutorInterface::Execute);
  }

private:

  ExecutorInterface &executor_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTOR_RPC_PROTOCOL_HPP
