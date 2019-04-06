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
#include "math/bignumber.hpp"

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

  using FakeStorageUnitPtr    = std::unique_ptr<FakeStorageUnit>;
  using Address               = fetch::byte_array::ConstByteArray;
  using ContractRegister      = fetch::consensus::SynergeticContractRegister;
  using Work                  = fetch::consensus::Work;
  using RandomGenerator       = fetch::random::LaggedFibonacciGenerator<>;
  using UniqueCertificate     = std::unique_ptr<fetch::crypto::ECDSASigner>;

  using ConstByteArray        = fetch::byte_array::ConstByteArray;
  using ContractName          = fetch::byte_array::ConstByteArray;
  using ContractAddress       = fetch::storage::ResourceAddress;  
  using Identifier            = fetch::ledger::Identifier;

  char const* CONTRACT_NAME = "zS6wg3ybTJYsItL/i1RBt7RYBYh/BR05vxM7WDuvaGA=.Z+ZQSog6NcP8LMGGpFHHWUeXRVXJT9AXZPvFWdrwT6iixAn7Q7KKKtiSviPWvPmT7KdKNbb8vs4oqg85PMsjaw==.synergetic";

  SynergeticExecutorTest() = default;

  void SetUp() override
  {
    // Component setup
    storage_     = std::make_unique<FakeStorageUnit>();
    dag_ = UniqueDAG( new DAG() );
    executor_ = UniqueExecutor( new SynergeticExecutor(*dag_.get(), *storage_.get()) );    
    chain_ = UniqueBlockChain( new FakeChain() );
    miner_ = UniqueMiner( new SynergeticMiner(*dag_.get()) );
    certificate_ = std::make_unique<fetch::crypto::ECDSASigner>();

    // Preparing genesis block
    Block next_block;
    next_block.body.previous_hash = "genesis";
    next_block.body.block_number  = 0;
    next_block.body.miner         = "unkown";
    next_block.body.dag_nodes     = {};
    chain_->push_back(next_block);

    // Adding contract
    std::string source;
    std::ifstream      file("./synergetic_test_contract.etch", std::ios::binary);
    if(!file)
    {
      throw std::runtime_error("Could not open contract code.");
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    source = ss.str();

    // Adding contract to stroge
    Identifier contract_id;
    EXPECT_TRUE(contract_id.Parse(CONTRACT_NAME));

    // Create the resource address for the contract
    ContractAddress contract_address = fetch::ledger::SmartContractManager::CreateAddressForContract(contract_id);    
    fetch::serializers::ByteArrayBuffer adapter;
    adapter << static_cast<ConstByteArray>(source);
    storage_->Set(contract_address, adapter.data());

    EXPECT_EQ(GetContract(), source);
    EXPECT_TRUE(cregister_.CreateContract(CONTRACT_NAME, source));

    // Clearing the register
    cregister_.Clear();

    // Testing that no work is deemed invalid
    executor_->OnDishonestWork([](Work /*work*/){
      FAIL() << "This test is not supposed to have any dishonest work.";
    });

    // Checking that winning work really is best in the segment.
    executor_->OnRewardWork([this](Work work){
      // The work block number contains the block number at which 
      // the data should be extract from. The work is located in the 
      // following segment 
      auto block = chain_->at(work.block_number + 1);
      auto segment = dag_->ExtractSegment(block);

      for(auto &s: segment)
      {
        if(s.type == fetch::ledger::DAGNode::WORK)
        {
          Work w;
          s.GetObject(w);

          if(w.score < work.score)
          {
            std::cout << w.contract_name << " "  << std::endl;
            FAIL() << "There exists work in segment that performs better: " << w.score << " vs " << work.score;
          }
        }
      }

    });

    random_.Seed(42);
  }

  std::string GetContract()
  {
    Identifier contract_id;
    EXPECT_TRUE(contract_id.Parse(CONTRACT_NAME));

    // create the resource address for the contract
    ContractAddress contract_address = fetch::ledger::SmartContractManager::CreateAddressForContract(contract_id);    

    auto const result = storage_->Get(contract_address);    
    EXPECT_FALSE(result.failed);

    ConstByteArray source; 
    fetch::serializers::ByteArrayBuffer adapter{result.document};
    adapter >> source;
    return static_cast<std::string>(source);
  }

  void TearDown() override
  {
    certificate_.reset();
    miner_.reset();
    chain_.reset();
    executor_.reset();
    dag_.reset();
    storage_.reset();    
  }

  void ExecuteRound(uint64_t N = 100, uint64_t mine_every = 5)
  {
    if(!cregister_.HasContract(CONTRACT_NAME))
    {
      EXPECT_TRUE(cregister_.CreateContract(CONTRACT_NAME, GetContract()));
    }

    // Data is generated
    GenerateDAGData(N, mine_every);

    // The remaining bits should work with out the contract register
    cregister_.Clear();

    // Block is mined
    MakeBlock();

    // Block is executed
    ExecuteBlock();
  }

  UniqueDAG&          dag() { return dag_; }
  uint64_t random() { return random_(); }
  UniqueBlockChain&   chain() { return chain_; }
private:
  void GenerateDAGData(uint64_t N = 100, uint64_t mine_every = 5)
  {
    for(uint32_t i=0; i < N; ++i)
    {
      if( (i % mine_every) == 0 )
      {
        // Mining solutions to the data submitted in the last round
        auto work = Mine();
        miner_->SetBlock(chain_->back());

        // Keeping the old segment for later testing.
        auto old_segment = dag_->ExtractSegment(chain_->back());
        assert(work.block_number == chain_->back().body.block_number);

        { // Testing that work was correctly mined
          EXPECT_EQ( work.contract_name, CONTRACT_NAME );
          miner_->AttachContract(*storage_, cregister_.GetContract(work.contract_name));
          miner_->DefineProblem();

          auto test_score = miner_->ExecuteWork(work);
          EXPECT_EQ(test_score, work.score);

          miner_->DetachContract();
        }

        // Storing the work in the DAG
        auto node = executor_->SubmitWork(certificate_, work);
        EXPECT_TRUE( bool(node) );

        // Testing that nodes in two segments are the same
        auto new_segment = dag_->ExtractSegment(chain_->back());        
        EXPECT_EQ( old_segment.size(), new_segment.size() );
        EXPECT_EQ( old_segment, new_segment);

        { // Testing that work is correctly deserialised
          Work work2;
          node.GetObject(work2);
          work2.contract_name = node.contract_name;
          work2.miner = node.identity.identifier();

          EXPECT_EQ(work, work2);

          miner_->AttachContract(*storage_, cregister_.GetContract(work2.contract_name));
          miner_->DefineProblem();
          auto test_score2 = miner_->ExecuteWork(work2);
          EXPECT_EQ(test_score2, work.score);
          miner_->DetachContract();        
        }
      }
      else
      {
        // Generating data in this round
        EXPECT_TRUE(miner_->AttachContract(*storage_, cregister_.GetContract(CONTRACT_NAME)));

        // Creating test node
        auto node = miner_->CreateDAGTestData(static_cast<int32_t>(chain_->size()), static_cast<int64_t>( random_() & ((1ul<<31) - 1ul ) ));
        miner_->DetachContract();

        // Adding it to the DAG
        node.contract_name = CONTRACT_NAME;
        dag_->SetNodeReferences(node);

        // Finalising node    
        node.identity = certificate_->identity();
        node.Finalise();
        if(!certificate_->Sign(node.hash))
        {
          throw std::runtime_error("Signing failed");
        }
        node.signature = certificate_->signature();

        // And pushing it to the DAG
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
    next_block.body.miner         = "unkown";
    next_block.body.dag_nodes     = dag_->UncertifiedTipsAsVector();

    chain_->push_back(next_block);
  }
  
  enum ExecuteStatus
  {
    SUCCESS = 0,
    REJECT_BLOCK
  };

  void ExecuteBlock()
  {
    using PStatus = SynergeticExecutor::PreparationStatusType;

    // Ignoring genesis block as no work was done prior to this
    if(chain_->size() < 2)
    {
      return;
    }

    auto current_block = chain_->back();
    auto previous_block = chain_->at(chain_->size() - 2);

    // Testing executor - preparing work
    EXPECT_EQ(PStatus::SUCCESS, executor_->PrepareWorkQueue(previous_block, current_block)) ;

    // Executing work
    EXPECT_TRUE(executor_->ValidateWorkAndUpdateState());
  }

  Work Mine(int32_t search_length = 10) 
  {
    Work work = executor_->Mine(certificate_, CONTRACT_NAME, chain_->back(), 277326, search_length);
    EXPECT_TRUE(bool(work));

    return work;
  }

  FakeStorageUnitPtr storage_;
  UniqueDAG          dag_;
  UniqueExecutor     executor_;
  UniqueBlockChain   chain_;
  UniqueMiner        miner_;  
  UniqueCertificate  certificate_;
  ContractRegister cregister_;  
  RandomGenerator    random_;
};


TEST_F(SynergeticExecutorTest, CheckMiningFlow)
{
  std::vector< uint64_t > dag_counters;
  dag_counters.push_back(0); // 0 dag nodes before genesis
  uint64_t total  = 1;       // 1 becuase of Genesis
  uint64_t rounds = 30;

  // Testing live execution for a number of rounds
  for(std::size_t i=0; i < rounds; ++i)
  {
    uint64_t N = random() % 100;
    dag_counters.push_back(N);
    total += N;

    ExecuteRound(N);
    EXPECT_TRUE(dag()->size() == total);    
  }  

  // Verifying DAG certification
  auto &ch = *chain();
  for(std::size_t i=0; i < ch.size(); ++i)
  {
    auto block = ch[i];
    auto segment = dag()->ExtractSegment(block);

    for(auto &s: segment)
    {
      if(s.type == fetch::ledger::DAGNode::WORK)      
      {
        fetch::consensus::Work work;
        s.GetObject(work);
        EXPECT_EQ(block.body.block_number - 1, work.block_number);
      }
    }
    
    EXPECT_TRUE(segment.size() == dag_counters[i]);
  }  
}