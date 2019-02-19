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

#include "vm/vm_factory.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace fetch {
namespace ledger {

byte_array::ConstByteArray const CONTRACT_SOURCE{"contract_source"};
byte_array::ConstByteArray const CONTRACT_HASH{"contract_hash"};

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
    return Status::FAILED;
  }

  byte_array::ConstByteArray contract_source;
  byte_array::ByteArray contract_build_up;
  byte_array::ByteArray contract_build_up_final;
  byte_array::ByteArray contract_with_quotes{'"', 1};
  contract_with_quotes.Resize(1);

  if (Extract(data, CONTRACT_SOURCE, contract_source))
  {
    contract_build_up.Append(contract_with_quotes, contract_source, contract_with_quotes);
    contract_build_up_final = contract_build_up.Copy();
    //contract_build_up.Append(contract_source);
    //contract_build_up.Append(contract_with_quotes);

    //std::cerr << "Hashing --" <<  contract_source.As<std::string> << "--" << std::endl;
    //
    std::cerr << "Hashing1Y XX" <<  contract_build_up_final << "++" << std::endl;
    auto smart_contract_hash = crypto::Hash<crypto::SHA256>(contract_build_up_final);

    std::cerr << "asdfasdf zzxx:"<< contract_with_quotes.size() << std::endl;
    //exit(1);


    FETCH_LOG_WARN(LOGGING_NAME, "Adding smart contract, PK: ", ToHex(smart_contract_hash));

    SetRawState(contract_source, smart_contract_hash);

    FETCH_LOG_WARN(LOGGING_NAME, "Added smart contract, PK: ", ToHex(smart_contract_hash));
  }
  else
  {
    return Status::FAILED;
  }

  return Status::OK;
}

}  // namespace ledger
}  // namespace fetch
