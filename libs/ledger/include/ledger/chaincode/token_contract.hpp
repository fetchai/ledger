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

#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/contract.hpp"

#include <vector>

namespace fetch {
namespace ledger {

class Address;
class Transaction;

class TokenContract : public Contract
{
public:
  struct StakeUpdate
  {
    Address  address;  ///< The address of the staker
    uint64_t from;     ///< The block index from which the stake becomes active
    uint64_t amount;   ///< The amount being staked
  };

  using StakeUpdates = std::vector<StakeUpdate>;

  static constexpr char const *LOGGING_NAME = "TokenContract";
  static constexpr char const *NAME         = "fetch.token";

  // Construction / Destruction
  TokenContract();
  ~TokenContract() override = default;

  // library functions
  uint64_t GetBalance(Address const &address);
  bool     AddTokens(Address const &address, uint64_t amount);
  bool     SubtractTokens(Address const &address, uint64_t amount);
  bool     TransferTokens(Transaction const &tx, Address const &to, uint64_t amount);

  // transaction handlers
  Status CreateWealth(Transaction const &tx, BlockIndex);
  Status Deed(Transaction const &tx, BlockIndex);
  Status Transfer(Transaction const &tx, BlockIndex);
  Status AddStake(Transaction const &tx, BlockIndex);

  // queries
  Status Balance(Query const &query, Query &response);
  Status Stake(Query const &query, Query &response);

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
