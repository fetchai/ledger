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

#include "ledger/upow/synergetic_executor.hpp"
#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_register.hpp"
#include "core/random/lfg.hpp"

#include "mock_storage_unit.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <random>
#include <fstream>

class SynergeticExecutorTest : public ::testing::Test
{
public:
  using SynergeticExecutor    = fetch::consensus::SynergeticExecutor;
  using DAG                   = fetch::ledger::DAG;
  using Block                 = fetch::ledger::Block;
  using FakeChain             = std::vector< Block >;
  using SynergeticMiner       = fetch::consensus::SynergeticMiner;

  using UniqueExecutor        = std::unique_ptr< SynergeticExecutor >;
  using UniqueDAG             = std::unique_ptr< DAG >;
  using UniqueBlock           = std::unique_ptr< Block > ;  
  using UniqueBlockChain      = std::unique_ptr< FakeChain > ;
  using UniqueMiner           = std::unique_ptr< SynergeticMiner >;

  using MockStorageUnitPtr    = std::unique_ptr<MockStorageUnit>;
  using Address               = fetch::byte_array::ConstByteArray;
  using ContractRegister      = fetch::consensus::SynergeticContractRegister;
  using Work                  = fetch::consensus::Work;
  using RandomGenerator       = fetch::random::LaggedFibonacciGenerator<>;

  char const* CONTRACT_NAME = "fetch.synergetic";

  SynergeticExecutorTest() = default;

  void SetUp() override
  {
    storage_     = std::make_unique<MockStorageUnit>();
    dag_ = UniqueDAG( new DAG() );
    executor_ = UniqueExecutor( new SynergeticExecutor(*dag_.get(), *storage_.get()) );    
    chain_ = UniqueBlockChain( new FakeChain() );
    miner_ = UniqueMiner( new SynergeticMiner(*dag_.get()) );

    // Preparing genesis block
    Block next_block;
    next_block.body.previous_hash = "genesis";
    next_block.body.block_number  = 0;
    next_block.body.miner         = "unkown";
    next_block.body.dag_nodes     = {};

    chain_->push_back(next_block);

    // Adding contract
    std::string source;
    // TODO: load source
    if(!cregister_.CreateContract(CONTRACT_NAME, source))
    {
      std::cout << "Could not attach contract." << std::endl;
      exit(-1);
    }

    random_.Seed(42);    
  }

  void TearDown() override
  {
    miner_.reset();
    chain_.reset();
    executor_.reset();
    dag_.reset();
    storage_.reset();    
  }

  void ExecuteRound()
  {
    // Data is generated
    GenerateDAGData();

    // Block is mined
    MakeBlock();

    // Block is executed
    ExecuteBlock();

  }

private:
  void GenerateDAGData(uint64_t N = 100, uint64_t mine_every = 5)
  {
    for(uint32_t i=0; i < N; ++i)
    {
      if( (N % mine_every) == 0 )
      {
        // Mining solutions to the data submitted in the last round
        auto work = Mine();

        // Storing the work in the DAG
        fetch::ledger::DAGNode node;
        node.type = fetch::ledger::DAGNode::WORK;
        for(auto n: dag_->tips())
        {
          node.previous.push_back(n.first);
        }

        node.SetObject(work);
        node.contract_name = work.contract_name;

        dag_->Push(node);
      }
      else
      {
        // Generating data in this round
        fetch::ledger::DAGNode node;
        node.type = fetch::ledger::DAGNode::DATA;

        for(auto n: dag_->tips())
        {
          node.previous.push_back(n.first);
        }

        // TODO: Add relevant contents for the next mining process
        node.contents = "yada yada";

        dag_->Push(node);
      }
    }
  }

  void MakeBlock()
  {
    auto current_block = chain_->back();

    Block next_block;
    next_block.body.previous_hash = current_block.body.hash;
    next_block.body.block_number  = current_block.body.block_number + 1;
    next_block.body.miner        = "unkown";
    next_block.body.dag_nodes     = dag_->UncertifiedTipsAsVector();

    chain_->push_back(next_block);
  }
  
  void ExecuteBlock()
  {
    auto current_block = chain_->back();

    // Annotating nodes in the DAG
    dag_->SetNodeTime(current_block.body.block_number, current_block.body.dag_nodes);

    // Testing executor - preparing work
    executor_->PrepareWorkQueue(current_block);

    // Executing work
    executor_->ValidateWorkAndUpdateState();
  }

  Work Mine(int64_t search_length = 10) 
  {

    fetch::consensus::Work work;
    work.contract_name = CONTRACT_NAME;
    work.miner = "miner9";

    if(!miner_->DefineProblem(cregister_.GetContract(work.contract_name)))
    {
      std::cout << "Could not define problem!" << std::endl;
      exit(-1);
    }

    // Let's mine
    fetch::consensus::WorkRegister wreg;

    fetch::consensus::Work::ScoreType best_score = std::numeric_limits< fetch::consensus::Work::ScoreType >::max();
    for(int64_t i = 0; i < search_length; ++i) {
      work.nonce = 29188 + i;
      work.score = miner_->ExecuteWork(cregister_.GetContract(work.contract_name), work);

      if(work.score < best_score)
      {
        best_score = work.score;
      }
      
      wreg.RegisterWork(work);
    }

    // Returning the best work from this round
    return wreg.ClearWorkPool( cregister_.GetContract(work.contract_name) );
  }


  MockStorageUnitPtr storage_;
  UniqueDAG          dag_;
  UniqueExecutor     executor_;
  UniqueBlockChain   chain_;
  UniqueMiner        miner_;  
  ContractRegister   cregister_;  
  RandomGenerator    random_;
};


TEST_F(SynergeticExecutorTest, CheckMiningFlow)
{

}