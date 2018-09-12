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

#include "ledger/chain/wired_transaction.hpp"
#include "core/json/document.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/script/variant.hpp"

namespace fetch {
namespace ledger {

byte_array::ByteArray ToWireTransaction(MutableTransaction const &transaction, bool const addDebugInfo=false)
{
  script::Variant tx_v;
  tx_v["ver"] = "1.0";

  if (addDebugInfo)
  {
    script::Variant tx_data_to_sign;
    tx_data_to_sign["data"] = byte_array::ToBase64(transaction.data());
    tx_data_to_sign["fee"]  = transaction.summary().fee;
    tx_data_to_sign["contract_name"] = transaction.contract_name();

    script::VariantArray resources;
    resources.CopyFrom(transaction.resources().begin(), transaction.resources().end());

    tx_data_to_sign["resources"] = resources;
    tx_v.["debug_info"] = tx_data_to_sign;
  }

  //tx_v["data"] =
  return byte_array::ByteArray();
}

MutableTransaction FromWireTransaction(byte_array::ConstByteArray const &transaction)
{
  return Muta
}

}  // namespace ledger
}  // namespace fetch
