#ifndef FETCH_EXECUTION_MANAGER_CLIENT_HPP
#define FETCH_EXECUTION_MANAGER_CLIENT_HPP

#include "ledger/execution_manager_interface.hpp"
#include "network/service/client.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class ExecutionManagerRpcClient : public ExecutionManagerInterface
{
public:
  using service_type = std::unique_ptr<fetch::service::ServiceClient>;

  // Construction / Destruction
  ExecutionManagerRpcClient(byte_array::ConstByteArray const &host,
                            uint16_t const &                  port,
                            network::NetworkManager const &   network_manager);
  ~ExecutionManagerRpcClient() override = default;

  /// @name Execution Manager Interface
  /// @{
  Status            Execute(block_type const &block) override;
  block_digest_type LastProcessedBlock() override;
  bool              IsActive() override;
  bool              IsIdle() override;
  bool              Abort() override;
  /// @}

  bool is_alive() const { return service_->is_alive(); }

private:
  service_type service_;
};

}  // namespace ledger
}  // namespace fetch

#endif  // FETCH_EXECUTION_MANAGER_CLIENT_HPP
