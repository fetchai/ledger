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

#include "ledger/chaincode/charge_configuration.hpp"
#include "ledger/chaincode/contract.hpp"

#include <cstdint>

namespace fetch {
namespace ledger {

class GovernanceProposal;
class Ballot;

class GovernanceContract : public Contract
{
public:
  using GovernanceVotes = std::vector<chain::Address>;

  static constexpr char const *NAME = "fetch.governance";

  GovernanceContract();
  ~GovernanceContract() override = default;

  ChargeConfiguration GetCurrentChargeConfiguration();

  Contract::Result Propose(chain::Transaction const &tx);
  Contract::Result Accept(chain::Transaction const &tx);
  Contract::Result Reject(chain::Transaction const &tx);

  Contract::Status GetProposals(Query const &query, Query &response);

private:
  uint64_t CalculateFee() const override;

  using BallotQueue = std::vector<Ballot>;

  template <typename GetVotesFn>
  Contract::Result CastVote(chain::Transaction const &, GetVotesFn &&);
  template <typename GetVotesFn>
  bool IsDecided(GetVotesFn &&) const;

  bool GovernanceTxPreCheck(chain::Transaction const &tx) const;

  bool SignedAndIssuedBySameCabinetMember(chain::Transaction const &tx) const;
  bool IsExpired(GovernanceProposal const &proposal) const;
  bool IsRejected(Ballot const &ballot) const;
  bool IsAccepted(Ballot const &ballot) const;

  BallotQueue Load();
  bool        Save(BallotQueue const &ballots);

  uint64_t charge_{0};
};

}  // namespace ledger
}  // namespace fetch
