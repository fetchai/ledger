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
#include "ledger/consensus/consensus.hpp"

namespace fetch {
namespace ledger {

class TokenContract;
class StateAdapter;
class StorageInterface;

struct ContractContext
{
  class Builder
  {
  public:
    Builder()                = default;
    ~Builder()               = default;
    Builder(Builder const &) = delete;
    Builder(Builder &&)      = delete;
    Builder &operator=(Builder const &) = delete;
    Builder &operator=(Builder &&) = delete;

    Builder &SetTokenContract(TokenContract *);
    Builder &SetContractAddress(chain::Address);
    Builder &SetStorage(StorageInterface const *);
    Builder &SetStateAdapter(StateAdapter *);
    Builder &SetBlockIndex(chain::TransactionLayout::BlockIndex);
    Builder &SetVmChargeMultiplier(uint64_t);
    Builder &SetCabinet(std::unordered_set<crypto::Identity>);

    ContractContext Build() const;

  private:
    class TokenContract *                token_contract_{nullptr};
    chain::Address                       contract_address_{};
    StorageInterface const *             storage_{nullptr};
    class StateAdapter *                 state_adapter_{nullptr};
    chain::TransactionLayout::BlockIndex block_index_{0};
    uint64_t                             vm_charge_multiplier_{0};
    std::unordered_set<crypto::Identity> cabinet_{};
  };

  ContractContext()                        = default;
  ContractContext(ContractContext const &) = default;
  ContractContext(ContractContext &&)      = default;

  TokenContract *const                       token_contract{nullptr};
  chain::Address const                       contract_address{};
  StorageInterface const *const              storage{nullptr};
  StateAdapter *const                        state_adapter{nullptr};
  chain::TransactionLayout::BlockIndex const block_index{0};
  uint64_t const                             vm_charge_multiplier{0};
  std::unordered_set<crypto::Identity> const cabinet{};

private:
  ContractContext(TokenContract *token_contract_param, chain::Address address,
                  StorageInterface const *storage_param, StateAdapter *state_adapter_param,
                  chain::TransactionLayout::BlockIndex block_index_param,
                  uint64_t                             charge_multiplier_param,
                  std::unordered_set<crypto::Identity> cabinet_param);

  friend class Builder;
};

}  // namespace ledger
}  // namespace fetch
