#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "chain/transaction.hpp"
#include "core/digest.hpp"
#include "core/mutex.hpp"
#include "ledger/storage_unit/transaction_pool_interface.hpp"

#include <unordered_map>

namespace fetch {
namespace ledger {

class TransactionMemoryPool : public TransactionPoolInterface
{
public:
  /// @name Transaction Storage Interface
  /// @{
  void     Add(chain::Transaction const &tx) override;
  bool     Has(Digest const &tx_digest) const override;
  bool     Get(Digest const &tx_digest, chain::Transaction &tx) const override;
  uint64_t GetCount() const override;
  void     Remove(Digest const &tx_digest) override;
  /// @}

private:
  using TxStore = DigestMap<chain::Transaction>;

  mutable Mutex lock_;
  TxStore       transaction_store_;
};

}  // namespace ledger
}  // namespace fetch
