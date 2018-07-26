#pragma once

#include "ledger/chain/block.hpp"
#include "ledger/chain/mutable_transaction.hpp"

namespace fetch {
namespace miner {

class MinerInterface
{
public:
  // Construction / Destruction
  MinerInterface()          = default;
  virtual ~MinerInterface() = default;

  /// @name Miner Interface
  /// @{
  virtual void EnqueueTransaction(chain::TransactionSummary const &tx) = 0;
  virtual void GenerateBlock(chain::BlockBody &block, std::size_t num_lanes,
                             std::size_t num_slices)                   = 0;
  /// @}
};

}  // namespace miner
}  // namespace fetch
