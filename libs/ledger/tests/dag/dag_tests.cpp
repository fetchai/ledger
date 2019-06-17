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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "ledger/dag/dag.hpp"
#include "ledger/dag/dag_interface.hpp"

#include "crypto/ecdsa.hpp"
#include "crypto/hash.hpp"
#include "crypto/prover.hpp"
#include "crypto/sha256.hpp"

#include <algorithm>

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::StrictMock;

using namespace fetch;

static constexpr char const *LOGGING_NAME = "DagTests";

using DAGChild     = ledger::DAG;
using DAGInterface = ledger::DAGInterface;
using DAG          = std::shared_ptr<DAGInterface>;
using EpochHistory = std::vector<std::set<std::string>>;
using Epochs       = std::vector<ledger::DAGEpoch>;

using Prover         = fetch::crypto::Prover;
using ProverPtr      = std::shared_ptr<Prover>;
using Certificate    = fetch::crypto::Prover;
using CertificatePtr = std::shared_ptr<Certificate>;

class DagTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    FETCH_UNUSED(LOGGING_NAME);
    dag_ = MakeDAG("dag_test_file", false);
  }

  DAG MakeDAG(std::string const &id, bool load_from_file)
  {
    auto certificate = CreateNewCertificate();
    DAG  dag;
    dag.reset(new DAGChild(id, load_from_file, certificate));
    return dag;
  }

  void TearDown() override
  {}

  // Verify that the nodes in the latest dag epoch match the sanity check epoch_history_
  void VerifyEpochNodes(uint64_t index)
  {
    auto latest_nodes = dag_->GetLatest(true);

    auto &epoch_history_relevant = epoch_history_[index];

    if (index != 0)
    {
      ASSERT_EQ(epoch_history_relevant.empty(), false);
    }
    ASSERT_EQ(latest_nodes.size(), epoch_history_relevant.size());

    for (auto const &node : latest_nodes)
    {
      auto found =
          epoch_history_relevant.find(std::string(node.contents)) != epoch_history_relevant.end();

      ASSERT_EQ(found, true);
    }
  }

  void PopulateDAG()
  {
    EXPECT_EQ(dag_->CurrentEpoch(), 0);

    const size_t epochs_to_create = 10;
    const size_t nodes_in_epoch   = 1000;

    epoch_history_.resize(epochs_to_create);
    epochs_.resize(epochs_to_create);

    // N - 1 epochs (create epoch 0 corner case)
    for (std::size_t epoch_index = 1; epoch_index < epochs_to_create; ++epoch_index)
    {
      // 1000 nodes in each epoch
      for (std::size_t dnode_index = 0; dnode_index < nodes_in_epoch; ++dnode_index)
      {
        std::string dag_contents(std::to_string(epoch_index) + ":" + std::to_string(dnode_index));
        epoch_history_[epoch_index].insert(dag_contents);
        dag_->AddArbitrary(dag_contents);

        if (dnode_index == nodes_in_epoch - 1)
        {
          auto epoch = dag_->CreateEpoch(epoch_index);
          EXPECT_EQ(epoch.block_number, epoch_index);
          EXPECT_EQ(epoch.all_nodes.size(), nodes_in_epoch);

          epochs_[epoch_index] = epoch;

          auto result = dag_->CommitEpoch(epoch);
          EXPECT_EQ(result, true);

          VerifyEpochNodes(epoch_index);
        }
      }
    }
  }

  DAG          dag_;
  EpochHistory epoch_history_;
  Epochs       epochs_;

private:
  ProverPtr CreateNewCertificate()
  {
    using Signer    = fetch::crypto::ECDSASigner;
    using SignerPtr = std::shared_ptr<Signer>;

    SignerPtr certificate = std::make_shared<Signer>();

    certificate->GenerateKeys();

    return certificate;
  }
};

// Check that the dag can consistently add nodes locally and advance the epochs
TEST_F(DagTests, CheckBasicDagFunctionality)
{
  // This function has assertions
  PopulateDAG();
}

// Check the basic functionality, plus that the dag can revert
TEST_F(DagTests, CheckDagRevertsCorrectly)
{
  PopulateDAG();

  while (epoch_history_.size() > 0)
  {
    const auto epochs_head = epoch_history_.size() - 1;
    VerifyEpochNodes(epochs_head);

    if (epochs_head != 0)
    {
      EXPECT_EQ(dag_->CurrentEpoch(), epochs_head);
      dag_->RevertToEpoch(epochs_head - 1);
    }

    epoch_history_.pop_back();
  }
}

