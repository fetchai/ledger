#pragma once

#include "ledger/chain/transaction.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "miner/miner_interface.hpp"

namespace fetch {
namespace ledger {

class TransactionProcessor {
public:

  TransactionProcessor(StorageUnitInterface &storage, miner::MinerInterface &miner)
    : storage_{storage}
    , miner_{miner} {
  }

  void AddTransaction(chain::Transaction const &tx) {

    // tell the node about the transaction
    storage_.AddTransaction(tx);

    // tell the miner about the transaction
    miner_.EnqueueTransaction(tx.summary());
  }

private:

  StorageUnitInterface &storage_;
  miner::MinerInterface &miner_;
};

} // namespace ledger
} // namespace fetch

