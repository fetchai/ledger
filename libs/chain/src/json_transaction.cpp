//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "chain/json_transaction.hpp"
#include "chain/transaction.hpp"
#include "chain/transaction_serializer.hpp"
#include "core/byte_array/decoders.hpp"
#include "logging/logging.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

namespace fetch {
namespace chain {

using variant::Variant;
using variant::Extract;
using byte_array::ConstByteArray;
using byte_array::FromBase64;

static constexpr char const *LOGGING_NAME        = "JsonTx";
static const ConstByteArray  JSON_FORMAT_VERSION = "1.2";

/**
 * Convert an input JSON object into a transaction
 *
 * @param src The variant to decode into a transaction
 * @param dst The transaction to be populated
 */
bool FromJsonTransaction(Variant const &src, Transaction &dst)
{
  // determine if this payload is of the correct version
  ConstByteArray version{};
  if (!Extract(src, "ver", version))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No version field present in payload");
    return false;
  }

  // ensure that the version matches expectation
  if (JSON_FORMAT_VERSION != version)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Unexpected version: ", version);
    return false;
  }

  // extract the data field
  ConstByteArray data{};
  if (!Extract(src, "data", data))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No data field present in payload");
    return false;
  }

  // convert the data field to binary
  data = FromBase64(data);

  // create the serializer and try and deserialize the transaction
  TransactionSerializer serializer{data};
  if (!serializer.Deserialize(dst))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "No data field present in payload");
    return false;
  }

  return true;
}

/**
 * Convert a Transaction into a JSON object
 *
 * @param src The transaction to be encoded into JSON
 * @param dst The destination variant object
 */
bool ToJsonTransaction(Transaction const &src, Variant &dst, bool include_metadata)
{
  bool success{false};

  TransactionSerializer serializer{};
  if (serializer.Serialize(src))
  {
    // create the destination object
    dst = Variant::Object();

    // populate the mandatory field
    dst["ver"]  = JSON_FORMAT_VERSION;
    dst["data"] = serializer.data().ToBase64();

    if (include_metadata)
    {
      // build update the metadata object
      auto &metadata = dst["metadata"] = Variant::Object();

      metadata["digest"] = src.digest().ToHex();
    }

    // signal success
    success = true;
  }

  return success;
}

}  // namespace chain
}  // namespace fetch
