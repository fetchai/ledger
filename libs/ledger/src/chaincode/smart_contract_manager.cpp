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
#include "ledger/state_sentinel.hpp"

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

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ToBase64;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace ledger {

ConstByteArray const CONTRACT_SOURCE{"text"};
ConstByteArray const CONTRACT_HASH{"digest"};

SmartContractManager::SmartContractManager()
{
  OnTransaction("create", this, &SmartContractManager::OnCreate);
}

Contract::Status SmartContractManager::OnCreate(Transaction const &tx)
{
  // attempt to parse the transaction
  variant::Variant data;
  if (!ParseAsJson(tx, data))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "FAILED TO PARSE TRANSACTION");
    return Status::FAILED;  // TODO(HUT):  // discuss with ed
  }

  ConstByteArray contract_source;
  ConstByteArray contract_hash;

  // extract the fields from the contract
  bool const extract_success = Extract(data, CONTRACT_HASH, contract_hash) &&
                               Extract(data, CONTRACT_SOURCE, contract_source);

  // fail if the extraction fails
  if (!extract_success)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse contract source from transaction body");
    return Status::FAILED;
  }

  // decode the contents of the
  contract_source = FromBase64(contract_source);

  // calculate a hash to compare against the one submitted
  auto const calculated_hash = ToBase64(crypto::Hash<crypto::SHA256>(contract_source));

  if (calculated_hash != contract_hash)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Warning! Failed to match calculated hash with provided hash: ",
                   calculated_hash, " to ", contract_hash);

    return Status::FAILED;
  }

  FETCH_LOG_WARN(LOGGING_NAME, "Adding smart contract, ID: ", calculated_hash);

  // store the contract
  SetStateRecord(contract_source, calculated_hash);

  return Status::OK;
}

/**
 * Used to generate resource address for the storage of the smart contract code
 *
 * @param contract_id The identifier for the smart contract being stored
 * @return The generated address
 */
storage::ResourceAddress SmartContractManager::CreateAddressForContract(Identifier const &contract_id)
{
  // this function is only really applicable to the storage of smart contracts
  assert(contract_id.type() == Identifier::Type::SMART_CONTRACT);

  // create the resource address in the form fetch.contracts.state.<digest of contract>
  return StateAdapter::CreateAddress(Identifier{NAME}, contract_id.qualifier());
}

}  // namespace ledger
}  // namespace fetch
