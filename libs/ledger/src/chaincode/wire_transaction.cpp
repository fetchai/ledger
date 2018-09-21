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

#include "ledger/chain/wire_transaction.hpp"
#include "core/json/document.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/script/variant.hpp"

#include <sstream>

namespace fetch {
namespace chain {

byte_array::ByteArray ToWireTransaction(MutableTransaction const &tx, bool const addDebugInfo)
{
  script::Variant tx_v;
  //TODO(pbukva) (private issue: find nice way to deal with versioning of raw (C++) transaction and version of wire format)
  tx_v["ver"] = "1.0";

  if (addDebugInfo)
  {
    script::Variant tx_data_to_sign;
    tx_data_to_sign["data"] = byte_array::ToBase64(tx.data());
    tx_data_to_sign["fee"]  = tx.summary().fee;
    tx_data_to_sign["contract_name"] = tx.contract_name();

    script::VariantArray resources;
    resources.CopyFrom(tx.resources().begin(), tx.resources().end());

    tx_data_to_sign["resources"] = resources;
    tx_v["dbg"] = tx_data_to_sign;
  }

  //TxDataForSigningC<typename std::remove_reference<decltype(tx)>::type> txdfs{tx};
  auto txdfs{TxDataForSigningCFactory(tx)};
  tx_v["data"] = byte_array::ToBase64(txdfs.DataForSigning());

  script::VariantArray signatures;
  for( auto const& sig: tx.signatures())
  {
    script::Variant sig_v;
    sig_v[byte_array::ToBase64(sig.first.identifier())] = byte_array::ToBase64(sig.second.signature_data);
  }
  if (signatures.size() > 0 )
  {
    tx_v["signatures"] = signatures;
  }

  std::stringstream stream;
  stream << tx_v;
  return stream.str();
}

MutableTransaction FromWireTransaction(byte_array::ConstByteArray const &transaction)
{
  MutableTransaction tx;

  json::JSONDocument tx_json{ transaction };
  auto &tx_v = tx_json.root();
  
  serializers::ByteArrayBuffer stream{ byte_array::FromBase64(tx_v["data"].As<byte_array::ByteArray>()) };
  auto txdata {TxDataForSigningCFactory(tx)};
  stream >> txdata;
  return tx;
}

}  // namespace chain
}  // namespace fetch
