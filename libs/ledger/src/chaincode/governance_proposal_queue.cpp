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

#include "ledger/chaincode/governance_proposal_queue.hpp"
#include "variant/variant_utils.hpp"

namespace fetch {
namespace ledger {

namespace {

template <uint64_t VERSION>
bool ValidateDataForVersion(variant::Variant const &data);

template <>
bool ValidateDataForVersion<0>(variant::Variant const &data)
{
  return data.Has(GOVERNANCE_CHARGE_MULTIPLIER_PROPERTY_NAME) &&
         data[GOVERNANCE_CHARGE_MULTIPLIER_PROPERTY_NAME].IsInteger();
}

void validate_data(variant::Variant const &data, uint64_t const version)
{
  if (data.IsObject())
  {
    switch (version)
    {
    case 0:
      if (ValidateDataForVersion<0>(data))
      {
        return;
      }
    default:
      break;
    }
  }

  throw std::runtime_error("Proposal data failed validation");
}

}  // namespace

GovernanceProposal::GovernanceProposal(uint64_t version_param, variant::Variant data_param,
                                       uint64_t accept_by_param)
  : version{version_param}
  , data{std::move(data_param)}
  , accept_by{accept_by_param}
{
  validate_data(data, version);
}

GovernanceProposal::GovernanceProposal(variant::Variant const &v)
{
  if (variant::Extract(v, GOVERNANCE_VERSION_PROPERTY_NAME, version) &&
      variant::Extract(v, GOVERNANCE_ACCEPT_BY_PROPERTY_NAME, accept_by) &&
      v.Has(GOVERNANCE_DATA_PROPERTY_NAME))
  {
    data = v[GOVERNANCE_DATA_PROPERTY_NAME];
    validate_data(data, version);

    return;
  }

  throw std::runtime_error("Invalid proposal format");
}

SubmittedGovernanceProposal::SubmittedGovernanceProposal(
    GovernanceProposal proposal_param, std::vector<chain::Address> votes_for_param,
    std::vector<chain::Address> votes_against_param)
  : proposal{std::move(proposal_param)}
  , votes_for{std::move(votes_for_param)}
  , votes_against{std::move(votes_against_param)}
{
  validate_data(proposal.data, proposal.version);
}

SubmittedGovernanceProposal SubmittedGovernanceProposal::CreateDefaultProposal()
{
  auto data = variant::Variant::Object();

  data[GOVERNANCE_CHARGE_MULTIPLIER_PROPERTY_NAME] = 0;

  return SubmittedGovernanceProposal(GovernanceProposal{0u, std::move(data), 0u},
                                     std::vector<chain::Address>{}, std::vector<chain::Address>{});
}

bool GovernanceProposal::operator==(GovernanceProposal const &x) const
{
  return version == x.version && data == x.data && accept_by == x.accept_by;
}

variant::Variant GovernanceProposal::AsVariant() const
{
  auto obj = variant::Variant::Object();

  obj[GOVERNANCE_VERSION_PROPERTY_NAME]   = version;
  obj[GOVERNANCE_DATA_PROPERTY_NAME]      = data;
  obj[GOVERNANCE_ACCEPT_BY_PROPERTY_NAME] = accept_by;

  return obj;
}

}  // namespace ledger
}  // namespace fetch
