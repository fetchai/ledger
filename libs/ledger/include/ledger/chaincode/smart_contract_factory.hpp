#pragma once
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

#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/main_serializer_definition.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/smart_contract_wrapper.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <exception>
#include <memory>
#include <stdexcept>

namespace fetch {
namespace ledger {

template <typename ContractType>
auto CreateSmartContract(chain::Address const &contract_address, StorageInterface const &storage)
    -> std::unique_ptr<ContractType>
{
  auto const addr     = SmartContractManager::CreateAddressForContract(contract_address);
  auto const resource = storage.Get(addr);

  if (!resource.failed)
  {
    serializers::MsgPackSerializer buffer{resource.document};
    SmartContractWrapper           document{};
    buffer >> document;

    return std::make_unique<ContractType>(std::string(document.source));
  }

  FETCH_LOG_ERROR("SmartContractFactory",
                  "Unable to construct requested smart contract: ", addr.address());

  return {};
}

}  // namespace ledger
}  // namespace fetch
