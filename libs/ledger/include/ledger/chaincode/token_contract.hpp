#pragma once
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

#include "chain/transaction.hpp"
#include "ledger/chaincode/contract.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace chain {

class Address;
class Transaction;

}  // namespace chain
namespace ledger {

class TokenContract : public Contract
{
public:
  struct StakeUpdate
  {
    Identity identity;  ///< The identity of the staker
    uint64_t from;      ///< The block index from which the stake becomes active
    uint64_t amount;    ///< The amount being staked
  };

  using StakeUpdates = std::vector<StakeUpdate>;

  static constexpr char const *LOGGING_NAME = "TokenContract";
  static constexpr char const *NAME         = "fetch.token";

  // Construction / Destruction
  TokenContract();
  ~TokenContract() override = default;

  // library functions
  uint64_t GetBalance(chain::Address const &address);
  bool     AddTokens(chain::Address const &address, uint64_t amount);
  bool     SubtractTokens(chain::Address const &address, uint64_t amount);
  bool     TransferTokens(chain::Transaction const &tx, chain::Address const &to, uint64_t amount);

  // transaction handlers
  Result CreateWealth(chain::Transaction const &tx);
  Result Deed(chain::Transaction const &tx);
  Result Transfer(chain::Transaction const &tx);
  Result AddStake(chain::Transaction const &tx);
  Result DeStake(chain::Transaction const &tx);
  Result CollectStake(chain::Transaction const &tx);

  // queries
  Status Balance(Query const &query, Query &response);
  Status Stake(Query const &query, Query &response);
  Status CooldownStake(Query const &query, Query &response);

  void         ClearStakeUpdates();
  StakeUpdates stake_updates() const;

private:
  StakeUpdates stake_updates_;
};

inline void TokenContract::ClearStakeUpdates()
{
  stake_updates_.clear();
}

inline TokenContract::StakeUpdates TokenContract::stake_updates() const
{
  return stake_updates_;
}

}  // namespace ledger
}  // namespace fetch
