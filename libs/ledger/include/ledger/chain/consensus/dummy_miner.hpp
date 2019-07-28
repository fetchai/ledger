#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "ledger/chain/consensus/consensus_miner_interface.hpp"
#include "ledger/chain/transaction_layout_rpc_serializers.hpp"
#include "ledger/chain/transaction_rpc_serializers.hpp"

namespace fetch {
namespace ledger {
namespace consensus {

class DummyMiner : public ConsensusMinerInterface
{
public:
  // Construction / Destruction
  DummyMiner()                   = default;
  DummyMiner(DummyMiner const &) = delete;
  DummyMiner(DummyMiner &&)      = delete;
  ~DummyMiner() override         = default;

  /// @name Consensus Miner Interface
  /// @{
  void Mine(Block &block) override;
  bool Mine(Block &block, uint64_t iterations) override;
  /// @}

  // Operators
  DummyMiner &operator=(DummyMiner const &) = delete;
  DummyMiner &operator=(DummyMiner &&) = delete;
};
}  // namespace consensus
}  // namespace ledger
}  // namespace fetch
