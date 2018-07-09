#ifndef FETCH_EXECUTION_MANAGER_INTERFACE_HPP
#define FETCH_EXECUTION_MANAGER_INTERFACE_HPP

#include "ledger/chain/block.hpp"

namespace fetch {
namespace ledger {

class ExecutionManagerInterface {
public:
  using tx_digest_type = chain::Transaction::digest_type;
  using tx_index_type = std::vector<tx_digest_type>;
  using block_map_type = std::vector<uint64_t>;
  using block_digest_type = byte_array::ConstByteArray;


  virtual bool Execute(block_digest_type const &block_hash,
                       block_digest_type const &prev_block_hash,
                       tx_index_type const &index,
                       block_map_type &map,
                       std::size_t num_lanes,
                       std::size_t num_slices) = 0;
  virtual block_digest_type LastProcessedBlock() = 0;
  virtual bool IsActive() = 0;
  virtual bool IsIdle() = 0;
  virtual bool Abort() = 0;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTION_MANAGER_INTERFACE_HPP
