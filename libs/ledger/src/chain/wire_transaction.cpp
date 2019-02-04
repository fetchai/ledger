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

#include "ledger/chain/wire_transaction.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/serializers/serialisation_argument_wrapper.hpp"
#include "variant/variant.hpp"

#include <sstream>

using fetch::variant::Variant;
using fetch::byte_array::ToBase64;
using fetch::byte_array::FromBase64;
using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace ledger {

byte_array::ByteArray ToWireTransaction(MutableTransaction const &tx, bool const add_metadata)
{
  Variant tx_v = Variant::Object();

  // TODO(pbukva) (private issue: find nice way to deal with versioning of raw (C++) transaction and
  // version of wire format)
  tx_v["ver"] = "1.0";

  if (add_metadata)
  {
    Variant &metadata = tx_v["metadata"];
    metadata          = Variant::Object();

    metadata["data"]          = byte_array::ToBase64(tx.data());
    metadata["fee"]           = tx.summary().fee;
    metadata["contract_name"] = tx.contract_name();

    Variant &resources = metadata["resource"];
    resources          = Variant::Array(tx.resources().size());

    // encode all the resources as base64
    std::size_t idx{0};
    for (auto const &resource : tx.resources())
    {
      resources[idx++] = ToBase64(resource);
    }

    if (!tx.signatures().empty())
    {
      Variant &signatures = metadata["identities"];
      signatures          = Variant::Array(tx.signatures().size());

      std::size_t idx{0};
      for (auto const &signature : tx.signatures())
      {
        signatures[idx++] = ToBase64(signature.first.identifier());
      }
    }
  }

  // generate the wire format for the transaction
  serializers::ByteArrayBuffer stream;
  stream.Append(TxSigningAdapterFactory(tx));

  tx_v["data"] = ToBase64(stream.data());

  // encode the transaction object into JSON
  std::stringstream str_stream;
  str_stream << tx_v;

  return str_stream.str();
}

MutableTransaction FromWireTransaction(ConstByteArray const &transaction)
{
  json::JSONDocument tx_json{transaction};
  return FromWireTransaction(tx_json.root());
}

MutableTransaction FromWireTransaction(variant::Variant const &transaction)
{
  MutableTransaction tx;

  serializers::ByteArrayBuffer stream{FromBase64(transaction["data"].As<ConstByteArray>())};
  auto                         tx_data = TxSigningAdapterFactory(tx);
  stream >> tx_data;

  return tx;
}

}  // namespace ledger
}  // namespace fetch
