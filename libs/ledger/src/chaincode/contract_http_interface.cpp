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

#include "ledger/chaincode/contract_http_interface.hpp"

namespace fetch {
namespace ledger {

constexpr char const *           ContractHttpInterface::LOGGING_NAME;
byte_array::ConstByteArray const ContractHttpInterface::API_PATH_CONTRACT_PREFIX("/api/contract/");
byte_array::ConstByteArray const ContractHttpInterface::CONTRACT_NAME_SEPARATOR(".");
byte_array::ConstByteArray const ContractHttpInterface::PATH_SEPARATOR("/");

}  // namespace ledger
}  // namespace fetch
