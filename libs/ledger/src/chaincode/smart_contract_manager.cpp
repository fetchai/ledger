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

#include "ledger/chaincode/smart_contract_manager.hpp"

#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chaincode/vm_definition.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include "core/byte_array/encoders.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace fetch {
namespace ledger {

byte_array::ConstByteArray const CONTRACT_SOURCE{"contract_source_b64"};
byte_array::ConstByteArray const CONTRACT_HASH{"contract_hash_b64"};

SmartContractManager::SmartContractManager()
  : Contract("fetch.smart_contract_manager")
{
  OnTransaction("create_initial_contract", this, &SmartContractManager::CreateInitialContract);
}

Contract::Status SmartContractManager::CreateInitialContract(Transaction const &tx)
{
  variant::Variant data;
  if (!ParseAsJson(tx, data))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "FAILED TO PARSE TRANSACTION");
    return Status::FAILED;  // TODO(HUT):  // discuss with ed
  }

  byte_array::ConstByteArray contract_source;
  byte_array::ConstByteArray contract_hash;

  if (Extract(data, CONTRACT_SOURCE, contract_source))
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Extracted source from TX");
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse contract source from transaction body");
    return Status::OK;
  }

  // Not strictly needed
  Extract(data, CONTRACT_HASH, contract_hash);

  auto source_hashed = ToBase64(crypto::Hash<crypto::SHA256>(contract_source));

  if (source_hashed != contract_hash)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Warning! Failed to match calculated hash with provided hash: ",
                   ToBase64(source_hashed), " to ", contract_hash);
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Adding smart contract, ID: ", source_hashed);

  // Source stored as base64, against its hash base64
  SetRawState(contract_source, source_hashed);

  if (!CheckRawState(source_hashed))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to find SC after trying to set it! : ", source_hashed,
                    " in main DB");
    return Status::FAILED;
  }

  FETCH_LOG_WARN(LOGGING_NAME, "<---------------- Added smart contract, ID: ", source_hashed);

  return Status::OK;
}

}  // namespace ledger
}  // namespace fetch
