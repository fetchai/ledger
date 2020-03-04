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
#include "json/document.hpp"
#include "ledger/chaincode/governance_contract.hpp"
#include "ledger/chaincode/governance_proposal_queue.hpp"
#include "variant/variant.hpp"

namespace fetch {
namespace ledger {

namespace {

constexpr char const *LOGGING_NAME                 = "GovernanceContract";
constexpr char const *GOVERNANCE_PROPOSALS_ADDRESS = "proposals";

constexpr uint64_t GOVERNANCE_VOTE_CHARGE    = 1;
constexpr uint64_t GOVERNANCE_PROPOSE_CHARGE = 1000;

// Total size of proposal queue, i.e. this must include the currently accepted proposal
constexpr uint64_t MAX_NUMBER_OF_PROPOSALS = 2;

// About a week at 10s mining interval
constexpr uint64_t MAX_VOTING_PERIOD_DURATION = 60000;

uint64_t ToChargeMultiplier(GovernanceProposal const &proposal)
{
  if (proposal.version == 0u)
  {
    auto const &data = proposal.data;
    if (data.IsObject() && data.Has(GOVERNANCE_CHARGE_MULTIPLIER_PROPERTY_NAME))
    {
      return data[GOVERNANCE_CHARGE_MULTIPLIER_PROPERTY_NAME].As<uint64_t>();
    }
  }

  return 0;
}

std::unique_ptr<GovernanceProposal> ProposalFromTx(chain::Transaction const &tx) noexcept
{
  try
  {
    decltype(auto)     data      = tx.data();
    auto const         json_text = data.FromBase64();
    json::JSONDocument doc{json_text};

    auto proposal = std::make_unique<GovernanceProposal>(doc.root());

    return proposal;
  }
  catch (std::exception const &)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Submitted proposal is invalid");
  }

  return nullptr;
}

bool AcceptByValid(uint64_t const accept_by, uint64_t const current_block)
{
  return accept_by <= current_block + MAX_VOTING_PERIOD_DURATION;
}

}  // namespace

GovernanceContract::GovernanceContract()
{
  OnTransaction("propose", this, &GovernanceContract::Propose);
  OnTransaction("accept", this, &GovernanceContract::Accept);
  OnTransaction("reject", this, &GovernanceContract::Reject);

  OnQuery("get_proposals", this, &GovernanceContract::GetProposals);
}

uint64_t GovernanceContract::CalculateFee() const
{
  return charge_;
}

bool GovernanceContract::Save(SubmittedGovernanceProposalQueue const &proposals)
{
  auto const status = SetStateRecord(proposals, GOVERNANCE_PROPOSALS_ADDRESS);
  if (status != StateAdapter::Status::OK)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to store proposals");

    return false;
  }

  return true;
}

SubmittedGovernanceProposalQueue GovernanceContract::Load()
{
  SubmittedGovernanceProposalQueue proposals{};

  // The following read will legitimately fail for early blocks,
  // because a governance proposal had not been issued before. This
  // case is handled below by using a default proposal which corresponds
  // to original ledger defaults
  if (!GetStateRecord(proposals, GOVERNANCE_PROPOSALS_ADDRESS))
  {
    return {SubmittedGovernanceProposal::CreateDefaultProposal()};
  }

  return proposals;
}

ChargeConfiguration GovernanceContract::GetCurrentChargeConfiguration()
{
  SubmittedGovernanceProposalQueue proposals = Load();

  auto const current_accepted_proposal = proposals.front();

  return ChargeConfiguration::Builder{}
      .SetVmChargeMultiplier(ToChargeMultiplier(current_accepted_proposal.proposal))
      .Build();
}

bool GovernanceContract::SignedAndIssuedBySameCabinetMember(chain::Transaction const &tx) const
{
  if (!tx.IsSignedByFromAddress())
  {
    return false;
  }

  if (context().cabinet.empty())
  {
    return false;
  }

  for (auto const &member : context().cabinet)
  {
    if (tx.IsSignedBy(member))
    {
      return true;
    }
  }

  return false;
}

bool GovernanceContract::IsRejected(SubmittedGovernanceProposal const &proposal) const
{
  auto const cabinet_size = context().cabinet.size();

  return proposal.votes_against.size() > cabinet_size / 2;
}

bool GovernanceContract::IsAccepted(SubmittedGovernanceProposal const &proposal) const
{
  auto const cabinet_size = context().cabinet.size();

  return proposal.votes_for.size() > cabinet_size / 2;
}

bool GovernanceContract::IsExpired(GovernanceProposal const &proposal) const
{
  return proposal.accept_by < context().block_index;
}

bool GovernanceContract::GovernanceTxPreCheck(chain::Transaction const &tx) const
{
  if (tx.signatories().size() != 1 && tx.IsSignedByFromAddress())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Multisig proposals not supported");

    return false;
  }

  if (!SignedAndIssuedBySameCabinetMember(tx))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Expected transaction by cabinet member");

    return false;
  }

  return true;
}

