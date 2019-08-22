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

namespace serializers {

template <typename D>
struct MapSerializer<ledger::TransactionLayout, D>
{
public:
  using Type       = ledger::TransactionLayout;
  using DriverType = D;

  static uint8_t const DIGEST      = 1;
  static uint8_t const MASK        = 2;
  static uint8_t const CHARGE      = 3;
  static uint8_t const VALID_FROM  = 4;
  static uint8_t const VALID_UNTIL = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &tx)
  {
    auto map = map_constructor(5);
    map.Append(DIGEST, tx.digest_);
    map.Append(MASK, tx.mask_);
    map.Append(CHARGE, tx.charge_);
    map.Append(VALID_FROM, tx.valid_from_);
    map.Append(VALID_UNTIL, tx.valid_until_);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &tx)
  {
    map.ExpectKeyGetValue(DIGEST, tx.digest_);
    map.ExpectKeyGetValue(MASK, tx.mask_);
    map.ExpectKeyGetValue(CHARGE, tx.charge_);
    map.ExpectKeyGetValue(VALID_FROM, tx.valid_from_);
    map.ExpectKeyGetValue(VALID_UNTIL, tx.valid_until_);
  }
};

}  // namespace serializers

}  // namespace fetch
