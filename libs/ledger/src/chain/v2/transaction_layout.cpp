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
  : digest_{tx.digest()}
  , mask_{tx.shard_mask()}
  , charge_{tx.charge()}
  , valid_from_{tx.valid_from()}
  , valid_until_{tx.valid_until()}
{
}

} // namespace v2
} // namespace ledger
} // namespace fetch
