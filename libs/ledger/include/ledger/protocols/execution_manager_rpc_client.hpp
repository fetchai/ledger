#ifndef FETCH_EXECUTION_MANAGER_CLIENT_HPP
#define FETCH_EXECUTION_MANAGER_CLIENT_HPP

#include "network/service/client.hpp"
#include "ledger/execution_manager_interface.hpp"

#include<memory>
namespace fetch {
namespace ledger {

class ExecutionManagerRpcClient: public ExecutionManagerInterface {
public:

  ExecutionManagerRpcClient(byte_array::ConstByteArray const &host, uint16_t const &port,
                            network::ThreadManager const &thread_manager);

  bool Execute(tx_index_type const &index,
               block_map_type &map,
               std::size_t num_lanes,
               std::size_t num_slices) override;
private:

  std::unique_ptr<fetch::service::ServiceClient > service_;
  
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_CLIENT_HPP