Contract::Result GovernanceContract::Propose(chain::Transaction const &tx)
{
  charge_ += GOVERNANCE_PROPOSE_CHARGE;

  if (!GovernanceTxPreCheck(tx))
  {
    return {Contract::Status::FAILED};
  }

  auto proposals = Load();

  // For sanity - limit queue size to max
  if (proposals.size() > MAX_NUMBER_OF_PROPOSALS)
  {
    proposals.erase(proposals.begin() + MAX_NUMBER_OF_PROPOSALS, proposals.end());
  }

  // Queue is full - check for any expired proposal that we can remove to make room
  if (proposals.size() == MAX_NUMBER_OF_PROPOSALS)
  {
    // Add one, as first entry is the active proposal
    auto expired_proposal =
        std::find_if(proposals.begin() + 1, proposals.end(),
                     [this](auto const &proposal) { return IsExpired(proposal.proposal); });
    if (expired_proposal == proposals.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Queue is full and voting is still ongoing");

      return {Contract::Status::FAILED};
    }

    proposals.erase(expired_proposal);
  }

  if (proposals.size() < MAX_NUMBER_OF_PROPOSALS)
  {
    auto proposal = ProposalFromTx(tx);
    if (!proposal || IsExpired(*proposal) ||
        !AcceptByValid(proposal->accept_by, context().block_index))
    {
      return {Contract::Status::FAILED};
    }

    proposals.emplace_back(*proposal, GovernanceVotes{}, GovernanceVotes{});
  }

  return Save(proposals) ? Contract::Result{Contract::Status::OK}
                         : Contract::Result{Contract::Status::FAILED};
}

Contract::Result GovernanceContract::Accept(chain::Transaction const &tx)
{
  return CastVote(
      tx, [](auto const &selected_proposal) -> auto & { return selected_proposal->votes_for; });
}

Contract::Result GovernanceContract::Reject(chain::Transaction const &tx)
{
  return CastVote(
      tx, [](auto const &selected_proposal) -> auto & { return selected_proposal->votes_against; });
}

Contract::Result GovernanceContract::CastVote(chain::Transaction const &tx,
                                              VotesFromQueueIterFn      get_votes)
{
  charge_ += GOVERNANCE_VOTE_CHARGE;

  if (!GovernanceTxPreCheck(tx))
  {
    return {Contract::Status::FAILED};
  }

  auto proposals = Load();

  // Do not consider the first position in queue, as that
  // implicitly contains the currently active proposal
  auto selected_proposal = find_if(proposals.begin() + 1, proposals.end(), [tx](auto const &x) {
    auto const prop = ProposalFromTx(tx);
    if (prop)
    {
      return x.proposal == *prop;
    }

    return false;
  });
  if (selected_proposal == proposals.end())
  {
    return {Contract::Status::FAILED};
  }

  auto &         votes_for              = selected_proposal->votes_for;
  auto &         votes_against          = selected_proposal->votes_against;
  decltype(auto) cabinet_member_address = tx.from();

  // Prevent double-voting
  auto const vote_for = find(votes_for.begin(), votes_for.end(), cabinet_member_address);
  auto const vote_against =
      find(votes_against.begin(), votes_against.end(), cabinet_member_address);
  if (vote_for != votes_for.end() || vote_against != votes_against.end())
  {
    return {Contract::Status::FAILED};
  }

  get_votes(selected_proposal).push_back(cabinet_member_address);

  if (IsAccepted(*selected_proposal) && !IsRejected(*selected_proposal) &&
      !IsExpired(selected_proposal->proposal))
  {
    using std::swap;

    // Copy accepted vote to front of queue - it will come into force
    // once written to the state DB
    swap(*proposals.begin(), *selected_proposal);
  }
  if (IsAccepted(*selected_proposal) || IsRejected(*selected_proposal) ||
      IsExpired(selected_proposal->proposal))
  {
    proposals.erase(selected_proposal);
  }

  return Save(proposals) ? Contract::Result{Contract::Status::OK}
                         : Contract::Result{Contract::Status::FAILED};
}

Contract::Status GovernanceContract::GetProposals(Query const & /*query*/, Query &response)
{
  auto proposals = Load();

  response                            = Query::Object();
  response["max_number_of_proposals"] = MAX_NUMBER_OF_PROPOSALS;
  response["active_proposal"]         = proposals[0].proposal.AsVariant();
  response["voting_queue"]            = Query::Array(proposals.size() - 1);

  for (unsigned i = 1; i < proposals.size(); ++i)
  {
    response["voting_queue"][i - 1] = proposals[i].proposal.AsVariant();
  }

  return Contract::Status::OK;
}

ChargeConfiguration::Builder &ChargeConfiguration::Builder::SetVmChargeMultiplier(
    uint64_t multiplier)
{
  vm_charge_multiplier_ = multiplier;

  return *this;
}

ChargeConfiguration ChargeConfiguration::Builder::Build() const
{
  return ChargeConfiguration{vm_charge_multiplier_};
}

ChargeConfiguration::ChargeConfiguration(uint64_t multiplier)
  : charge_multiplier{multiplier}
{}

}  // namespace ledger
}  // namespace fetch
