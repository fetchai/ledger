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
namespace serializers {

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

template <typename D>
struct MapSerializer<ledger::Ballot, D>
{
public:
  using Type       = ledger::Ballot;
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
struct ArraySerializer<ledger::BallotQueue, D>
{
public:
  using Type       = ledger::BallotQueue;
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

namespace ledger {

namespace {

constexpr char const *LOGGING_NAME               = "GovernanceContract";
constexpr char const *GOVERNANCE_BALLOTS_ADDRESS = "ballots";

constexpr uint64_t GOVERNANCE_VOTE_CHARGE    = 1;
constexpr uint64_t GOVERNANCE_PROPOSE_CHARGE = 1000;

// Total size of proposal queue, i.e. this must include the currently accepted proposal
constexpr uint64_t MAX_NUMBER_OF_PROPOSALS = 2;

// About a week at 10s block mining interval
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

  return Ballot::CreateDefaultBallot()
      .proposal.data[GOVERNANCE_CHARGE_MULTIPLIER_PROPERTY_NAME]
      .As<uint64_t>();
}

std::unique_ptr<GovernanceProposal> ProposalFromTx(chain::Transaction const &tx) noexcept
{
  try
  {
    auto const &       data      = tx.data();
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

bool GovernanceContract::Save(BallotQueue const &ballots)
{
  auto const status = SetStateRecord(ballots, GOVERNANCE_BALLOTS_ADDRESS);
  if (status != StateAdapter::Status::OK)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to store ballots");

    return false;
  }

  return true;
}

BallotQueue GovernanceContract::Load()
{
  BallotQueue ballots{};

  // The following read will legitimately fail for early blocks,
  // because no governance txs had been issued before. This
  // case is handled below by using a default proposal which
  // corresponds to original ledger defaults
  if (!GetStateRecord(ballots, GOVERNANCE_BALLOTS_ADDRESS))
  {
    return {Ballot::CreateDefaultBallot()};
  }

  assert(!ballots.empty());

  return ballots;
}

ChargeConfiguration GovernanceContract::GetCurrentChargeConfiguration()
{
  auto const ballots = Load();

  auto const current_accepted_proposal = ballots.front().proposal;

  return ChargeConfiguration::Builder{}
      .SetChargeMultiplier(ToChargeMultiplier(current_accepted_proposal))
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

template <typename GetVotesFn>
bool GovernanceContract::IsDecided(GetVotesFn &&get_votes) const
{
  auto const &votes = get_votes();

  auto const votes_by_current_cabinet =
      std::count_if(votes.cbegin(), votes.cend(), [cabinet{context().cabinet}](auto const &vote) {
        return cabinet.cend() !=
               std::find_if(cabinet.cbegin(), cabinet.cend(),
                            [vote](auto const &member) { return chain::Address{member} == vote; });
      });

  return static_cast<std::size_t>(votes_by_current_cabinet) > context().cabinet.size() / 2;
}

bool GovernanceContract::IsRejected(Ballot const &ballot) const
{
  return IsDecided([&ballot]() -> auto const & { return ballot.votes_against; });
}

bool GovernanceContract::IsAccepted(Ballot const &ballot) const
{
  return IsDecided([&ballot]() -> auto const & { return ballot.votes_for; });
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

  auto ballots = Load();

  // Sanity check - limit queue size to max
  if (ballots.size() > MAX_NUMBER_OF_PROPOSALS)
  {
    ballots.erase(ballots.begin() + MAX_NUMBER_OF_PROPOSALS, ballots.end());
  }

  // Queue is full - check for any expired proposals that we can remove to make room
  if (ballots.size() == MAX_NUMBER_OF_PROPOSALS)
  {
    // Add one, as first entry is the active proposal
    ballots.erase(std::remove_if(ballots.begin() + 1, ballots.end(),
                                 [this](auto const &ballot) { return IsExpired(ballot.proposal); }),
                  ballots.end());

    if (ballots.size() == MAX_NUMBER_OF_PROPOSALS)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Queue is full and voting is still ongoing");

      return {Contract::Status::FAILED};
    }
  }

  auto proposal = ProposalFromTx(tx);

  if (!proposal || IsExpired(*proposal) ||
      !AcceptByValid(proposal->accept_by, context().block_index))
  {
    return {Contract::Status::FAILED};
  }

  bool const duplicate_found =
      ballots.cend() != std::find_if(ballots.cbegin(), ballots.cend(),
                                     [proposal{proposal.get()}](auto const &ballot) {
                                       return ballot.proposal == *proposal;
                                     });
  if (duplicate_found)
  {
    return {Contract::Status::FAILED};
  }

  ballots.emplace_back(*proposal, GovernanceVotes{}, GovernanceVotes{});

  return Save(ballots) ? Contract::Result{Contract::Status::OK}
                       : Contract::Result{Contract::Status::FAILED};
}

Contract::Result GovernanceContract::Accept(chain::Transaction const &tx)
{
  return CastVote(
      tx, [](auto const &selected_ballot) -> auto & { return selected_ballot->votes_for; });
}

Contract::Result GovernanceContract::Reject(chain::Transaction const &tx)
{
  return CastVote(
      tx, [](auto const &selected_ballot) -> auto & { return selected_ballot->votes_against; });
}

template <typename GetVotesFn>
Contract::Result GovernanceContract::CastVote(chain::Transaction const &tx, GetVotesFn &&get_votes)
{
  charge_ += GOVERNANCE_VOTE_CHARGE;

  if (!GovernanceTxPreCheck(tx))
  {
    return {Contract::Status::FAILED};
  }

  auto ballots = Load();

  // Do not consider the first position in queue, as that
  // implicitly contains the currently active proposal
  auto selected_ballot = std::find_if(ballots.begin() + 1, ballots.end(), [tx](auto const &x) {
    auto const prop = ProposalFromTx(tx);
    if (prop)
    {
      return x.proposal == *prop;
    }

    return false;
  });
  if (selected_ballot == ballots.end())
  {
    return {Contract::Status::FAILED};
  }

  auto const &votes_for              = selected_ballot->votes_for;
  auto const &votes_against          = selected_ballot->votes_against;
  auto const &cabinet_member_address = tx.from();

  // Prevent double-voting
  auto const vote_for = std::find(votes_for.begin(), votes_for.end(), cabinet_member_address);
  auto const vote_against =
      std::find(votes_against.begin(), votes_against.end(), cabinet_member_address);
  if (vote_for != votes_for.end() || vote_against != votes_against.end())
  {
    return {Contract::Status::FAILED};
  }

  // Cast vote
  get_votes(selected_ballot).push_back(cabinet_member_address);

  if (IsAccepted(*selected_ballot) || IsRejected(*selected_ballot))
  {
    // Sanity check before accepting proposal
    if (!IsRejected(*selected_ballot) && !IsExpired(selected_ballot->proposal))
    {
      // Move vote to front of queue - it will come into force once
      // written to the state DB, starting from the next block
      std::swap(*ballots.begin(), *selected_ballot);
    }

    // This removes the current proposal (if it's been rejected) or the previously
    // accepted proposal (if it's been replaced with the current one)
    ballots.erase(selected_ballot);
  }

  return Save(ballots) ? Contract::Result{Contract::Status::OK}
                       : Contract::Result{Contract::Status::FAILED};
}

Contract::Status GovernanceContract::GetProposals(Query const & /*query*/, Query &response)
{
  auto ballots = Load();

  assert(!ballots.empty());

  response                            = Query::Object();
  response["max_number_of_proposals"] = MAX_NUMBER_OF_PROPOSALS;
  response["active_proposal"]         = ballots[0].proposal.AsVariant();

  response["voting_queue"] = Query::Array(ballots.size() - 1);
  for (unsigned i = 1; i < ballots.size(); ++i)
  {
    response["voting_queue"][i - 1] = ballots[i].proposal.AsVariant();
  }

  return Contract::Status::OK;
}

}  // namespace ledger
}  // namespace fetch
