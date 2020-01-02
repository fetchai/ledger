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

#include "chain/transaction.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/consensus/stake_update_event.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace chain {

class Address;
class Transaction;

}  // namespace chain
namespace ledger {

class Deed;

class TokenContract : public Contract
{
public:
  using DeedPtr = std::shared_ptr<Deed>;

  static constexpr char const *LOGGING_NAME = "TokenContract";
  static constexpr char const *NAME         = "fetch.token";

  // Construction / Destruction
  TokenContract();
  ~TokenContract() override = default;

  // library functions
  DeedPtr  GetDeed(chain::Address const &address);
  void     SetDeed(chain::Address const &address, DeedPtr const &deed);
  uint64_t GetBalance(chain::Address const &address);
  bool     AddTokens(chain::Address const &address, uint64_t amount);
  bool     SubtractTokens(chain::Address const &address, uint64_t amount);
  bool     TransferTokens(chain::Transaction const &tx, chain::Address const &to, uint64_t amount);

  // transaction handlers
  Result UpdateDeed(chain::Transaction const &tx);
  Result Transfer(chain::Transaction const &tx);
  Result AddStake(chain::Transaction const &tx);
  Result DeStake(chain::Transaction const &tx);
  Result CollectStake(chain::Transaction const &tx);

  // queries
  Status Balance(Query const &query, Query &response);
  Status QueryDeed(Query const &query, Query &response);
  Status Stake(Query const &query, Query &response);
  Status CooldownStake(Query const &query, Query &response);

  void ExtractStakeUpdates(StakeUpdateEvents &updates);
  void ClearStakeUpdates();

private:
  StakeUpdateEvents stake_updates_;
};

}  // namespace ledger
}  // namespace fetch
