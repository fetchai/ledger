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

#include "ledger/chain/v2/transaction_layout.hpp"
#include "ledger/chain/v2/transaction.hpp"

namespace fetch {
namespace ledger {
namespace v2 {

/**
 * Construct a transaction layout from the specified transaction
 *
 * @param tx The input transaction
 */
TransactionLayout::TransactionLayout(Transaction const &tx)
  : TransactionLayout(tx.digest(), tx.shard_mask(), tx.charge(), tx.valid_from(), tx.valid_until())
{}

/**
 * Construct a transaction layout from the specified reference layout and specified mask
 * @param tx The main contents of the transaction
 * @param mask The new mask to be applied
 */
TransactionLayout::TransactionLayout(TransactionLayout const &tx, BitVector const &mask)
  : TransactionLayout(tx.digest(), mask, tx.charge(), tx.valid_from(), tx.valid_until())
{
}

/**
 * Construct a transaction layout from its constituent parts
 *
 * @param digest The digest to be set
 * @param mask The mask to be set
 * @param charge The charge to be set
 * @param valid_from The block index from which point the transaction is valid
 * @param valid_until The block index from which point the transaction is no longer valid
 */
TransactionLayout::TransactionLayout(Digest digest, BitVector const &mask, TokenAmount charge,
                                     BlockIndex valid_from, BlockIndex valid_until)
  : digest_{std::move(digest)}
  , mask_{mask}
  , charge_{charge}
  , valid_from_{valid_from}
  , valid_until_{valid_until}
{
}

}  // namespace v2
}  // namespace ledger
}  // namespace fetch
