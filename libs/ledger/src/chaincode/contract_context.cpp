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

#include <utility>

namespace fetch {
namespace ledger {

ContractContext::ContractContext(TokenContract *const token_contract_param, chain::Address address,
                                 StorageInterface const *const              storage_param,
                                 StateAdapter *const                        state_adapter_param,
                                 chain::TransactionLayout::BlockIndex const block_index_param,
                                 ChargeConfiguration                        charge_config_param,
                                 std::unordered_set<crypto::Identity>       cabinet_param)
  : token_contract{token_contract_param}
  , contract_address{std::move(address)}
  , storage{storage_param}
  , state_adapter{state_adapter_param}
  , block_index{block_index_param}
  , charge_config{std::move(charge_config_param)}
  , cabinet{std::move(cabinet_param)}
{}

ContractContext::Builder &ContractContext::Builder::SetTokenContract(TokenContract *const tc)
{
  token_contract_ = tc;

  return *this;
}

ContractContext::Builder &ContractContext::Builder::SetContractAddress(chain::Address ca)
{
  contract_address_ = std::move(ca);

  return *this;
}

ContractContext::Builder &ContractContext::Builder::SetStorage(StorageInterface const *const s)
{
  storage_ = s;

  return *this;
}

ContractContext::Builder &ContractContext::Builder::SetStateAdapter(StateAdapter *const sa)
{
  state_adapter_ = sa;

  return *this;
}

ContractContext::Builder &ContractContext::Builder::SetBlockIndex(
    chain::TransactionLayout::BlockIndex const bi)
{
  block_index_ = bi;

  return *this;
}

ContractContext::Builder &ContractContext::Builder::SetChargeConfig(ChargeConfiguration config)
{
  charge_config_ = std::move(config);

  return *this;
}

ContractContext::Builder &ContractContext::Builder::SetCabinet(
    std::unordered_set<crypto::Identity> c)
{
  cabinet_ = std::move(c);

  return *this;
}

ContractContext ContractContext::Builder::Build() const
{
  return ContractContext{token_contract_, contract_address_, storage_, state_adapter_,
                         block_index_,    charge_config_,    cabinet_};
}

}  // namespace ledger
}  // namespace fetch
