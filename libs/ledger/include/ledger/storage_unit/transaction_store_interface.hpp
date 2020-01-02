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

#include "core/digest.hpp"

namespace fetch {
namespace chain {

class Transaction;

}  // namespace chain

namespace ledger {

class TransactionStoreInterface
{
public:
  // Construction / Destruction
  TransactionStoreInterface()          = default;
  virtual ~TransactionStoreInterface() = default;

  /// @name Transaction Storage Interface
  /// @{

  /**
   * Add a transaction to the store
   *
   * @param tx The transaction to set added to storage
   */
  virtual void Add(chain::Transaction const &tx) = 0;

  /**
   * Check to see if requested transaction exists
   *
   * @param tx_digest The transaction digest to be searched for
   * @return true if present, otherwise false
   */
  virtual bool Has(Digest const &tx_digest) const = 0;

  /**
   * Lookup a transaction from the store
   *
   * @param tx_digest The transaction digest to lookup
   * @param tx
   * @return
   */
  virtual bool Get(Digest const &tx_digest, chain::Transaction &tx) const = 0;

  /**
   * Get the total number of transactions in this store
   *
   * @return The number of transactions stored
   */
  virtual uint64_t GetCount() const = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
