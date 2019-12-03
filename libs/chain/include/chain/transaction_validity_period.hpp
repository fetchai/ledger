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

#include "chain/transaction.hpp"

namespace fetch {
namespace chain {

template <typename TxOrTxLayout>
Transaction::Validity GetValidity(TxOrTxLayout const &tx, Transaction::BlockIndex block_index)
{
  auto const valid_until = tx.valid_until();

  auto const fallback_valid_from = valid_until >= Transaction::DEFAULT_TX_VALIDITY_PERIOD
                                       ? valid_until - Transaction::DEFAULT_TX_VALIDITY_PERIOD
                                       : 0u;

  auto const valid_from = tx.valid_from() == 0 ? fallback_valid_from : tx.valid_from();

  if (valid_until - valid_from > Transaction::MAXIMUM_TX_VALIDITY_PERIOD)
  {
    return Transaction::Validity::INVALID;
  }

  if (valid_until <= block_index)
  {
    return Transaction::Validity::INVALID;
  }

  if (valid_from > block_index)
  {
    return Transaction::Validity::PENDING;
  }

  return Transaction::Validity::VALID;
}

}  // namespace chain
}  // namespace fetch
