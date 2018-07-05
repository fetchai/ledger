#include "ledger/lane.hpp"

namespace fetch {
namespace ledger {

/**
 * Add Transaction to the current lane
 * @param tx The transaction to be added to the store
 */
void Lane::AddTransaction(shared_tx_type tx) {
  // ensure the transaction doesn't already exist
  if (tx) {
    if (tx_store_.find(tx->digest()) != tx_store_.end()) {
      tx_store_[tx->digest()] = tx;
    }
  }
}

/**
 * Add a block slice to the chain
 *
 * @param block_hash The block hash
 * @param hash_list The list of transaction hashes
 */
void Lane::AddBlockSlice(block_hash_type const &block_hash, tx_hash_list_type hash_list) {
  side_chain_[block_hash] = std::move(hash_list);
}

/**
 * Trigger the start of a block
 *
 * @param hash The block hash
 * @param previous  The previous block hash
 * @return true if successful, otherwise false
 */
bool Lane::StartBlock(block_hash_type const &hash, block_hash_type const &previous) {
  // failed the validate
  if (!Validate(hash)) {
    return false;
  }

  return false;
}

/**
 * Advance the slot for the lane
 */
void Lane::AdvanceSlot() {

}

/**
 * Validate that a lane has all the required components needed in order to process a block
 * @param hash The block hash
 * @return true if ready to process the block, otherwise false.
 */
bool Lane::Validate(block_hash_type const &hash) {
  bool success = false;

  auto it = side_chain_.find(hash);
  if (it != side_chain_.end()) {
    success = true;
    for (auto const &tx_hash : it->second) {
      if (tx_store_.find(tx_hash) == tx_store_.end()) {
        success = false;
      }
    }
  }

  return success;
}

} // namespace ledger
} // namespace fetch
