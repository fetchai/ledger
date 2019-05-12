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

#include "ledger/chain/block.hpp"
#include "ledger/dag/dag.hpp"
#include "core/random/lfg.hpp"
#include "math/bignumber.hpp"


#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <fstream>

class DAGTest : public ::testing::Test
{
public:
  using DAG                   = fetch::ledger::DAG;
  using Block                 = fetch::ledger::Block;
  using FakeChain             = std::vector< Block >;

  using UniqueDAG             = std::unique_ptr< DAG >;
  using UniqueBlock           = std::unique_ptr< Block > ;  
  using UniqueBlockChain      = std::unique_ptr< FakeChain > ;

  using Address               = fetch::byte_array::ConstByteArray;
  using RandomGenerator       = fetch::random::LaggedFibonacciGenerator<>;
  using UniqueCertificate     = std::unique_ptr<fetch::crypto::ECDSASigner>;

  using ConstByteArray        = fetch::byte_array::ConstByteArray;
  using ContractName          = fetch::byte_array::ConstByteArray;
  using DAGNode               = fetch::ledger::DAGNode;
  char const* CONTRACT_NAME = "zS6wg3ybTJYsItL/i1RBt7RYBYh/BR05vxM7WDuvaGA=.Z+ZQSog6NcP8LMGGpFHHWUeXRVXJT9AXZPvFWdrwT6iixAn7Q7KKKtiSviPWvPmT7KdKNbb8vs4oqg85PMsjaw==.synergetic";

  DAGTest() = default;

  void SetUp() override
  {
    // Component setup
    dag_ = UniqueDAG( new DAG() );
    chain_ = UniqueBlockChain( new FakeChain() );
    certificate_ = std::make_unique<fetch::crypto::ECDSASigner>();

    // Preparing genesis block    
    Block next_block;
    next_block.body.previous_hash = "genesis";
    next_block.body.block_number  = 0;
    next_block.body.miner         = "unkown";
    next_block.body.dag_nodes     = {};
    chain_->push_back(next_block);

    random_.Seed(42);
  }

  void TearDown() override
  {
    certificate_.reset();
    chain_.reset();
    dag_.reset();
  }

  void ExecuteRound(uint64_t N = 100)
  {

    GenerateDAGData(N);
    MakeBlock();
  }

  UniqueDAG&          dag() { return dag_; }
  uint64_t random() { return random_(); }
  UniqueBlockChain&   chain() { return chain_; }
private:
  void GenerateDAGData(uint64_t N = 100)
  {
    for(uint32_t i=0; i < N; ++i)
    {
      // Generating data
      fetch::byte_array::ByteArray data(32);
      for(uint32_t j=0; j < 32; ++j)
      {
        data[j] = (random_() >> 19) & 255;
      }

      // Creating test node
      DAGNode node;
      node.contents = data;

      // Adding it to the DAG
      node.contract_name = CONTRACT_NAME;
      dag_->SetNodeReferences(node);

      // Finalising node    
      node.identity = certificate_->identity();
      node.Finalise();
      if(!certificate_->Sign(node.hash))
      {
        FAIL() << "Signing failed";
      }
      node.signature = certificate_->signature();

      ASSERT_FALSE(dag_->HasNode(node.hash));

      // And pushing it to the DAG
      dag_->Push(node);
    }
  }

  void MakeBlock()
  {
    auto current_block = chain_->back();

    Block next_block;
    next_block.body.previous_hash = current_block.body.hash;
    next_block.body.block_number  = current_block.body.block_number + 1;
    next_block.body.miner         = "unkown";
    next_block.body.dag_nodes     = dag_->UncertifiedTipsAsVector();

    chain_->push_back(next_block);
    dag_->SetNodeTime(next_block);
  }
  

  UniqueDAG          dag_;
  UniqueBlockChain   chain_;
  UniqueCertificate  certificate_;
  RandomGenerator    random_;
};


TEST_F(DAGTest, BasicOperations)
{
  std::vector< uint64_t > dag_counters;
  dag_counters.push_back(0); // 0 dag nodes before genesis
  uint64_t total  = 1;       // 1 becuase of Genesis
  uint64_t rounds = 10;

  // Testing live execution for a number of rounds
  for(std::size_t i=0; i < rounds; ++i)
  {
    uint64_t N = random() % 10;
    dag_counters.push_back(N);
    total += N;

    ExecuteRound(N);

    EXPECT_EQ(dag()->size(), total);    
  }

  dag_counters.push_back(0); // There is no nodes after last block

  // Verifying DAG certification
  auto &ch = *chain();
  for(std::size_t i=0; i < ch.size(); ++i)
  {
    auto block   = ch[i];
    auto segment = dag()->ExtractSegment(block);
    EXPECT_EQ(segment.size(), dag_counters[i]);
  }

  // Test forward extract
  for(std::size_t i=0; i < ch.size(); ++i)
  {
    auto block   = ch[i];
    auto segment = dag()->GetBefore(block.body.dag_nodes, block.body.block_number, total);
    EXPECT_EQ(segment.size(), dag_counters[i]);
  }


  for(std::size_t i=1; i < ch.size(); ++i)
  {
    auto block   = ch[i];
    auto prev_block   = ch[i - 1];    

    // Note that as block N is not garantueed to certify all of block N-1
    // we need to take the combined set of the two hashes when getting 
    // the previous nodes
    std::vector< fetch::byte_array::ConstByteArray > hashes;
    for(auto &h: block.body.dag_nodes)
    {
      hashes.push_back(h);
    }
    for(auto &h: prev_block.body.dag_nodes)
    {
      hashes.push_back(h);
    }    

    auto segment = dag()->GetBefore(hashes, block.body.block_number - 1, total);
    EXPECT_EQ(segment.size(), dag_counters[i] + dag_counters[i - 1]);
  }  


  // TODO: Test remove node


}