// Check that an epoch that does not contain all of the nodes doesn't invalidate
// nodes that have not yet been epoched (epoch tips don't contain all dag nodes)
TEST_F(DagTests, CheckDagMaintainsTipsCorrectly)
{
  // The easiest way to create a partitioned DAG is to:
  // -Create two DAGs
  // -Push items A to DAG 1 & DAG 2
  // -Push items B to DAG 1
  // -Create epoch 1 on DAG 2 and synchronise (contains items A)
  // -Create epoch 2 on DAG 1 and synchronise (contains items B)

  const size_t nodes_to_push = 1000;

  std::vector<std::string> items_A;
  std::vector<std::string> items_B;

  DAG dag_2 = MakeDAG("dag2", false);

  for (std::size_t dnode_index = 0; dnode_index < nodes_to_push; ++dnode_index)
  {
    std::string dag_contents_A("A:" + std::to_string(dnode_index));
    std::string dag_contents_B("B:" + std::to_string(dnode_index));

    items_A.push_back(dag_contents_A);
    items_B.push_back(dag_contents_B);
  }

  // Push items A, then add those dag nodes
  for (auto const &item : items_A)
  {
    dag_->AddArbitrary(item);
  }

  for (auto const &newly_minted_dnode : dag_->GetRecentlyAdded())
  {
    dag_2->AddDAGNode(newly_minted_dnode);
  }

  // Push items B
  for (auto const &item : items_B)
  {
    dag_->AddArbitrary(item);
  }

  // Create, commit epoch 1 to both
  auto epoch_1 = dag_2->CreateEpoch(1);
  ASSERT_EQ(dag_->SatisfyEpoch(epoch_1), true);
  ASSERT_EQ(dag_2->SatisfyEpoch(epoch_1), true);
  ASSERT_EQ(dag_->CommitEpoch(epoch_1), true);
  ASSERT_EQ(dag_2->CommitEpoch(epoch_1), true);

  // Create epoch 2 from dag 1, this contains nodes dag 2 doesn't have
  auto epoch_2 = dag_->CreateEpoch(2);
  ASSERT_EQ(dag_->SatisfyEpoch(epoch_2), true);
  ASSERT_EQ(dag_2->SatisfyEpoch(epoch_2), false);  // can't satisfy!

  // Provide dag 2 the missing nodes
  auto recently_added = dag_->GetRecentlyAdded();

  std::random_shuffle(recently_added.begin(), recently_added.end());

  for (auto const &newly_minted_dnode : recently_added)
  {
    dag_2->AddDAGNode(newly_minted_dnode);
  }

  ASSERT_EQ(dag_2->SatisfyEpoch(epoch_2), true);  // can now satisfy

  ASSERT_EQ(dag_->CommitEpoch(epoch_2), true);
  ASSERT_EQ(dag_2->CommitEpoch(epoch_2), true);

  ASSERT_EQ(epoch_1.all_nodes.size(), nodes_to_push);
  ASSERT_EQ(epoch_2.all_nodes.size(), nodes_to_push);
}

// Check has epoch functionality
TEST_F(DagTests, CheckBasicDagFunctionality___CheckHasEpochWorks)
{
  PopulateDAG();

  // Corner case for epoch 0
  for (std::size_t i = 1; i < epochs_.size(); ++i)
  {
    ASSERT_EQ(dag_->HasEpoch(epochs_[i].hash), true);
  }
}

// Check that the dag can recover from file
TEST_F(DagTests, CheckDagFileRecovery)
{
  PopulateDAG();

  dag_.reset();

  dag_ = MakeDAG("dag_test_file", true);

  auto current_dag = dag_->CurrentEpoch();

  ASSERT_NE(current_dag, 0);

  epoch_history_.resize(current_dag + 1);

  VerifyEpochNodes(current_dag);
}

TEST_F(DagTests, CheckDagGetNode)
{
  dag_->AddArbitrary("one_dag_node");

  auto dnodes = dag_->GetRecentlyAdded();

  EXPECT_EQ(dnodes.size(), 1);

  ledger::DAGNode dummy;
  EXPECT_EQ(dag_->GetDAGNode(dnodes.back().hash, dummy), true);
  EXPECT_EQ(dag_->GetDAGNode(crypto::Hash<crypto::SHA256>("not here"), dummy), false);
}
