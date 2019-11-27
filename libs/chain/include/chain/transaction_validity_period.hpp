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
  constexpr Transaction::BlockIndex MAXIMUM_TX_VALIDITY_PERIOD = 40000;
  constexpr Transaction::BlockIndex DEFAULT_TX_VALIDITY_PERIOD = 1000;

  auto const default_valid_from = tx.valid_until() >= DEFAULT_TX_VALIDITY_PERIOD
                                      ? tx.valid_until() - DEFAULT_TX_VALIDITY_PERIOD
                                      : 0u;
  auto const valid_from = tx.valid_from() == 0 ? default_valid_from : tx.valid_from();

  if (tx.valid_until() - valid_from > MAXIMUM_TX_VALIDITY_PERIOD)
  {
    return Transaction::Validity::INVALID;
  }

  if (tx.valid_until() < block_index)
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
