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
  tx_v.MakeObject();
  //TODO(pbukva) (private issue: find nice way to deal with versioning of raw (C++) transaction and version of wire format)
  tx_v["ver"] = "1.0";

  if (addDebugInfo)
  {
    script::Variant tx_debug_data;
    tx_debug_data.MakeObject();
    tx_debug_data["data"] = byte_array::ToBase64(tx.data());
    tx_debug_data["fee"]  = tx.summary().fee;
    tx_debug_data["contract_name"] = tx.contract_name();

    script::VariantArray resources;
    resources.CopyFrom(tx.resources().begin(), tx.resources().end());

    tx_debug_data["resources"] = resources;
    tx_v["dbg"] = tx_debug_data;
  }

  auto txdfs{TxDataForSigningCFactory(tx)};
  tx_v["data"] = byte_array::ToBase64(txdfs.DataForSigning());

  script::VariantArray signatures{ tx.signatures().size() };
  std::size_t i = 0;
  std::size_t identity_serialised_size = 0;
  auto eval_identity_size = serializers::LazyEvalArgumentFactory([&identity_serialised_size](auto& stream){
    identity_serialised_size = stream.size();
  });

  serializers::ByteArrayBuffer stream;
  for( auto const& sig: tx.signatures())
  {
    auto &sig_v = signatures[i++];
    sig_v.MakeObject();
    stream.Resize(0, serializers::eResizeParadigm::absolute);
    stream.Append(sig.first, eval_identity_size, sig.second);
    sig_v[byte_array::ToBase64(stream.data().SubArray(0, identity_serialised_size))] = byte_array::ToBase64(stream.data().SubArray(identity_serialised_size, stream.data().size()-identity_serialised_size));
  }

  if (signatures.size() > 0 )
  {
    tx_v["signatures"] = signatures;
  }

  std::stringstream str_stream;
  str_stream << tx_v;
  return str_stream.str();
}

MutableTransaction FromWireTransaction(byte_array::ConstByteArray const &transaction)
{
  MutableTransaction tx;

  json::JSONDocument tx_json{ transaction };
  auto &tx_v = tx_json.root();
  
  serializers::ByteArrayBuffer stream{ byte_array::FromBase64(tx_v["data"].As<byte_array::ByteArray>()) };
  auto txdata {TxDataForSigningCFactory(tx)};
  stream >> txdata;

  auto const &signatures = tx_v["signatures"];

  MutableTransaction::signatures_type mtx_signatures;

  signatures.ForEach([&mtx_signatures](script::Variant const &sig_pair_v) -> bool{
    sig_pair_v.ForEach([&mtx_signatures](script::Variant const &identity_v, script::Variant const &signature_v) -> bool{
      auto identity_bin = byte_array::FromBase64(identity_v.As<byte_array::ByteArray>());
      auto signature_bin = byte_array::FromBase64(signature_v.As<byte_array::ByteArray>());
      serializers::ByteArrayBuffer i_stream{ identity_bin };
      crypto::Identity identity;
      i_stream >> identity;

      serializers::ByteArrayBuffer s_stream{ signature_bin };
      Signature signature;
      s_stream >> signature;
      mtx_signatures[identity] = signature;
      return true;
    });
    return true;
  });

  tx.set_signatures(mtx_signatures);

  return tx;
}

}  // namespace chain
}  // namespace fetch
