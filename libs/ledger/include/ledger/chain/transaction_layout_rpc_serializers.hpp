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

#include "ledger/chain/transaction_layout.hpp"

namespace fetch {
namespace ledger {

template <typename T>
void Serialize(T &s, TransactionLayout const &tx)
{
  s << tx.digest_;
  Serialize(s, tx.mask_);
  s << tx.charge_ << tx.valid_from_ << tx.valid_until_;
}

template <typename T>
void Deserialize(T &s, TransactionLayout &tx)
{
  s >> tx.digest_;
  Deserialize(s, tx.mask_);
  s >> tx.charge_ >> tx.valid_from_ >> tx.valid_until_;
}

}  // namespace ledger
}  // namespace fetch
