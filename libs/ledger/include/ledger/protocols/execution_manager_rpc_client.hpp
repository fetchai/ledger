#ifndef FETCH_EXECUTION_MANAGER_CLIENT_HPP
#define FETCH_EXECUTION_MANAGER_CLIENT_HPP

#include "network/service/client.hpp"
#include "ledger/execution_manager_interface.hpp"

#include <memory>

namespace fetch {
namespace ledger {

class ExecutionManagerRpcClient: public ExecutionManagerInterface {
public:
  using service_type = std::unique_ptr<fetch::service::ServiceClient>;

  // Construction / Destruction
  ExecutionManagerRpcClient(byte_array::ConstByteArray const &host, uint16_t const &port,
                            network::ThreadManager const &thread_manager);
  ~ExecutionManagerRpcClient() override = default;

  /// @name Execution Manager Interface
  /// @{
  bool Execute(block_digest_type const &block_hash,
               block_digest_type const &prev_block_hash,
               tx_index_type const &index,
               block_map_type &map,
               std::size_t num_lanes,
               std::size_t num_slices) override;
  block_digest_type LastProcessedBlock() override;
  bool IsActive() override;
  bool IsIdle() override;
  bool Abort() override;
  /// @}

  bool is_alive() const {
    return service_->is_alive();
  }

private:

  service_type service_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_CLIENT_HPP
