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

constexpr char const *GOVERNANCE_VERSION_PROPERTY_NAME   = "version";
constexpr char const *GOVERNANCE_DATA_PROPERTY_NAME      = "data";
constexpr char const *GOVERNANCE_ACCEPT_BY_PROPERTY_NAME = "accept_by";

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

class SubmittedGovernanceProposal
{
public:
  SubmittedGovernanceProposal() = default;
  SubmittedGovernanceProposal(GovernanceProposal          proposal,
                              std::vector<chain::Address> votes_for_param,
                              std::vector<chain::Address> votes_against_param);
  SubmittedGovernanceProposal(SubmittedGovernanceProposal const &) = default;
  SubmittedGovernanceProposal(SubmittedGovernanceProposal &&)      = default;

  SubmittedGovernanceProposal &operator=(SubmittedGovernanceProposal const &) = default;
  SubmittedGovernanceProposal &operator=(SubmittedGovernanceProposal &&) = default;

  static SubmittedGovernanceProposal CreateDefaultProposal();

  GovernanceProposal          proposal;
  std::vector<chain::Address> votes_for;
  std::vector<chain::Address> votes_against;
};

using SubmittedGovernanceProposalQueue = std::vector<SubmittedGovernanceProposal>;

}  // namespace ledger

namespace serializers {

template <typename T, typename D>
struct ArraySerializer;
template <typename T, typename D>
struct MapSerializer;

template <typename D>
struct MapSerializer<ledger::GovernanceProposal, D>
{
public:
  using Type       = ledger::GovernanceProposal;
  using DriverType = D;

  static uint8_t const VERSION   = 1;
  static uint8_t const DATA      = 2;
  static uint8_t const ACCEPT_BY = 3;

  template <typename Constructor>
  static void Serialize(Constructor &constructor, Type const &x)
  {
    auto serializer = constructor(3);

    serializer.Append(VERSION, x.version);
    serializer.Append(DATA, x.data);
    serializer.Append(ACCEPT_BY, x.accept_by);
  }

  template <typename Deserializer>
  static void Deserialize(Deserializer &deserializer, Type &x)
  {
    deserializer.ExpectKeyGetValue(VERSION, x.version);
    deserializer.ExpectKeyGetValue(DATA, x.data);
    deserializer.ExpectKeyGetValue(ACCEPT_BY, x.accept_by);
  }
};
//???file jira ticket - investigate which serializers may be dumped in cpp files
template <typename D>
struct MapSerializer<ledger::SubmittedGovernanceProposal, D>
{
public:
  using Type       = ledger::SubmittedGovernanceProposal;
  using DriverType = D;

  static uint8_t const PROPOSAL      = 1;
  static uint8_t const VOTES_FOR     = 2;
  static uint8_t const VOTES_AGAINST = 3;

  template <typename Constructor>
  static void Serialize(Constructor &constructor, Type const &x)
  {
    auto serializer = constructor(3);

    serializer.Append(PROPOSAL, x.proposal);
    serializer.Append(VOTES_FOR, x.votes_for);
    serializer.Append(VOTES_AGAINST, x.votes_against);
  }

  template <typename Deserializer>
  static void Deserialize(Deserializer &deserializer, Type &x)
  {
    deserializer.ExpectKeyGetValue(PROPOSAL, x.proposal);
    deserializer.ExpectKeyGetValue(VOTES_FOR, x.votes_for);
    deserializer.ExpectKeyGetValue(VOTES_AGAINST, x.votes_against);
  }
};

template <typename D>
struct ArraySerializer<ledger::SubmittedGovernanceProposalQueue, D>
{
public:
  using Type       = ledger::SubmittedGovernanceProposalQueue;
  using DriverType = D;

  template <typename Constructor>
  static void Serialize(Constructor &constructor, Type const &x)
  {
    auto serializer = constructor(x.size());

    for (auto const &proposal : x)
    {
      serializer.Append(proposal);
    }
  }

  template <typename Deserializer>
  static void Deserialize(Deserializer &deserializer, Type &x)
  {
    std::size_t const size = deserializer.size();
    x.resize(size);
    for (std::size_t i = 0; i < size; ++i)
    {
      deserializer.GetNextValue(x[i]);
    }
  }
};

}  // namespace serializers
}  // namespace fetch
