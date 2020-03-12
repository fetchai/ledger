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

#include "contract_test.hpp"

#include "chain/transaction_builder.hpp"
#include "json/document.hpp"
#include "ledger/chaincode/deed.hpp"
#include "ledger/chaincode/governance_contract.hpp"

#include "gmock/gmock.h"

#include <initializer_list>
#include <memory>
#include <sstream>
#include <string>

using fetch::chain::Address;
using fetch::chain::TransactionBuilder;
using testing::_;

namespace fetch {
namespace ledger {

namespace {

class GovernanceContractTests : public ContractTest
{
public:
  GovernanceContractTests()
  {
    for (auto const &entity : cabinet_entities)
    {
      cabinet_addresses.insert(entity.signer.identity());
    }
  }

  void SetUp() override
  {
    contract_      = std::make_unique<GovernanceContract>();
    contract_name_ = std::make_shared<ConstByteArray>(std::string{GovernanceContract::NAME});
  }

  Contract::Result SendPropose(ConstByteArray const &data, bool const set_call_expected)
  {
    auto const &issuing_miner = cabinet_entities.back();

    TransactionBuilder builder{};

    auto const tx = builder.From(issuing_miner.address)
                        .TargetChainCode(*contract_name_, BitVector(FullShards(1)))
                        .Action("propose")
                        .Counter(counter_++)
                        .Data(data.ToBase64())
                        .ValidUntil(block_number_ + 10)
                        .Signer(issuing_miner.signer.identity())
                        .Seal()
                        .Sign(issuing_miner.signer)
                        .Build();

    EXPECT_CALL(*storage_, Get(_));
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
    EXPECT_CALL(*storage_, Set(_, _)).Times(set_call_expected ? 1 : 0);
    EXPECT_CALL(*storage_, Lock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, Unlock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    // adapt the storage engine for this execution
    StateSentinelAdapter storage_adapter{*storage_, tx->chain_code(), shards_};

    // dispatch the transaction to the contract
    auto context = fetch::ledger::ContractContext::Builder{}
                       .SetContractAddress(tx->contract_address())
                       .SetStateAdapter(&storage_adapter)
                       .SetBlockIndex(block_number_++)
                       .SetCabinet(cabinet_addresses)
                       .Build();
    fetch::ledger::ContractContextAttacher raii_attacher(*contract_, std::move(context));

    return contract_->DispatchTransaction(*tx);
  }

  Contract::Result CastVote(ConstByteArray const &data, ConstByteArray const &action,
                            Entity const &issuing_miner, bool const set_call_expected)
  {
    TransactionBuilder builder{};

    auto const tx = builder.From(issuing_miner.address)
                        .TargetChainCode(*contract_name_, BitVector(FullShards(1)))
                        .Action(action)
                        .Counter(counter_++)
                        .Data(data.ToBase64())
                        .ValidUntil(block_number_ + 10)
                        .Signer(issuing_miner.signer.identity())
                        .Seal()
                        .Sign(issuing_miner.signer)
                        .Build();

    EXPECT_CALL(*storage_, Get(_));
    EXPECT_CALL(*storage_, GetOrCreate(_)).Times(0);
    EXPECT_CALL(*storage_, Set(_, _)).Times(set_call_expected ? 1 : 0);
    EXPECT_CALL(*storage_, Lock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, Unlock(_)).Times(testing::AnyNumber());
    EXPECT_CALL(*storage_, AddTransaction(_)).Times(0);
    EXPECT_CALL(*storage_, GetTransaction(_, _)).Times(0);

    // adapt the storage engine for this execution
    StateSentinelAdapter storage_adapter{*storage_, tx->chain_code(), shards_};

    // dispatch the transaction to the contract
    auto context = fetch::ledger::ContractContext::Builder{}
                       .SetContractAddress(tx->contract_address())
                       .SetStateAdapter(&storage_adapter)
                       .SetBlockIndex(block_number_++)
                       .SetCabinet(cabinet_addresses)
                       .Build();
    fetch::ledger::ContractContextAttacher raii_attacher(*contract_, std::move(context));

    return contract_->DispatchTransaction(*tx);
  }

  Contract::Result SendAccept(ConstByteArray const &data, Entity const &issuing_miner,
                              bool const set_call_expected)
  {
    return CastVote(data, "accept", issuing_miner, set_call_expected);
  }

  Contract::Result SendReject(ConstByteArray const &data, Entity const &issuing_miner,
                              bool const set_call_expected)
  {
    return CastVote(data, "reject", issuing_miner, set_call_expected);
  }

  variant::Variant SendGetProposals()
  {
    EXPECT_CALL(*storage_, Get(_));

    variant::Variant response{};
    auto const       status = SendQuery("get_proposals", {}, response);

    EXPECT_EQ(status, Contract::Status::OK);

    EXPECT_TRUE(response.IsObject());

    EXPECT_TRUE(response.Has("active_proposal"));
    EXPECT_TRUE(response.Has("voting_queue"));
    EXPECT_TRUE(response.Has("max_number_of_proposals"));

    EXPECT_TRUE(response["active_proposal"].IsObject());
    EXPECT_TRUE(response["voting_queue"].IsArray());
    EXPECT_TRUE(response["max_number_of_proposals"].IsInteger());

    return response;
  }

  void SendRejectVotes(ConstByteArray const &                       data,
                       std::initializer_list<Entity const *> const &entities)
  {
    for (auto const entity : entities)
    {
      EXPECT_TRUE(SendReject(data, *entity, true).status == Contract::Status::OK);
    }
  }

  void SendAcceptVotes(ConstByteArray const &                       data,
                       std::initializer_list<Entity const *> const &entities)
  {
    for (auto const entity : entities)
    {
      EXPECT_TRUE(SendAccept(data, *entity, true).status == Contract::Status::OK);
    }
  }

  Entities const              cabinet_entities{5};
  Consensus::UnorderedCabinet cabinet_addresses{};
  uint64_t                    counter_{};

  ConstByteArray valid_v0_proposal1{R"(
    {
      "version": 0,
      "accept_by": 1200,
      "data": {
        "charge_multiplier": 17
      }
    }
  )"};
  ConstByteArray valid_v0_proposal2{R"(
    {
      "version": 0,
      "accept_by": 1000,
      "data": {
        "charge_multiplier": 7
      }
    }
  )"};
};

TEST_F(GovernanceContractTests, submit_proposal_with_invalid_payload)
{
  auto const data = ConstByteArray{"invalid JSON"};

  auto const result = SendPropose(data, false);

  ASSERT_EQ(result.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, submit_proposal_with_invalid_data_field)
{
  auto const data = ConstByteArray{R"(
    {
      "version": 0,
      "accept_by": 1000,
      "data": {
        "foo": 1
      }
    }
  )"};

  auto const result = SendPropose(data, false);

  ASSERT_EQ(result.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, submit_proposal_with_invalid_charge_multiplier_type)
{
  auto const data = ConstByteArray{R"(
    {
      "version": 0,
      "accept_by": 1000,
      "data": {
        "charge_multiplier": "charge_multiplier should be an int"
      }
    }
  )"};

  auto const result = SendPropose(data, false);

  ASSERT_EQ(result.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, submit_proposal_with_incorrect_version)
{
  auto const data = ConstByteArray{R"(
    {
      "version": 700000,
      "accept_by": 1000,
      "data": {
        "charge_multiplier": 2
      }
    }
  )"};

  auto const result = SendPropose(data, false);

  ASSERT_EQ(result.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, submit_proposal_with_too_long_voting_period)
{
  auto const data = ConstByteArray{R"(
    {
      "version": 0,
      "accept_by": 100000,
      "data": {
        "charge_multiplier": 2
      }
    }
  )"};

  auto const result = SendPropose(data, false);

  ASSERT_EQ(result.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, submit_valid_proposal_with_queue_empty)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const response = SendGetProposals();

  ASSERT_TRUE(response["voting_queue"].size() == 1);
  ASSERT_TRUE(response["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["accept_by"].As<uint64_t>() == 0);

  ASSERT_TRUE(response["voting_queue"][0]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["voting_queue"][0]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response["voting_queue"][0]["accept_by"].As<uint64_t>() == 1200);
}

TEST_F(GovernanceContractTests, submit_valid_proposal_when_queue_full)
{
  auto const result1 = SendPropose(valid_v0_proposal1, true);
  auto const result2 = SendPropose(valid_v0_proposal2, false);

  ASSERT_EQ(result1.status, Contract::Status::OK);
  ASSERT_EQ(result2.status, Contract::Status::FAILED);

  auto const response = SendGetProposals();

  ASSERT_TRUE(response["voting_queue"].size() == 1);
  ASSERT_TRUE(response["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["accept_by"].As<uint64_t>() == 0);

  ASSERT_TRUE(response["voting_queue"][0]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["voting_queue"][0]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response["voting_queue"][0]["accept_by"].As<uint64_t>() == 1200);
}

TEST_F(GovernanceContractTests, query_proposals_before_any_had_been_submitted)
{
  auto const response = SendGetProposals();

  ASSERT_TRUE(response["voting_queue"].size() == 0);
  ASSERT_TRUE(response["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["accept_by"].As<uint64_t>() == 0);
}

TEST_F(GovernanceContractTests, query_proposals_after_one_had_been_submitted)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const response = SendGetProposals();

  ASSERT_TRUE(response["voting_queue"].size() == 1);
  ASSERT_TRUE(response["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["active_proposal"]["accept_by"].As<uint64_t>() == 0);

  ASSERT_TRUE(response["voting_queue"][0]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response["voting_queue"][0]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response["voting_queue"][0]["accept_by"].As<uint64_t>() == 1200);
}

TEST_F(GovernanceContractTests, submit_then_reject_proposal)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const response1 = SendGetProposals();

  ASSERT_TRUE(response1["voting_queue"].size() == 1);
  ASSERT_TRUE(response1["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response1["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["active_proposal"]["accept_by"].As<uint64_t>() == 0);

  ASSERT_TRUE(response1["voting_queue"][0]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["voting_queue"][0]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response1["voting_queue"][0]["accept_by"].As<uint64_t>() == 1200);

  SendRejectVotes(valid_v0_proposal1,
                  {&cabinet_entities[0], &cabinet_entities[1], &cabinet_entities[2]});

  auto const response2 = SendGetProposals();

  ASSERT_TRUE(response2["voting_queue"].size() == 0);
  ASSERT_TRUE(response2["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response2["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response2["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response2["active_proposal"]["accept_by"].As<uint64_t>() == 0);
}

TEST_F(GovernanceContractTests, submit_then_accept_proposal)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const response1 = SendGetProposals();

  ASSERT_TRUE(response1["voting_queue"].size() == 1);
  ASSERT_TRUE(response1["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response1["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["active_proposal"]["accept_by"].As<uint64_t>() == 0);

  ASSERT_TRUE(response1["voting_queue"][0]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["voting_queue"][0]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response1["voting_queue"][0]["accept_by"].As<uint64_t>() == 1200);

  SendAcceptVotes(valid_v0_proposal1,
                  {&cabinet_entities[0], &cabinet_entities[1], &cabinet_entities[2]});

  auto const response2 = SendGetProposals();

  ASSERT_TRUE(response2["voting_queue"].size() == 0);
  ASSERT_TRUE(response2["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response2["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response2["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response2["active_proposal"]["accept_by"].As<uint64_t>() == 1200);
}

TEST_F(GovernanceContractTests, submit_insufficient_votes_to_accept_or_reject)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const response1 = SendGetProposals();

  ASSERT_TRUE(response1["voting_queue"].size() == 1);
  ASSERT_TRUE(response1["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response1["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["active_proposal"]["accept_by"].As<uint64_t>() == 0);

  ASSERT_TRUE(response1["voting_queue"][0]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response1["voting_queue"][0]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response1["voting_queue"][0]["accept_by"].As<uint64_t>() == 1200);

  SendAcceptVotes(valid_v0_proposal1, {&cabinet_entities[0], &cabinet_entities[1]});
  SendRejectVotes(valid_v0_proposal1, {&cabinet_entities[2], &cabinet_entities[3]});

  auto const response2 = SendGetProposals();

  ASSERT_TRUE(response2["voting_queue"].size() == 1);
  ASSERT_TRUE(response2["max_number_of_proposals"].As<uint64_t>() == 2);

  ASSERT_TRUE(response2["active_proposal"]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response2["active_proposal"]["data"]["charge_multiplier"].As<uint64_t>() == 0);
  ASSERT_TRUE(response2["active_proposal"]["accept_by"].As<uint64_t>() == 0);

  ASSERT_TRUE(response2["voting_queue"][0]["version"].As<uint64_t>() == 0);
  ASSERT_TRUE(response2["voting_queue"][0]["data"]["charge_multiplier"].As<uint64_t>() == 17);
  ASSERT_TRUE(response2["voting_queue"][0]["accept_by"].As<uint64_t>() == 1200);
}

TEST_F(GovernanceContractTests, voting_for_a_proposal_fails_if_it_had_not_been_submitted)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const result1 = SendAccept(valid_v0_proposal2, cabinet_entities[0], false);
  auto const result2 = SendReject(valid_v0_proposal2, cabinet_entities[1], false);

  ASSERT_EQ(result1.status, Contract::Status::FAILED);
  ASSERT_EQ(result2.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, after_a_miner_accepts_a_proposal_they_cannot_vote_further)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const result1 = SendAccept(valid_v0_proposal1, cabinet_entities[0], true);
  auto const result2 = SendAccept(valid_v0_proposal1, cabinet_entities[0], false);
  auto const result3 = SendReject(valid_v0_proposal1, cabinet_entities[0], false);

  ASSERT_EQ(result1.status, Contract::Status::OK);
  ASSERT_EQ(result2.status, Contract::Status::FAILED);
  ASSERT_EQ(result3.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, after_a_miner_rejects_a_proposal_they_cannot_vote_further)
{
  auto const result = SendPropose(valid_v0_proposal1, true);
  ASSERT_EQ(result.status, Contract::Status::OK);

  auto const result1 = SendReject(valid_v0_proposal1, cabinet_entities[0], true);
  auto const result2 = SendReject(valid_v0_proposal1, cabinet_entities[0], false);
  auto const result3 = SendAccept(valid_v0_proposal1, cabinet_entities[0], false);

  ASSERT_EQ(result1.status, Contract::Status::OK);
  ASSERT_EQ(result2.status, Contract::Status::FAILED);
  ASSERT_EQ(result3.status, Contract::Status::FAILED);
}

TEST_F(GovernanceContractTests, duplicate_proposals_are_forbidden)
{
  auto const response = SendGetProposals();

  std::ostringstream ss;
  ss << response["active_proposal"];
  auto const json = ss.str();

  auto const result = SendPropose(json, false);

  ASSERT_EQ(result.status, Contract::Status::FAILED);
}

}  // namespace

}  // namespace ledger
}  // namespace fetch
