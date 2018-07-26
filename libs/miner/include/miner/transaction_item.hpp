#pragma once

#include "ledger/chain/mutable_transaction.hpp"

namespace fetch {
namespace miner {

class TransactionItem
{
public:
  // Construction / Destruction
  TransactionItem(chain::TransactionSummary const &tx, std::size_t id)
      : summary_(tx), id_(id)
  {}
  ~TransactionItem() = default;

  chain::TransactionSummary const &summary() const { return summary_; }

  std::size_t id() const { return id_; }

  // debug only
  std::unordered_set<std::size_t> lanes;

private:
  chain::TransactionSummary summary_;
  std::size_t               id_;
};

}  // namespace miner
}  // namespace fetch
