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

#include "ledger/chaincode/smart_contract.hpp"
#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chaincode/vm_definition.hpp"
#include "variant/variant.hpp"

#include <stdexcept>

namespace fetch {
namespace ledger {

byte_array::ConstByteArray const SMART_CONTRACT{"smart_contract"};
byte_array::ConstByteArray const ADDRESS{"address"};

SmartContract::SmartContract()
  : Contract("fetch.smart_contract")
{
  OnTransaction("create_initial_contract", this, &SmartContract::CreateInitialContract);
  OnTransaction("invoke_contract", this, &SmartContract::Invoke);
  OnTransaction("delete_contract", this, &SmartContract::DeleteContract);
}

Contract::Status SmartContract::CreateInitialContract(Transaction const &tx)
{
//  variant::Variant data;
//
//  if (!ParseAsJson(tx, data))
//  {
//    return Status::FAILED;
//  }
//
//  ConstByteArray smart_contract; // Smart contract as text
//  ConstByteArray address;        // Location to put the smart contract
//
//  if (!Extract(data, SMART_CONTRACT, smart_contract) && Extract(data, ADDRESS, address))
//  {
//    return Status::FAILED;
//  }
//
//  auto identity = *(tx.signatures().begin()).first;
//
//  if (!GetOrCreateStateRecord(smart_contract, tx.))
//  {
//  }

  /*
  address = FromBase64(address);  //  the address needs to be converted

  // retrieve the record (if it exists)
  WalletRecord record{};
  GetOrCreateStateRecord(record, address);

  // update the balance
  record.balance += amount;

  SetStateRecord(record, address);
  */

  return Status::OK;

}

Contract::Status SmartContract::Invoke(Transaction const &tx)
{
  return Status::OK;
}

Contract::Status SmartContract::DeleteContract(Transaction const &tx)
{
  return Status::OK;
}

}  // namespace ledger
}  // namespace fetch
