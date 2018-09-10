#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/chain/mutable_transaction.hpp"
#include "core/json/document.hpp"

namespace fetch {
namespace chain {

byte_array::ByteArray ToWireTransaction(MutableTransaction const& transaction)
{
  script::Variant tx_v;
  tx_v["ver"] = "1.0";
  script::Variant tx_data_to_sign;
  tx_data_to_sign["data"] = transaction.data();
  tx_data_to_sign["fee"] = transaction.summary().fee;
  tx_v[""]
   

}

MutableTransaction FromWireTransaction(byte_array::ConstByteArray const& transaction)
{
}

}  // namespace chain
}  // namespace fetch

