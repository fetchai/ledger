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

#include "core/digest.hpp"
#include "core/serializers/main_serializer.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/upow/synergetic_contract_factory.hpp"

namespace fetch {
namespace ledger {

constexpr char const *LOGGING_NAME = "SynContractFactory";

using byte_array::ConstByteArray;
using serializers::MsgPackSerializer;

SynergeticContractFactory::SynergeticContractFactory(StorageInterface &storage)
  : storage_{storage}
{}

SynergeticContractPtr SynergeticContractFactory::Create(Digest const &digest)
{
  SynergeticContractPtr contract{};

  // attempt to retrieve the document stored in the database
  auto const resource =
      storage_.Get(SmartContractManager::CreateAddressForContract(digest.ToHex()));

  if (!resource.failed)
  {
    try
    {
      // create and decode the document buffer
      MsgPackSerializer buffer{resource.document};

      // parse the contents of the document
      ConstByteArray document{};
      buffer >> document;

      // create the instance of the synergetic contract
      contract = std::make_shared<SynergeticContract>(document);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Error creating contract: ", ex.what());
    }
  }

  return contract;
}

}  // namespace ledger
}  // namespace fetch
