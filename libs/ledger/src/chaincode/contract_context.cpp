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

#include "chain/address.hpp"
#include "ledger/chaincode/contract_context.hpp"

namespace fetch {
namespace ledger {

ContractContext::ContractContext(TokenContract *token_contract_param, chain::Address address,
                                 StorageInterface const *                   storage_param,
                                 StateAdapter *                             state_adapter_param,
                                 chain::TransactionLayout::BlockIndex const block_index_param)
  : token_contract{token_contract_param}
  , contract_address{std::move(address)}
  , storage{storage_param}
  , state_adapter{state_adapter_param}
  , block_index{block_index_param}
{}

}  // namespace ledger
}  // namespace fetch
