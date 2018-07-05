#ifndef FETCH_LANE_HPP
#define FETCH_LANE_HPP

#include "crypto/fnv.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/contract_execution_interface.hpp"
#include "ledger/lane_interface.hpp"
#include "ledger/state_database.hpp"
#include "storage/document_store.hpp"

#include <memory>
#include <functional>
#include <unordered_map>

namespace fetch {
namespace ledger {

/**
 * Lane Implementation
 */
class Lane : public LaneInterface {
public:
  using shared_tx_type = std::shared_ptr<transaction_type>;
  using tx_store_type = std::unordered_map<transaction_hash_type , shared_tx_type, crypto::CallableFNV>;
  using block_hash_type = fetch::byte_array::ConstByteArray;
  using tx_hash_list_type = std::vector<transaction_hash_type>;
  using side_chain_type = std::unordered_map<block_hash_type, tx_hash_list_type, crypto::CallableFNV>;
  using state_database_type = StateDatabase;
  using slot_completion_cb_type = std::function<void()>;

  /// @name Block based interface
  /// @{
  void AddTransaction(shared_tx_type tx);
  void AddBlockSlice(block_hash_type const &block_hash, tx_hash_list_type hash_list);
  bool StartBlock(block_hash_type const &hash, block_hash_type const &previous);
  void AdvanceSlot();
  /// @}

  void SetSlotCompleteHandler(slot_completion_cb_type callback) {
    slot_complete_ = std::move(callback);
  }

  state_database_type &state_database() {
    return state_db_;
  }

private:

  tx_store_type tx_store_{};

  side_chain_type side_chain_{};
  state_database_type state_db_{};

  // callbacks
  slot_completion_cb_type slot_complete_{};

  //void StoreTransaction(Trans);

  bool Validate(block_hash_type const &hash);
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_LANE_HPP
