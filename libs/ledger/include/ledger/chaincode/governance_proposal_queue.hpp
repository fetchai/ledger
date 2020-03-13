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
#include "variant/variant.hpp"

#include <cstddef>
#include <vector>

namespace fetch {
namespace ledger {

constexpr char const *GOVERNANCE_CHARGE_MULTIPLIER_PROPERTY_NAME = "charge_multiplier";

class GovernanceProposal
{
public:
  GovernanceProposal() = default;
  explicit GovernanceProposal(variant::Variant const &);
  GovernanceProposal(uint64_t version_param, variant::Variant data_param, uint64_t accept_by_param);

  bool operator==(GovernanceProposal const &) const;

  variant::Variant AsVariant() const;

  uint64_t         version{0};
  variant::Variant data{variant::Variant::Object()};
  uint64_t         accept_by{0};
};

class Ballot
{
public:
  Ballot() = default;
  Ballot(GovernanceProposal proposal, std::vector<chain::Address> votes_for_param,
         std::vector<chain::Address> votes_against_param);
  Ballot(Ballot const &) = default;
  Ballot(Ballot &&)      = default;

  Ballot &operator=(Ballot const &) = default;
  Ballot &operator=(Ballot &&) = default;

  // Create ballot which, when accepted, results in
  // the configuration that ledger had at genesis
  static Ballot CreateDefaultBallot();

  GovernanceProposal          proposal;
  std::vector<chain::Address> votes_for;
  std::vector<chain::Address> votes_against;
};

using BallotQueue = std::vector<Ballot>;

}  // namespace ledger
}  // namespace fetch
