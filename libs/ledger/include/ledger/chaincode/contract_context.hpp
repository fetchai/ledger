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

#include "chain/address.hpp"
#include "chain/transaction_layout.hpp"

namespace fetch {
namespace ledger {

class TokenContract;
class StateAdapter;
class StorageInterface;

struct ContractContext
{
  ContractContext()                        = default;
  ContractContext(ContractContext const &) = default;
  ContractContext(ContractContext &&)      = default;

  ContractContext(TokenContract *token_contract_param, chain::Address address,
                  StorageInterface const *storage_param, StateAdapter *state_adapter_param,
                  chain::TransactionLayout::BlockIndex block_index_param);

  TokenContract *const                       token_contract{nullptr};
  chain::Address const                       contract_address{};
  StorageInterface const *const              storage{nullptr};
  StateAdapter *const                        state_adapter{nullptr};
  chain::TransactionLayout::BlockIndex const block_index{0};
};

}  // namespace ledger
}  // namespace fetch
